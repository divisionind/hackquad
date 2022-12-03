package com.divisionind.hq.api.event.ex;

public class ListenerRegisterException extends RuntimeException {
    public ListenerRegisterException() {
    }

    public ListenerRegisterException(String message) {
        super(message);
    }

    public ListenerRegisterException(String message, Throwable cause) {
        super(message, cause);
    }

    public ListenerRegisterException(Throwable cause) {
        super(cause);
    }
}
