package com.divisionind.hq.api.registry;

public enum RegisterType {

    REG_8B,
    REG_16B,
    REG_32B,
    REG_64B,
    REG_STR,
    REG_FLT;

    private int id;

    public int getId() {
        return id;
    }

    public static RegisterType getById(int id) {
        for (RegisterType type : values()) {
            if (type.id == id)
                return type;
        }

        throw new IllegalArgumentException();
    }

    static {
        RegisterType[] all = RegisterType.values();

        for (int i = 0; i < all.length; i++) {
            all[i].id = i;
        }
    }
}
