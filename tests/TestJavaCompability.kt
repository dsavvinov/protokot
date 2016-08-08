import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream

/**
 * Created by user on 7/21/16.
 */

object TestJavaCompability {
    fun testReadFromJava() {
        val file = File("/home/user/Downloads/carkot/proto/compiler/tests/addressbook-out")
        val ins = CodedInputStream(FileInputStream(file))
        val ab = AddressBook.BuilderAddressBook().parseFrom(ins)
        for (msg in ab.people) {
            println("email = ${msg.email}")
            println("id = ${msg.id}")
            println("name = ${msg.name}")
            for (phone in msg.phones) {
                println("Entry in phones:")
                println("    type = ${phone.type.toString()}")
                println("    number = ${phone.number}")
            }
            println("==============================")
        }
    }

    fun testWriteToJava() {
        val file = File("/home/user/Downloads/carkot/proto/compiler/tests/addressbook-out")
        val ins = CodedOutputStream(FileOutputStream(file))
        val ab = AddressBook.BuilderAddressBook()
            .addPerson(
                    Person.BuilderPerson()
                        .setEmail("email@domain.com")
                        .setId(42)
                        .setName("John Doe")
                        .addPhoneNumber(
                                Person.PhoneNumber.BuilderPhoneNumber()
                                    .setType(Person.PhoneType.HOME)
                                    .setNumber("8-800-555-35-35")
                                    .build()
                        )
                        .build()
            )
            .addPerson(
                    Person.BuilderPerson()
                            .setEmail("dsadasda")
                            .setId(123)
                            .setName("wetyquwet")
                            .addPhoneNumber(
                                    Person.PhoneNumber.BuilderPhoneNumber()
                                            .setType(Person.PhoneType.MOBILE)
                                            .setNumber("0987654")
                                            .build()
                            )
                            .build()
            )
            .build()

        ab.writeTo(ins)
    }
}
fun main(args: Array<String>) {
    TestJavaCompability.testWriteToJava()
}