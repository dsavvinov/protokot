import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream

/**
 * Created by user on 7/20/16.
 */
object TestExtendedMessages {
    val baseMessage = SomeMessage.BuilderSomeMessage().setInt32Field(228).setStringField("Hello").build()

    val extendedMessage = ExtendedMessage.BuilderExtendedMessage()
            .setBytesField(ByteArray(10, { x -> x.toByte() } ))
            .setDoubleField(3.14)
            .setInt32Field(228)
            .setStringField("Hello")
            .build()

    fun testBaseMessageSerialization() {
        val s = ByteArrayOutputStream()
        val output = CodedOutputStream(s)

        baseMessage.writeTo(output)

        var ins = CodedInputStream(ByteArrayInputStream(s.toByteArray()))

        val readMsg = SomeMessage.BuilderSomeMessage().parseFrom(ins).build()
        assert(readMsg == baseMessage)
    }

    fun testExtendedMessageSerialization() {
        val s = ByteArrayOutputStream()
        val output = CodedOutputStream(s)

        extendedMessage.writeTo(output)

        var ins = CodedInputStream(ByteArrayInputStream(s.toByteArray()))

        val readMsg = ExtendedMessage.BuilderExtendedMessage().parseFrom(ins).build()

        assert(readMsg == extendedMessage)
    }

    fun testBaseToExtended() {
        val s = ByteArrayOutputStream()
        val output = CodedOutputStream(s)

        baseMessage.writeTo(output)

        var ins = CodedInputStream(ByteArrayInputStream(s.toByteArray()))

        val readMsg = ExtendedMessage.BuilderExtendedMessage().parseFrom(ins).build()

        assert(readMsg == extendedMessage)
    }

    fun testExtendedToBase() {
        val s = ByteArrayOutputStream()
        val output = CodedOutputStream(s)

        extendedMessage.writeTo(output)

        var ins = CodedInputStream(ByteArrayInputStream(s.toByteArray()))

        val readMsg = SomeMessage.BuilderSomeMessage().parseFrom(ins).build()

        assert(readMsg == baseMessage)
    }
}

fun main(args: Array<String>) {
    TestExtendedMessages.testBaseMessageSerialization()
    TestExtendedMessages.testExtendedMessageSerialization()
    TestExtendedMessages.testBaseToExtended()
    println("OK")
}
