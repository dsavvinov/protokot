//
// Created by user on 7/11/16.
//

#include "kotlin_class_generator.h"
#include <iostream>
#include "kotlin_enum_generator.h"
#include "kotlin_field_generator.h"
#include "kotlin_name_resolver.h"
#include <algorithm>

namespace google {
namespace protobuf {
namespace compiler {
namespace kotlin {

class FieldGenerator;   // declared in "kotlin_file_generator.h"
class NameResolver;     // declared in "kotlin_name_resolver.h"

void ClassGenerator::generateCode(io::Printer *printer, bool isBuilder) const {
    generateHeader(printer, isBuilder);
    printer->Indent();

    /**
    * Field generator should know if it is generating code for builder.
    * or for fair class to choose between 'val' and 'var'.
    * Also note that fields should be declared before init section.
    */
    for (FieldGenerator *gen: properties) {
        gen->generateCode(printer, isBuilder);
        printer->Print("\n");
    }

    printer->Print("\n");
    generateInitSection(printer);

    // enum declarations and nested classes declarations only for fair classes
    if (!isBuilder) {
        for (EnumGenerator *gen: enumsDeclaraions) {
            gen->generateCode(printer);
            printer->Print("\n");
        }

        for (ClassGenerator *gen: classesDeclarations) {
            gen->generateCode(printer);
            printer->Print("\n");
        }
    }

    // write serialization methods only for fair classes, read methods only for Builders)
    printer->Print("\n");
    generateWriteToMethod(printer);

    // builder, mergeFrom and only for fair classes
    if (!isBuilder) {
        printer->Print("\n");
        generateBuilder(printer);

        printer->Print("\n");
        generateMergeMethods(printer);
    }

    // build() and setters are only for builders
    if (isBuilder) {
        printer->Print("\n");
        generateBuildMethod(printer);

        printer->Print("\n");
        generateParseMethods(printer);
    }

    // getSize()
    generateGetSizeMethod(printer);

    printer->Outdent();
    printer->Print("}\n");
}

ClassGenerator::ClassGenerator(Descriptor const *descriptor, NameResolver * nameResolver)
    : descriptor(descriptor)
    , nameResolver(nameResolver)
{
    int field_count = descriptor->field_count();
    for (int i = 0; i < field_count; ++i) {
        FieldDescriptor const * fieldDescriptor = descriptor->field(i);
        properties.push_back(new FieldGenerator(fieldDescriptor, /* enclosingClass = */ this, nameResolver));
    }

    int nested_types_count = descriptor->nested_type_count();
    for (int i = 0; i < nested_types_count; ++i) {
        Descriptor const * nestedClassDescriptor = descriptor->nested_type(i);
        nameResolver->addClass(nestedClassDescriptor->name(), getFullType());
        classesDeclarations.push_back(new ClassGenerator(nestedClassDescriptor, nameResolver));
    }

    int enums_declarations_count = descriptor->enum_type_count();
    for (int i = 0; i < enums_declarations_count; ++i) {
        EnumDescriptor const * nestedEnumDescriptor = descriptor->enum_type(i);
        nameResolver->addClass(nestedEnumDescriptor->name(), getFullType());
        enumsDeclaraions.push_back(new EnumGenerator(nestedEnumDescriptor, nameResolver));
    }

    /**
     * Sort properties in ascending order on their tag numbers. This order
     * affects order of serialization and deserialization, thus fields will be
     * serialized in order of their tags, as demanded by Google
     */
    std::sort(properties.begin(), properties.end(),
              [](FieldGenerator const * first, FieldGenerator const * second) {
                  return first->getFieldNumber() < second->getFieldNumber();
              });
}

ClassGenerator::~ClassGenerator() {
    for (int i = 0; i < properties.size(); ++i) {
        delete properties[i];
    }

    for (int i = 0; i < classesDeclarations.size(); ++i) {
        delete classesDeclarations[i];
    }

    for (int i = 0; i < enumsDeclaraions.size(); ++i) {
        delete enumsDeclaraions[i];
    }
}

void ClassGenerator::generateBuilder(io::Printer * printer) const {
    //XXX: just reuse generateCode with flag isBuilder set
    generateCode(printer, /* isBuilder = */ true);
}

void ClassGenerator::generateMergeMethods(io::Printer *printer) const {
    map <string, string> vars;

    // mergeWith(other: Message)
    printer->Print("\n");
    vars["className"] = getFullType();
    printer->Print(vars, "fun mergeWith (other: $className$) {\n");
    printer->Indent();

    for (int i = 0; i < properties.size(); ++i) {
        vars["fieldName"] = properties[i]->simpleName;

        // concatenate repeated fields
        if (properties[i]->getProtoLabel() == FieldDescriptor::LABEL_REPEATED) {
            printer->Print(vars, "$fieldName$.addAll(other.$fieldName$)\n");
        }

        // Bytes type is handled separately
        else if (properties[i]->getProtoType() == FieldDescriptor::TYPE_BYTES) {
            vars["initValue"] = properties[i]->getInitValue();
            printer->Print(vars, "$fieldName$.plus(other.$fieldName$)\n");
        }

        // for all other cases just take other's field
        else {
            printer->Print(vars, "$fieldName$ = other.$fieldName$\n");
        }
    }

    printer->Outdent();
    printer->Print("}\n");


    // mergeFromWithSize(input: CodedInputStream, expectedSize: Int)
    printer->Print("\n");
    printer->Print(vars, "fun mergeFromWithSize (input: CodedInputStream, expectedSize: Int) {\n");
    printer->Indent();

    vars["builderName"] = getBuilderFullType();
    printer->Print(vars, "val builder = $builderName$()\n");
    printer->Print("mergeWith(builder.parseFromWithSize(input, expectedSize).build())");

    printer->Outdent();
    printer->Print("}\n");

    // mergeFrom(input: CodedInputStream)
    printer->Print("\n");
    printer->Print(vars, "fun mergeFrom (input: CodedInputStream) {\n");
    printer->Indent();

    vars["builderName"] = getBuilderFullType();
    printer->Print(vars, "val builder = $builderName$()\n");
    printer->Print("mergeWith(builder.parseFrom(input).build())");

    printer->Outdent();
    printer->Print("}\n");
}


void ClassGenerator::generateWriteToMethod(io::Printer *printer) const {
    // generate function header
    printer->Print("fun writeTo (output: CodedOutputStream) {"
                   "\n");
    printer->Indent();

    // generate code for serialization/deserialization of fields
    for (int i = 0; i < properties.size(); ++i) {
        properties[i]->generateSerializationCode(printer, /* isRead = */ false, /* noTag = */ false);
    }

    printer->Outdent();
    printer->Print("}\n");
}



void ClassGenerator::generateHeader(io::Printer * printer, bool isBuilder) const {
    // build list of arguments like 'field1: Type1, field2: Type2, ... '
    string argumentList = "";
    for (int i = 0; i < properties.size(); ++i) {
        argumentList += properties[i]->simpleName + ": " + properties[i]->getFullType() + " = " + properties[i]->getInitValue();
        if (i + 1 != properties.size()) {
            argumentList += ", ";
        }
    }

    map<string, string> vars;
    vars["name"] = isBuilder? getBuidlerSimpleType() : getSimpleType();
    vars["argumentList"] = argumentList;
    vars["maybePrivate"] = isBuilder? "" : " private";
    printer->Print(vars,
                   "class $name$$maybePrivate$ constructor ($argumentList$) {"
                           "\n"
    );
}

void ClassGenerator::generateBuildMethod(io::Printer * printer) const {
    map <string, string> vars;
    vars["returnType"] = getFullType();
    printer->Print(vars,
                    "fun build(): $returnType$ {\n");
    printer->Indent();

    // pass all fields to constructor of enclosing class
    printer->Print(vars,
                    "return $returnType$(");
    for (int i = 0; i < properties.size(); ++i) {
        printer->Print(properties[i]->simpleName.c_str());
        if (i + 1 != properties.size()) {
            printer->Print(", ");
        }
    }
    printer->Print(")\n");
    printer->Outdent();
    printer->Print("}\n");
}

void ClassGenerator::generateInitSection(io::Printer * printer) const {
    printer->Print("init {\n");
    printer->Indent();

    for (int i = 0; i < properties.size(); ++i) {
        map <string, string> vars;
        vars["name"] = properties[i]->simpleName;
        printer->Print(vars,
                       "this.$name$ = $name$"
                               "\n"
        );
    }

    printer->Outdent();
    printer->Print("}\n");
}

void ClassGenerator::generateParseMethods(io::Printer *printer) const {
    // ====== parseFieldFrom(input: CodedInputStream): Boolean =========
    map <string, string> vars;
    vars["builderName"] = getBuilderFullType();

    printer->Print("fun parseFieldFrom(input: CodedInputStream): Boolean {\n");
    printer->Indent();

    // messages are not required to end with 0-tag, therefore parsing method should check for EOF
    printer->Print("if (input.isAtEnd()) { return false }\n");

    // read tag and check if some field will follow (0-tag indicates end of message)
    printer->Print("val tag = input.readInt32NoTag()\n");
    printer->Print("if (tag == 0) { return false } \n");

    // parse tag into field number and wire type
    printer->Print("val fieldNumber = WireFormat.getTagFieldNumber(tag)\n");
    printer->Print("val wireType = WireFormat.getTagWireType(tag)\n");

    // 'when' to map fieldNumber into fieldName
    printer->Print("when(fieldNumber) {\n");
    printer->Indent();

    for (int i = 0; i < properties.size(); ++i) {
        vars["fieldNumber"] = std::to_string(properties[i]->getFieldNumber());
        vars["camelCaseName"] = name_resolving::makeFirstLetterUpper(properties[i]->simpleName);
        vars["kotlinFunSuffix"] = properties[i]->getKotlinFunctionSuffix();
        vars["kotlinWireType"] = properties[i]->getWireType();
        vars["dl"] = "$";
        printer->Print(vars, "$fieldNumber$ -> ");

        printer->Print("{\n");
        printer->Indent();

        // check that wire type of that field is equal to expected
        printer->Print(vars, "if (wireType != $kotlinWireType$) {\n");
        printer->Indent();
        printer->Print(vars, "throw InvalidProtocolBufferException(\""
                               "Error: Field number $fieldNumber$ has wire type $kotlinWireType$"
                               " but read $dl${wireType.toString()}\")");
        printer->Outdent();
        printer->Print("}\n");

        properties[i]->generateSerializationCode(printer, /* isRead = */ true, /* noTag = */ true);

        printer->Print(vars, "has$camelCaseName$ = true\n");
        printer->Outdent();
        printer->Print("}\n");
    }

    // TODO: add parsing of unknown fields

    printer->Outdent();
    printer->Print("}\n");  // when-clause

    printer->Print("return true");
    printer->Outdent();
    printer->Print("}\n");  // parseFieldFrom body


    // ====== parseFromWithSize(input: CodedInputStream, expectedSize: Int) =========
    printer->Print(vars,
                   "fun parseFromWithSize(input: CodedInputStream, expectedSize: Int): $builderName$ {\n");
    printer->Indent();

    // read while we won't exceed expected amount of bytes
    printer->Print("while(getSizeNoTag() < expectedSize) {\n");
    printer->Indent();

    printer->Print("parseFieldFrom(input)\n");

    printer->Outdent(); // while-loop;
    printer->Print("}\n");

    // check if we have read more than expected
    vars["dollar"] = "$";
    printer->Print(vars,
                   "if (getSizeNoTag() > expectedSize) { "
                           "throw InvalidProtocolBufferException(\"Error: expected size of message $dollar$expectedSize, but have read at least $dollar${getSizeNoTag()}\") "
                           "}\n");

    printer->Print("return this\n");

    printer->Outdent(); // function body
    printer->Print("}\n");


    // ======== parseFrom(input: CodedInputStream) =========
    printer->Print(vars,
                   "fun parseFrom(input: CodedInputStream): $builderName$ {\n");
    printer->Indent();
    printer->Print("while(parseFieldFrom(input)) {");
    printer->Print("}\n");

    printer->Print("return this\n");

    printer->Outdent(); // function body
    printer->Print("}\n");
}

void ClassGenerator::generateGetSizeMethod(io::Printer *printer) const {
    // getSize(): Int
    printer->Print("fun getSize(fieldNumber: Int): Int {\n");
    printer->Indent();

    printer->Print("var size = 0\n");
    for (int i = 0; i < properties.size(); ++i) {
        properties[i]->generateSizeEstimationCode(printer, "size", /* noTag = */ false);
    }

    printer->Print("size += WireFormat.getVarint32Size(size) + WireFormat.getTagSize(fieldNumber, WireType.LENGTH_DELIMITED)\n");
    printer->Print("return size\n");
    printer->Outdent();
    printer->Print("}\n");


    // getSizeNoTag(): Int
    printer->Print("fun getSizeNoTag(): Int {\n");
    printer->Indent();

    printer->Print("var size = 0\n");
    for (int i = 0; i < properties.size(); ++i) {
        properties[i]->generateSizeEstimationCode(printer, "size", /* noTag = */ false);
    }

    printer->Print("return size\n");
    printer->Outdent();
    printer->Print("}\n");
}

string ClassGenerator::getSimpleType() const {
    return descriptor->name();
}

string ClassGenerator::getFullType() const {
    return nameResolver->getClassName(getSimpleType());
}

string ClassGenerator::getBuilderFullType() const {
    return nameResolver->getBuilderName(getSimpleType());
}

string ClassGenerator::getBuidlerSimpleType() const {
    return "Builder" + getSimpleType();
}


} // namespace kotlin
} // namespace compiler
} // namespace protobuf
} // namespace google


