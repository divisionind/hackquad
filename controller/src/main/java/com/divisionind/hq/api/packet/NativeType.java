package com.divisionind.hq.api.packet;

public enum NativeType {

    FLOAT(float.class, (out, data) -> out.writeFloat((float) data), HQBufferReader::readFloat),
    INT8(int.class,    (out, data) -> out.write((int) data),        HQBufferReader::read),
    INT32(int.class,   (out, data) -> out.writeInt((int) data),     HQBufferReader::readInt),
    CSTR(String.class, (out, data) -> out.writeStr((String) data),  HQBufferReader::readStr);

    private final Class<?> type;
    private final Writer writer;
    private final Reader reader;

    NativeType(Class<?> type, Writer writer, Reader reader) {
        this.type = type;
        this.writer = writer;
        this.reader = reader;
    }

    public Class<?> getType() {
        return type;
    }

    public void write(HQBufferWriter out, Object data) {
        writer.store(out, data);
    }

    public Object read(HQBufferReader in) {
        return reader.read(in);
    }

    private interface Writer {
        void store(HQBufferWriter out, Object data);
    }

    private interface Reader {
        Object read(HQBufferReader in);
    }
}
