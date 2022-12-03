package com.divisionind.hq.api.registry;

import com.divisionind.hq.api.HackQuadChild;

import java.util.List;

public interface Registry extends HackQuadChild {
    List<RemoteRegister> queryRegistry();

    RemoteRegister queryRegistryEntry(String key);

    void updateRegistryEntry(String key, Object value) throws RuntimeException;
}
