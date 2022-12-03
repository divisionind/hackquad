package com.divisionind.hq.api.registry;

import com.divisionind.hq.api.registry.ex.RegistryRendererException;

import java.util.HashMap;

public class EnumRenderer extends HashMap<Integer, String> implements RegistryRenderer {

    private int counter;

    /**
     * Initializes the enum with a specified base.
     *
     * @param base
     */
    public EnumRenderer(int base) {
        this.counter = base;
    }

    /**
     * Initializes enum with base of zero.
     */
    public EnumRenderer() {
        this(0);
    }

    /**
     * Automatically add an entry to the enum, incrementing the id.
     *
     * @param entry
     * @return
     */
    public EnumRenderer add(String entry) {
        put(counter++, entry);

        return this;
    }

    public EnumRenderer add(String... entries) {
        for (String ent : entries)
            add(ent);

        return this;
    }

    @Override
    public String render(Object value) throws RegistryRendererException {
        String ret;

        ret = get(value);
        if (ret == null)
            throw new RegistryRendererException();
        return ret;
    }

    @Override
    public Object parse(String value) throws RegistryRendererException {
        for (Entry<Integer, String> ent : entrySet()) {
            if (ent.getValue().equals(value))
                return ent.getKey();
        }

        throw new RegistryRendererException();
    }
}
