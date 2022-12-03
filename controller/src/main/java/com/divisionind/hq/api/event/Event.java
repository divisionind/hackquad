package com.divisionind.hq.api.event;

import com.divisionind.hq.api.HackQuad;
import com.divisionind.hq.api.HackQuadChild;

public class Event implements HackQuadChild {

    protected HackQuad hackQuad;

    @Override
    public HackQuad getParent() {
        return hackQuad;
    }
}
