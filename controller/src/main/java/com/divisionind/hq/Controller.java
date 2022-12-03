package com.divisionind.hq;

import com.divisionind.hq.api.HackQuad;
import com.divisionind.hq.ex.ValidateHackQuadException;
import com.divisionind.hq.ui.UIMain;
import javafx.application.Application;

import java.io.IOException;

public class Controller {

    public static final String VERSION = "@DivisionVersion@";
    public static final String GIT_HASH = "@DivisionGitHash@";

    private static HackQuad hackQuad;

    public static HackQuad getHackQuad() {
        return hackQuad;
    }

    public static void setHackQuad(HackQuad hackQuad) {
        if (Controller.getHackQuad() != null) {
            try {
                Controller.getHackQuad().close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }

        Controller.hackQuad = hackQuad;
    }

    public static HackQuad validateHackQuad() throws ValidateHackQuadException {
        HackQuad ret = getHackQuad();

        if (ret == null)
            throw new ValidateHackQuadException();

        return ret;
    }

    public static void main(String[] args) {
        Application.launch(UIMain.class);
    }
}
