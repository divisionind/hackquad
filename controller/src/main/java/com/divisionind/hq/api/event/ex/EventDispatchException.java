package com.divisionind.hq.api.event.ex;

public class EventDispatchException extends RuntimeException {
    public EventDispatchException() {
    }

    public EventDispatchException(String message) {
        super(message);
    }

    public EventDispatchException(String message, Throwable cause) {
        super(message, cause);
    }

    public EventDispatchException(Throwable cause) {
        super(cause);
    }
}
