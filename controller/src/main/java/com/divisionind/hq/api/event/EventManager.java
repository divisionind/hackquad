package com.divisionind.hq.api.event;

import com.divisionind.hq.api.HackQuadChild;

public interface EventManager extends HackQuadChild {

    /* number of threads to pool for handling async events */
    int HANDLER_THREADS = 2;

    void registerListeners(Listener... listeners);

    void callEvent(Event event);

    void callEventAsync(Event event);

    void shutdown();
}
