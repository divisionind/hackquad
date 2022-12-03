package com.divisionind.hq.api.registry;

import com.divisionind.hq.api.registry.ex.RegistryRendererException;

public class StringRenderer implements RegistryRenderer {
    @Override
    public String render(Object value) throws RegistryRendererException {
        return (String) value;
    }

    @Override
    public Object parse(String value) throws RegistryRendererException {
        return value;
    }
}
