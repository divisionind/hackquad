package com.divisionind.hq.api.registry;

import com.divisionind.hq.api.HTTP;
import com.divisionind.hq.api.HackQuad;
import org.json.JSONArray;
import org.json.JSONObject;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public class RegistryImpl implements Registry {

    private final HackQuad hackQuad;

    public RegistryImpl(HackQuad hackQuad) {
        this.hackQuad = hackQuad;
    }

    private Object getValueFrom(JSONObject json, RegisterType type) {
        String key = "value";

        switch (type) {
            default:
            case REG_8B:
            case REG_16B:
            case REG_32B:
            case REG_64B:
                return json.getInt(key);
            case REG_FLT:
                return json.getFloat(key);
            case REG_STR:
                return json.getString(key);
        }
    }

    private String httpRoot() {
        return hackQuad.getHTTPRoot();
    }

    @Override
    public HackQuad getParent() {
        return hackQuad;
    }

    @Override
    public List<RemoteRegister> queryRegistry() {
        List<RemoteRegister> ret = new ArrayList<>();

        try {
            HTTP resp = HTTP.request(httpRoot() + "/reg/list", "GET", null);

            if (resp.getCode() == 200) {
                JSONArray jsonList = new JSONArray(resp.getResponse());
                Iterator<Object> jsonIt = jsonList.iterator();

                while (jsonIt.hasNext()) {
                    JSONObject curr = (JSONObject) jsonIt.next();
                    RegisterType type = RegisterType.getById(curr.getInt("type"));

                    ret.add(new RemoteRegisterImpl(this, curr.getString("key"), type, getValueFrom(curr, type)));
                }
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return ret;
    }

    @Override
    public RemoteRegister queryRegistryEntry(String key) {
        RemoteRegister ret = null;

        JSONObject body = new JSONObject();
        body.put("key", key);

        try {
            HTTP resp = HTTP.request(httpRoot() + "/reg/get", "GET", body.toString());

            if (resp.getCode() == 200) {
                JSONObject jsonResp = new JSONObject(resp.getResponse());
                RegisterType type = RegisterType.getById(jsonResp.getInt("type"));

                ret = new RemoteRegisterImpl(this, key, type, getValueFrom(jsonResp, type));
            }
        } catch (IOException e) {
            e.printStackTrace();
        }
        return ret;
    }

    @Override
    public void updateRegistryEntry(String key, Object value) throws RuntimeException {
        JSONObject body = new JSONObject();
        body.put("key", key);
        body.put("value", value);

        try {
            HTTP resp = HTTP.request(httpRoot() + "/reg/set", "POST", body.toString());
            if (resp.getCode() != 200)
                throw new RuntimeException("failed to set registry value");
        } catch (IOException e) {
            e.printStackTrace();
            throw new RuntimeException("IOException attempting to set registry value");
        }
    }
}
