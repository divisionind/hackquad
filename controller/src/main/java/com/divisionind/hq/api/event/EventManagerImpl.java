package com.divisionind.hq.api.event;

import com.divisionind.hq.api.HackQuad;
import com.divisionind.hq.api.event.ex.EventDispatchException;
import com.divisionind.hq.api.event.ex.ListenerRegisterException;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadFactory;
import java.util.concurrent.atomic.AtomicInteger;

public class EventManagerImpl implements EventManager, ThreadFactory {

    private final HackQuad hackQuad;
    private final Map<Class<? extends Event>, List<ListenerEntry>> handlers;
    private final AtomicInteger latestThreadId;
    private final ExecutorService executorService;

    public EventManagerImpl(HackQuad hackQuad) {
        this.hackQuad = hackQuad;
        this.handlers = new HashMap<>();
        this.latestThreadId = new AtomicInteger(0);
        this.executorService = Executors.newFixedThreadPool(HANDLER_THREADS, this);
    }

    @Override
    public HackQuad getParent() {
        return hackQuad;
    }

    private void registerListener(Listener listener) {
        Method[] methods = listener.getClass().getDeclaredMethods();

        for (Method method : methods) {
            if (method.isAnnotationPresent(EventHandler.class)) {
                Class<?>[] params = method.getParameterTypes();

                // verify correct form
                if (params.length != 1 || !Event.class.isAssignableFrom(params[0]))
                    throw new ListenerRegisterException("Invalid event handler parameters.");

                // if private, allow access
                if (Modifier.isPrivate(method.getModifiers()))
                    method.setAccessible(true);

                // create list if doesnt exist already
                List<ListenerEntry> listenerEntries = handlers.get(params[0]);
                if (listenerEntries == null) {
                    listenerEntries = new ArrayList<>();
                    handlers.put((Class<? extends Event>) params[0], listenerEntries);
                }

                // add handler
                listenerEntries.add(new ListenerEntry(listener, method));
            }
        }
    }

    @Override
    public void registerListeners(Listener... listeners) {
        for (Listener listener : listeners)
            registerListener(listener);
    }

    @Override
    public void callEvent(Event event) {
        List<ListenerEntry> entries = handlers.get(event.getClass());

        if (entries != null) {
            Exception lastError = null;

            event.hackQuad = getParent();
            for (ListenerEntry entry : entries) {
                try {
                    entry.function.invoke(entry.listener, event);
                } catch (Exception e) {
                    lastError = e;
                }
            }

            if (lastError != null)
                throw new EventDispatchException("Failed to call one or many event handler(s).", lastError);
        }
    }

    @Override
    public void callEventAsync(Event event) {
        executorService.execute(() -> callEvent(event));
    }

    @Override
    public Thread newThread(Runnable r) {
        Thread ret = new Thread(r);
        ret.setDaemon(true);
        ret.setName(String.format("hq-eventhandler-%d", latestThreadId.getAndIncrement()));

        return ret;
    }

    @Override
    public void shutdown() {
        executorService.shutdown();
    }

    private static class ListenerEntry {

        private final Listener listener;
        private final Method function;

        public ListenerEntry(Listener listener, Method function) {
            this.listener = listener;
            this.function = function;
        }
    }
}
