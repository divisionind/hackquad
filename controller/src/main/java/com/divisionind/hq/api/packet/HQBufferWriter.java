package com.divisionind.hq.api.packet;

import java.io.ByteArrayOutputStream;

public class HQBufferWriter extends ByteArrayOutputStream {

    public void writeInt(int i) {
        write(i);
        write(i >> 8);
        write(i >> 16);
        write(i >> 24);
    }

    public void write24Int(int i) {
        write(i);
        write(i >> 8);
        write(i >> 16);
    }

    public void writeFloat(float f) {
        writeInt(Float.floatToIntBits(f));
    }

    public void writeStr(String str) {
        char[] cStr = str.toCharArray();
        for (char c : cStr)
            write(c);

        write(0); // null-term
    }
}
