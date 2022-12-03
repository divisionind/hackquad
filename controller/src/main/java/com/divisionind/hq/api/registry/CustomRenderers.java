package com.divisionind.hq.api.registry;

public enum CustomRenderers {

    WIFI_AP_AUTHMODE(new EnumRenderer().add("WIFI_AUTH_OPEN", "WIFI_AUTH_WEP",
            "WIFI_AUTH_WPA_PSK", "WIFI_AUTH_WPA2_PSK", "WIFI_AUTH_WPA_WPA2_PSK", "WIFI_AUTH_WPA2_ENTERPRISE",
            "WIFI_AUTH_WPA3_PSK", "WIFI_AUTH_WPA2_WPA3_PSK")),
    WIFI_ST_AUTHMODE(WIFI_AP_AUTHMODE.customRenderer),
    WIFI_MODE(new EnumRenderer(1).add("WIFI_MODE_STA", "WIFI_MODE_AP", "WIFI_MODE_APSTA")),
    ;

    private final RegistryRenderer customRenderer;

    CustomRenderers(RegistryRenderer customRenderer) {
        this.customRenderer = customRenderer;
    }

    public RegistryRenderer getCustomRenderer() {
        return customRenderer;
    }
}
