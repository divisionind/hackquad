package com.divisionind.hq.api.packet.outbound;

import com.divisionind.hq.api.packet.NativeType;
import com.divisionind.hq.api.packet.PacketEntry;
import com.divisionind.hq.api.packet.UDPPacket;

public class HQOControl implements UDPPacket {

    @PacketEntry(NativeType.FLOAT)
    public float throttle;

    @PacketEntry(NativeType.FLOAT)
    public float pitch;

    @PacketEntry(NativeType.FLOAT)
    public float roll;

    @PacketEntry(NativeType.FLOAT)
    public float yawRate;

    public HQOControl(float throttle, float pitch, float roll, float yawRate, boolean flagClearPanic) {
        if (throttle < 0)
            throw new RuntimeException("negative throttle values are not acceptable");

        this.throttle = throttle;
        this.pitch = pitch;
        this.roll = roll;
        this.yawRate = yawRate;

        if (flagClearPanic)
            this.throttle *= -1.0f; // set sign-bit
    }

    @Override
    public int id() {
        return 69;
    }
}
