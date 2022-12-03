package com.divisionind.hq.api.packet;

import java.io.ByteArrayInputStream;

public class HQBufferReader extends ByteArrayInputStream {

    public HQBufferReader(byte[] buf) {
        super(buf);
    }

    public int readInt() {
        return read() | read() << 8 | read() << 16 | read() << 24;
    }

    public float readFloat() {
        return Float.intBitsToFloat(readInt());
    }

    public String readStr() {
        int curr;
        StringBuilder str = new StringBuilder();

        while ((curr = read()) != 0) { // read till null-term
            str.append((char) curr);
        }

        return str.toString();
    }
}
