package com.divisionind.hq.api;

import com.divisionind.hq.api.event.EventManager;
import com.divisionind.hq.api.packet.UDPPacket;
import com.divisionind.hq.api.registry.Registry;

import java.io.Closeable;
import java.net.SocketException;
import java.net.UnknownHostException;

public interface HackQuad extends Closeable {

    /* delay in ms between udp control update packets */
    long CONTROL_UPDATE_RATE = 20;

    /* port of udp control socket */
    int UDP_PORT = 25565;

    /* how long between status packet recv before isConnectionStable returns false */
    long CONNECTION_STABLE_TIMEOUT = 500;

    /* how long the connection can be unstable before it is terminated */
    long CONNECTION_TIMEOUT = 2000;

    static HackQuad open(String addr) throws SocketException, UnknownHostException {
        return new HackQuadImpl(addr);
    }

    void setControl(float throttle, float pitch, float roll, float yawRate, boolean flagClearPanic);

    void send(UDPPacket packet);

    int getRSSI();

    int getConnectionStrength();

    float getBattery();

    boolean isConnectionStable();

    void calibrate();

    Registry getRegistry();

    String getHost();

    String getHTTPRoot();

    EventManager getEventManger();
}
