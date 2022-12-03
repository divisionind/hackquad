package com.divisionind.hq.api;

import com.divisionind.hq.api.event.EventManager;
import com.divisionind.hq.api.event.EventManagerImpl;
import com.divisionind.hq.api.event.events.ConnectionTimeoutEvent;
import com.divisionind.hq.api.event.events.StatusUpdateEvent;
import com.divisionind.hq.api.packet.HQBufferReader;
import com.divisionind.hq.api.packet.HQBufferWriter;
import com.divisionind.hq.api.packet.UDPPacket;
import com.divisionind.hq.api.packet.inbound.HQIStatusUpdate;
import com.divisionind.hq.api.packet.outbound.HQOControl;
import com.divisionind.hq.api.registry.Registry;
import com.divisionind.hq.api.registry.RegistryImpl;

import java.io.IOException;
import java.net.*;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.ScheduledThreadPoolExecutor;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;

public class HackQuadImpl implements HackQuad {

    private DatagramSocket udpSocket;
    private SocketAddress udpAddress;
    private int udpNonce;
    private String hostIp;
    private String httpRoot;
    private Registry registry;
    private EventManager eventManager;

    private AtomicReference<HQOControl> controlData;

    private long lastStatusUpdate;
    private AtomicInteger rssi;
    private AtomicReference<Float> battery;

    private Thread udpSendTask;
    private ScheduledExecutorService scheduler;
    private ScheduledFuture<?> connectionTimeoutFuture;
    private Thread udpRecvTask;

    protected HackQuadImpl(String addr) throws SocketException, UnknownHostException {
        udpSocket = new DatagramSocket();
        udpAddress = new InetSocketAddress(addr, HackQuad.UDP_PORT);
        udpNonce = 0;
        InetAddress ipaddr = InetAddress.getByName(addr);
        hostIp = ipaddr.getHostAddress();
        httpRoot = "http://" + hostIp;
        registry = new RegistryImpl(this);
        eventManager = new EventManagerImpl(this);

        controlData = new AtomicReference<>(new HQOControl(0.0f, 0.0f, 0.0f, 0.0f, false));

        lastStatusUpdate = 0;
        rssi = new AtomicInteger(0);
        battery = new AtomicReference<>(0.0f);

        udpSendTask = new Thread(this::udpSendHandler);
        udpSendTask.setDaemon(true);
        udpSendTask.setName("udp_send_task");
        udpSendTask.start();

        udpRecvTask = new Thread(this::udpRecvHandler);
        udpRecvTask.setDaemon(true);
        udpRecvTask.setName("udp_recv_task");
        udpRecvTask.start();

        scheduler = new ScheduledThreadPoolExecutor(1);
        scheduler.scheduleAtFixedRate(this::connectionWatchdog, CONNECTION_TIMEOUT, CONNECTION_TIMEOUT, TimeUnit.MILLISECONDS);
    }

    @Override
    public void setControl(float throttle, float pitch, float roll, float yawRate, boolean flagClearPanic) {
        this.controlData.set(new HQOControl(throttle, pitch, roll, yawRate, flagClearPanic));
    }

    @Override
    public void send(UDPPacket packet) {
        HQBufferWriter out = new HQBufferWriter();

        // write id
        out.write(packet.id());

        // write nonce
        out.write24Int(udpNonce);
        incrementNonce();

        // write contents
        try {
            UDPPacket.serialize(out, packet);
        } catch (IllegalAccessException e) {
            e.printStackTrace();
            return;
        }

        byte[] buffer = out.toByteArray();
        DatagramPacket udp = new DatagramPacket(buffer, buffer.length, udpAddress);
        try {
            udpSocket.send(udp);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public int getRSSI() {
        return rssi.get();
    }

    private static boolean between(int number, int min, int max) {
        return number >= min && number <= max;
    }

    @Override
    public int getConnectionStrength() {
        int rssi = Math.abs(getRSSI());

        if (rssi < 50) {
            return 4;
        } else
        if (between(rssi, 50, 69)) {
            return 3;
        } else
        if (between(rssi, 70, 79)) {
            return 2;
        } else
        if (between(rssi, 80, 89)) {
            return 1;
        } else
        if (rssi >= 90) {
            return 0;
        }
        return 0;
    }

    @Override
    public float getBattery() {
        return battery.get();
    }

    @Override
    public boolean isConnectionStable() {
        return (System.currentTimeMillis() - lastStatusUpdate) < CONNECTION_STABLE_TIMEOUT;
    }

    @Override
    public void calibrate() {
        try {
            HTTP.request(httpRoot + "/mpu/calibrate", "POST", null);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public Registry getRegistry() {
        return registry;
    }

    @Override
    public String getHost() {
        return hostIp;
    }

    @Override
    public String getHTTPRoot() {
        return httpRoot;
    }

    @Override
    public EventManager getEventManger() {
        return eventManager;
    }

    private void incrementNonce() {
        udpNonce++;
        udpNonce &= 0xFFFFFF; // ensure never gets above 24-bit val
    }

    private void udpSendHandler() {
        while (!udpSocket.isClosed()) {
            try {
                Thread.sleep(HackQuad.CONTROL_UPDATE_RATE);
            } catch (InterruptedException e) { }

            send(controlData.get());
        }
    }

    private void connectionWatchdog() {
        if ((System.currentTimeMillis() - lastStatusUpdate) > CONNECTION_TIMEOUT) {
            try {
                close();
            } catch (Exception e) {
                e.printStackTrace();
            }
            getEventManger().callEvent(new ConnectionTimeoutEvent());
        }
    }

    private void udpRecvHandler() {
        byte[] buffer = new byte[256];
        DatagramPacket in = new DatagramPacket(buffer, buffer.length);

        while (!udpSocket.isClosed()) {
            try {
                udpSocket.receive(in);

                HQBufferReader reader = new HQBufferReader(buffer);
                int id = reader.read();

                switch (id) {
                    default:
                        break;
                    case 20 /* STATUS_UPDATE */:
                        HQIStatusUpdate status = new HQIStatusUpdate();
                        try {
                            UDPPacket.deserialize(reader, status);
                            status.fcLoopTime *= 1000.0f;

                            battery.set(status.battery);
                            rssi.set((byte) status.rssi);
                            lastStatusUpdate = System.currentTimeMillis();

                            getEventManger().callEventAsync(new StatusUpdateEvent(status.battery, (byte) status.rssi, status.fcLoopTime, status.angleX, status.angleY, status.angleZ, lastStatusUpdate));
                        } catch (IllegalAccessException e) { }
//                        battery.set(reader.readFloat());
//                        rssi.set((byte) reader.read()); // signed int
//                        lastStatusUpdate = System.currentTimeMillis();
                        break;
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    @Override
    public void close() {
        udpSocket.close();
        eventManager.shutdown();
        scheduler.shutdown();
    }
}
