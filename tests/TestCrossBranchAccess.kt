import java.io.ByteArrayInputStream
import java.io.ByteArrayOutputStream

/**
 * Created by user on 7/20/16.
 */

object TestCrossBranchAccess {
    fun testMessageSerialization() {
        val s = ByteArrayOutputStream()
        val outs = CodedOutputStream(s)

        val msg = Grandfather.FatherLeft.SonLeftLeft.BuilderSonLeftLeft()
                .setBrother(
                    Grandfather.FatherRight.SonRightLeft.BuilderSonRightLeft().setBar("Bar").build()
                )
                .setFather(
                    Grandfather.FatherLeft.BuilderFatherLeft().build()
                )
                .build()

        msg.writeTo(outs)

        var ins = CodedInputStream(ByteArrayInputStream(s.toByteArray()))

        val readMsg = Grandfather.FatherLeft.SonLeftLeft.BuilderSonLeftLeft().parseFrom(ins).build()
        assert(readMsg == msg)
    }
}

fun main(args: Array<String>) {
    TestCrossBranchAccess.testMessageSerialization()
    println("OK")
}
