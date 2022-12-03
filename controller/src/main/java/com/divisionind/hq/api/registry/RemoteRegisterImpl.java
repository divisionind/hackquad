package com.divisionind.hq.api.registry;

public class RemoteRegisterImpl implements RemoteRegister {

    private Registry manager;
    private RegistryRenderer renderer;
    private String key;
    private RegisterType type;
    private Object value;

    public RemoteRegisterImpl(Registry manager, String key, RegisterType type, Object value) {
        this.manager = manager;
        this.key = key;
        this.type = type;
        this.value = value;

        // handle custom renderers
        try {
            this.renderer = CustomRenderers.valueOf(key).getCustomRenderer();
            return;
        } catch (IllegalArgumentException e) { }

        // dynamically resolve renderer
        switch (type) {
            default:
            case REG_8B:
            case REG_16B:
            case REG_32B:
            case REG_64B:
                this.renderer = new IntegerRenderer();
                break;
            case REG_FLT:
                this.renderer = new FloatRenderer();
                break;
            case REG_STR:
                this.renderer = new StringRenderer();
                break;
        }
    }

    @Override
    public Registry getManager() {
        return manager;
    }

    @Override
    public RegistryRenderer getRenderer() {
        return renderer;
    }

    @Override
    public String getKey() {
        return key;
    }

    @Override
    public RegisterType getType() {
        return type;
    }

    @Override
    public Object getValue() {
        return value;
    }

    @Override
    public void updateValue(Object nValue) {
        value = nValue;
        manager.updateRegistryEntry(key, value);
    }
}
