/**
 * Created by user on 7/6/16.
 */


object WireFormat {
    // couple of constants for magic numbers
    val TAG_TYPE_BITS: Int = 3
    val TAG_TYPE_MASK: Int = (1 shl TAG_TYPE_BITS) - 1
    val VARINT_INFO_BITS_COUNT: Int = 7
    val VARINT_INFO_BITS_MASK: Int = 0b01111111    // mask for separating lowest 7 bits, where actual information stored
    val VARINT_UTIL_BIT_MASK: Int = 0b10000000     // mask for separating highest bit, that indicates next byte presence
    val FIXED_32_BYTE_SIZE: Int = 4
    val FIXED_64_BYTE_SIZE: Int = 8

    fun getTagWireType(tag: Int): WireType {
        return WireType from (tag and TAG_TYPE_MASK).toByte()
    }

    fun getTagFieldNumber(tag: Int): Int {
        return tag ushr TAG_TYPE_BITS
    }

    // TODO: refactor casts into function overloading as soon as translator will support it
    fun getTagSize(fieldNumber: Int, wireType: WireType): Int {
        return getVarint32Size((fieldNumber shl 3) or wireType.ordinal)
    }

    fun getVarint32Size(value: Int): Int {
        var curValue = value
        var size = 0
        while (curValue != 0) {
            size += 1
            curValue = value ushr VARINT_INFO_BITS_COUNT
        }
        return size
    }

    fun getVarint64Size(value: Long): Int {
        var curValue = value
        var size = 0
        while (curValue != 0L) {
            size += 1
            curValue = value ushr VARINT_INFO_BITS_COUNT
        }
        return size
    }

    fun getZigZag32Size(value: Int): Int {
        return getVarint32Size((value shl 1) xor (value shr 31))
    }

    fun getZigZag64Size(value: Long): Int {
        return getVarint64Size((value shl 1) xor (value shr 63))
    }

    fun getInt32Size(fieldNumber: Int, value: Int): Int {
        return getTagSize(fieldNumber, WireType.VARINT) + getVarint32Size(value)
    }

    fun getUInt32Size(fieldNumber: Int, value: Int): Int {
        return getInt32Size(fieldNumber, value)
    }

    fun getInt64Size(fieldNumber: Int, value: Long): Int {
        return getTagSize(fieldNumber, WireType.VARINT) + getVarint64Size(value)
    }

    fun getUInt64Size(fieldNumber: Int, value: Long): Int {
        return getInt64Size(fieldNumber, value)
    }

    fun getBoolSize(fieldNumber: Int, value: Boolean): Int {
        val intValue = if (value) 1 else 0
        return getInt32Size(fieldNumber, intValue)
    }

    fun getEnumSize(fieldNumber: Int, value: Int): Int {
        return getInt32Size(fieldNumber, value)
    }

    fun getSInt32Size(fieldNumber: Int, value: Int): Int {
        return getTagSize(fieldNumber, WireType.VARINT) + getZigZag32Size(value)
    }

    fun getSInt64Size(fieldNumber: Int, value: Long): Int {
        return getTagSize(fieldNumber, WireType.VARINT) + getZigZag64Size(value)
    }

    fun getFixed32Size(fieldNumber: Int, value: Long): Int {
        return getTagSize(fieldNumber, WireType.FIX_32) + FIXED_32_BYTE_SIZE
    }

    fun getFixed64Size(fieldNumber: Int, value: Long): Int {
        return getTagSize(fieldNumber, WireType.FIX_64) + FIXED_64_BYTE_SIZE
    }

    fun getDoubleSize(fieldNumber: Int, value: Double): Int {
        return getTagSize(fieldNumber, WireType.FIX_32) + FIXED_32_BYTE_SIZE
    }

    fun getFloatSize(fieldNumber: Int, value: Float): Int {
        return getTagSize(fieldNumber, WireType.FIX_64) + FIXED_64_BYTE_SIZE
    }

    fun getStringSize(fieldNumber: Int, value: String): Int {
        if (value.length == 0)
            return 0
        val encodedStringSize = value.toByteArray(Charsets.UTF_8).size  //TODO: not sure if it's the best way to do it
        return encodedStringSize + getTagSize(fieldNumber, WireType.LENGTH_DELIMITED) + getVarint32Size(encodedStringSize)
    }

    fun getBytesSize(fieldNumber: Int, value: ByteArray): Int {
        if (value.size == 0)
            return 0
        return value.size + getTagSize(fieldNumber, WireType.LENGTH_DELIMITED) + getVarint32Size(value.size)
    }
}