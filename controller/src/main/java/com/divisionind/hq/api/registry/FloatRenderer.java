package com.divisionind.hq.api.registry;

import com.divisionind.hq.api.registry.ex.RegistryRendererException;

public class FloatRenderer implements RegistryRenderer {
    @Override
    public String render(Object value) throws RegistryRendererException {
        return Float.toString((float) value);
    }

    @Override
    public Object parse(String value) throws RegistryRendererException {
        try {
            return Float.parseFloat(value);
        } catch (NumberFormatException e) {
            throw new RegistryRendererException();
        }
    }
}
