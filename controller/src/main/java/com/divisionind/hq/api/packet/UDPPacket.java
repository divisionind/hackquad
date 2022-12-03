package com.divisionind.hq.api.packet;

import java.lang.reflect.Field;

public interface UDPPacket {
    static void serialize(HQBufferWriter out, UDPPacket packet) throws IllegalAccessException {
        Field[] fields = packet.getClass().getFields();

        for (Field f : fields) {
            PacketEntry meta = f.getAnnotation(PacketEntry.class);

            if (meta != null) {
                Object val = f.get(packet);
                meta.value().write(out, val);
            }
        }
    }

    static void deserialize(HQBufferReader in, UDPPacket packet) throws IllegalAccessException {
        Field[] fields = packet.getClass().getFields();

        for (Field f : fields) {
            PacketEntry meta = f.getAnnotation(PacketEntry.class);

            if (meta != null) {
                Object val = meta.value().read(in);
                f.set(packet, val);
            }
        }
    }

    int id();
}
