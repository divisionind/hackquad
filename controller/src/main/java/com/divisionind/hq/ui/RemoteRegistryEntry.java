package com.divisionind.hq.ui;

import com.divisionind.hq.api.registry.RemoteRegister;
import com.divisionind.hq.api.registry.ex.RegistryRendererException;
import javafx.beans.property.SimpleStringProperty;

public class RemoteRegistryEntry {

    private final SimpleStringProperty key;
    private final SimpleStringProperty value;

    private final RemoteRegister base;

    public RemoteRegistryEntry(RemoteRegister base) {
        this.base = base;

        this.key = new SimpleStringProperty(this.base.getKey());
        this.value = new SimpleStringProperty(this.base.getRenderer().render(this.base.getValue()));
    }

    public void pushChanges() throws RegistryRendererException {
        Object resolvedValue = base.getRenderer().parse(getValue());
        base.updateValue(resolvedValue);
    }

    public String getKey() {
        return key.get();
    }

    public void setKey(String key) {
        this.key.set(key);
    }

    public String getValue() {
        return value.get();
    }

    public void setValue(String value) {
        this.value.set(value);
    }

    public RemoteRegister getBase() {
        return base;
    }
}
