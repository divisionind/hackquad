package com.divisionind.hq.api.packet.inbound;

import com.divisionind.hq.api.packet.NativeType;
import com.divisionind.hq.api.packet.PacketEntry;
import com.divisionind.hq.api.packet.UDPPacket;

public class HQIStatusUpdate implements UDPPacket {

    @PacketEntry(NativeType.FLOAT)
    public float battery;

    @PacketEntry(NativeType.INT8)
    public int rssi;

    @PacketEntry(NativeType.FLOAT)
    public float fcLoopTime;

    @PacketEntry(NativeType.FLOAT)
    public float angleX;

    @PacketEntry(NativeType.FLOAT)
    public float angleY;

    @PacketEntry(NativeType.FLOAT)
    public float angleZ;

    @Override
    public int id() {
        return 20;
    }
}
