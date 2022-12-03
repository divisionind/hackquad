package com.divisionind.hq.api.registry;

import com.divisionind.hq.api.registry.ex.RegistryRendererException;

public interface RegistryRenderer {
    /**
     * Renders the value as a string for display in the registry menu.
     *
     * @param value to represent
     * @return the rendered type
     * @throws RegistryRendererException when the action cannot be performed (invalid data)
     */
    String render(Object value) throws RegistryRendererException;

    /**
     * Parses a string representation back to its base value.
     *
     * @param value the previously rendered type
     * @return the correct value in the form originally given to the renderer
     * @throws RegistryRendererException when the action cannot be performed (invalid data)
     */
    Object parse(String value) throws RegistryRendererException;
}
