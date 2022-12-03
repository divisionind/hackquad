package com.divisionind.hq.api.event.events;

import com.divisionind.hq.api.event.Event;

public class StatusUpdateEvent extends Event {

    private final float battery;
    private final byte rssi;
    private final float fcLoopTime;
    private final float pitch;
    private final float roll;
    private final float yaw;
    private final long recvTime;

    public StatusUpdateEvent(float battery, byte rssi, float fcLoopTime, float pitch, float roll, float yaw, long recvTime) {
        this.battery = battery;
        this.rssi = rssi;
        this.fcLoopTime = fcLoopTime;
        this.pitch = pitch;
        this.roll = roll;
        this.yaw = yaw;
        this.recvTime = recvTime;
    }

    public float getBattery() {
        return battery;
    }

    public byte getRSSI() {
        return rssi;
    }

    public float getFcLoopTime() {
        return fcLoopTime;
    }

    public float getPitch() {
        return pitch;
    }

    public float getRoll() {
        return roll;
    }

    public float getYaw() {
        return yaw;
    }

    public long getRecvTime() {
        return recvTime;
    }
}
