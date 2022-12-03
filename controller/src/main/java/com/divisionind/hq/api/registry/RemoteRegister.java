package com.divisionind.hq.api.registry;

/**
 * Download and create registry key table at {@link Registry} startup. Add initializer which can override the
 * rendering technique used to support the {@link EnumRenderer}.
 */
public interface RemoteRegister {
    Registry getManager();

    RegistryRenderer getRenderer();

    String getKey();

    RegisterType getType();

    Object getValue();

    void updateValue(Object nValue);
}
