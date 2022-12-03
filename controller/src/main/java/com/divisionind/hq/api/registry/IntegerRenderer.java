package com.divisionind.hq.api.registry;

import com.divisionind.hq.api.registry.ex.RegistryRendererException;

public class IntegerRenderer implements RegistryRenderer {
    @Override
    public String render(Object value) throws RegistryRendererException {
        return Integer.toString((int) value);
    }

    @Override
    public Object parse(String value) throws RegistryRendererException {
        try {
            return Integer.parseInt(value);
        } catch (NumberFormatException e) {
            throw new RegistryRendererException();
        }
    }
}
