package com.divisionind.hq.ui;

import com.divisionind.hq.Controller;
import com.divisionind.hq.api.HackQuad;
import com.divisionind.hq.api.event.EventHandler;
import com.divisionind.hq.api.event.Listener;
import com.divisionind.hq.api.event.events.ConnectionTimeoutEvent;
import com.divisionind.hq.api.event.events.StatusUpdateEvent;
import com.divisionind.hq.ex.ValidateHackQuadException;
import com.studiohartman.jamepad.ControllerManager;
import com.studiohartman.jamepad.ControllerState;
import javafx.application.Application;
import javafx.application.Platform;
import javafx.fxml.FXMLLoader;
import javafx.fxml.Initializable;
import javafx.scene.Scene;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.control.*;
import javafx.scene.layout.Pane;
import javafx.stage.Stage;

import java.awt.*;
import java.io.IOException;
import java.net.*;
import java.util.Optional;
import java.util.ResourceBundle;

public class UIMain extends Application implements Initializable, Listener {

    private static final String PROJECT_PAGE = "https://github.com/divisionind/hackquad";

    private static final float DEADZONE     = 0.15f;
    private static final float MAX_ANGLE    = 30.0f;
    private static final float MAX_YAW_RATE = 70.0f;

    private static final float MIN_THROTTLE = 0.04f, MAX_THROTTLE = 0.4f;

    public Button connectButton;
    public TextField hostField;
    public Label ipLabel;
    public Label batteryLabel;
    public Label rssiLabel;
    public Label lastStatusLabel;
    public Label controllerStatusLabel;
    public TitledPane connectionPane;

    /* temp ui part */
    public Label throttleLabel;
    public Label setPitchLabel;
    public Label setRollLabel;
    public Label setYawLabel;
    public Label fcRefreshLabel;
    public Label angleXLabel;
    public Label angleYLabel;
    public Label angleZLabel;

    private Thread controllerThread;
    private ControllerManager controllerManager;
    private boolean controllerThreadActive;
    private float throttle = 0.0f, desiredPitch = 0.0f, desiredRoll = 0.0f, desiredYawRate = 0.0f;

    @Override
    public void start(Stage stage) throws Exception {
        Pane startPane = FXMLLoader.load(getClass().getResource("/com/divisionind/hq/ui/uimain.fxml"));
        Scene scene = new Scene(startPane, 900, 550);

        stage.setTitle(String.format("HackQuad Controller v%s (git: %s)", Controller.VERSION, Controller.GIT_HASH));
        stage.setScene(scene);
        stage.show();
    }

    @Override
    public void initialize(URL location, ResourceBundle resources) {
        controllerThread = new Thread(this::controllerTask);
        controllerThread.setDaemon(true);
        controllerThread.setName("controller-daemon");
        controllerThread.start();

        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            // shutdown controller lib
            controllerThreadActive = false;
        }));

        // set handler for validation exceptions
        Thread.setDefaultUncaughtExceptionHandler((thread, ex) -> {
            try {
                Throwable fromJavafx = ex.getCause().getCause();

                if (fromJavafx instanceof ValidateHackQuadException) {
                    Platform.runLater(this::sendNotConnectedAlert);
                    return;
                }
            } catch (NullPointerException e) { }

            ex.printStackTrace();
        });
    }

    private void restoreConnectButton() {
        connectButton.setDisable(false);
        connectButton.setText("Connect");
    }

    public void disconnect() {
        connectButton.setDisable(true);

        new Thread(() -> {
            try {
                Controller.getHackQuad().close();
            } catch (IOException e) {
                e.printStackTrace();
            }

            // looks better with a little delay
            try {
                Thread.sleep(100);
            } catch (InterruptedException e) { }

            Platform.runLater(() -> {
                ipLabel.setText("--");
                batteryLabel.setText("--V (--%)");
                rssiLabel.setText("--dB");
                lastStatusLabel.setText("--");

                restoreConnectButton();
            });
            Controller.setHackQuad(null);
        }).start();
    }

    public void onConnectButton() {
        // must be disconnect
        if (Controller.getHackQuad() != null) {
            disconnect();
            return;
        }

        // ensure ctrl is zero
        throttle = 0;
        desiredPitch = 0;
        desiredRoll = 0;
        desiredYawRate = 0;

        // must be connect
        connectButton.setDisable(true);
        connectButton.setText("Connecting...");

        new Thread(() -> {
            String addr = hostField.getText();
            if (addr == null || addr.equals(""))
                addr = "hackquad.local";

            try {
                Controller.setHackQuad(HackQuad.open(addr));
            } catch (SocketException | UnknownHostException e) {
                e.printStackTrace();

                Platform.runLater(() -> {
                    Alert connErr = new Alert(Alert.AlertType.ERROR);
                    connErr.setTitle("Failed to connect");
                    connErr.setContentText(String.format("Failed to connect a HackQuad at the specified address. Error (%s: %s)", e.getClass().getName(), e.getLocalizedMessage()));
                    connErr.showAndWait();
                    restoreConnectButton();
                });
                return;
            }

            Controller.getHackQuad().getEventManger().registerListeners(this);

            // connection successful
            Platform.runLater(() -> {
                connectButton.setDisable(false);
                connectButton.setText("Disconnect");
                ipLabel.setText(Controller.getHackQuad().getHost());
            });
        }).start();
    }

    public void onEditRegistry() {
        Controller.validateHackQuad();

        try {
            UIRegistry.open();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Deprecated
    public void onCalibrateMPU() {
        Controller.validateHackQuad();

        // show confirmation
        ButtonType buttonYes = new ButtonType("Yes", ButtonBar.ButtonData.YES);
        ButtonType buttonNo = new ButtonType("No", ButtonBar.ButtonData.NO);
        Alert alert = new Alert(Alert.AlertType.CONFIRMATION, null, buttonYes, buttonNo);
        alert.setTitle("Run calibration?");
        alert.setContentText("DO NOT USE!\n\nPlace the quadcopter upside-down on as level a surface as possible before starting.\n\n" +
                "During MPU calibration, the light will blink rapidly. After it slows, the " +
                "calibration is complete (this usually takes a few seconds). Do NOT move the quadcopter " +
                "during calibration.\n\n" +
                "Running MPU calibration will overwrite the offsets in the registry with new ones. Do you " +
                "wish to continue?");

        // check ret
        Optional<ButtonType> ret = alert.showAndWait();
        if (ret.isPresent() && ret.get().equals(buttonYes)) {
            Controller.getHackQuad().calibrate();
        }
    }

    public void onExit() {
        System.exit(0);
    }

    public void onViewGithub() {
        if (Desktop.isDesktopSupported() && Desktop.getDesktop().isSupported(Desktop.Action.BROWSE)) {
            try {
                Desktop.getDesktop().browse(new URI(PROJECT_PAGE));
            } catch (IOException | URISyntaxException e) {
                e.printStackTrace();
            }
        }
    }

    public void onAbout() {
        Alert alert = new Alert(Alert.AlertType.INFORMATION);
        alert.setContentText("HackQuad is an open-source, hackable, IoT quadcopter platform created by Andrew Howard (https://divisionind.com).");
        alert.setTitle("About");
        alert.showAndWait();
    }

    private void sendNotConnectedAlert() {
        Alert alert = new Alert(Alert.AlertType.ERROR);
        alert.setContentText("Error: HackQuad must be connected to use this action.");
        alert.setTitle("Error");
        alert.showAndWait();
    }

    private static float stickToPercent(float stick) {
        return ((Math.abs(stick) - DEADZONE) / (1.0f - DEADZONE)) * (stick < 0 ? -1 : 1);
    }

    private static float stickToPercent(float stick, float deadzone) {
        return ((Math.abs(stick) - deadzone) / (1.0f - deadzone)) * (stick < 0 ? -1 : 1);
    }

    @EventHandler
    private void onStatusUpdate(StatusUpdateEvent event) {
        Platform.runLater(() -> {
            rssiLabel.setText(String.format("(%d/5) %ddB", event.getParent().getConnectionStrength() + 1, event.getRSSI()));
            batteryLabel.setText(String.format("%.3fV (%d%%)", event.getBattery(), Math.round(estimateBatteryPercent(event.getBattery()))));

            fcRefreshLabel.setText(String.format("%.0f", 1000.0f / event.getFcLoopTime()));
            angleXLabel.setText(String.format("%.2f", event.getPitch()));
            angleYLabel.setText(String.format("%.2f", event.getRoll()));
            angleZLabel.setText(String.format("%.2f", event.getYaw()));
            // could just print time of last update at bottom corner of ui instead of connected (use color to show that)
        });
    }

    // verrryy rough
    private static float estimateBatteryPercent(float V) {
        final float a = 0.608033063f, b = 0.006425956f, c = 11.35951638f, d = 3.392643243f;
        return a / (b + (float) Math.exp(-c * (V - d)));
    }

    @EventHandler
    private void onConnectionTimeout(ConnectionTimeoutEvent event) {
        disconnect();

        Platform.runLater(() -> {
            Alert alert = new Alert(Alert.AlertType.ERROR);
            alert.setTitle("Disconnected");
            alert.setContentText("The connection was lost.");
            alert.showAndWait();
        });
    }

    /**
     * maps 0-1 -> min-max
     * @param var
     * @param min
     * @param max
     * @return
     */
    private static float mapVariable(float var, float min, float max) {
        float diff = max - min;
        return min + (var * diff);
    }

    // this is here because i was playing around w/ different control schemes
    // never planned to leave it here but i didn't have the heart to change it because this projects kinda nostalgic for me now
    private void controllerTask() {
        // init controller lib
        controllerManager = new ControllerManager();
        controllerManager.initSDLGamepad();

        controllerThreadActive = true;
        long lastControllerCheck = 0, currentTime, lastUIUpdate = 0, lastTrimIncr = 0;
        boolean aButtonStatus = false;
        boolean flying = false;

        do {
            ControllerState state = controllerManager.getState(0);

            if (state.isConnected) {
                // only trigger event when the state has changed
                if (state.lb != aButtonStatus) {
                    aButtonStatus = state.lb;
                    // Platform.runLater(() -> );
                }

                // kill throttle
                if (state.rb) {
                    flying = false;
                }

                // left stick full up to enable flying
                if (state.leftStickY >= 1.f && !flying)
                    flying = true;

                //if (state.leftTrigger >= 0.05f && (flying || Math.abs(state.leftStickY) >= 0.05f)) {
                if (state.leftTrigger >= 0.05f && flying) {
                    //flying = true;
                    throttle = mapVariable(stickToPercent(state.leftTrigger, 0.05f), MIN_THROTTLE, MAX_THROTTLE);
                } else {
                    throttle = flying ? MIN_THROTTLE : 0.f;
                }

                desiredPitch = state.rightStickY * -1; // need to flip sign here to make ctrling easier
                if (Math.abs(desiredPitch) >= DEADZONE) {
                    desiredPitch = stickToPercent(desiredPitch) * MAX_ANGLE;
                } else desiredPitch = 0.0f;

                if (Math.abs(state.rightStickX) >= DEADZONE) {
                    desiredRoll = stickToPercent(state.rightStickX) * MAX_ANGLE;
                } else desiredRoll = 0.0f;

                if (Math.abs(state.leftStickX) >= DEADZONE) {
                    desiredYawRate = stickToPercent(state.leftStickX) * MAX_YAW_RATE * -1; //flip dir
                } else desiredYawRate = 0.0f;

                if (Controller.getHackQuad() != null)
                    Controller.getHackQuad().setControl(throttle, desiredPitch, desiredRoll, desiredYawRate, aButtonStatus);
            }

            // update ui controllerStatusLabel
            currentTime = System.currentTimeMillis();
            if ((currentTime - lastControllerCheck) >= 500) {
                lastControllerCheck = currentTime;
                Platform.runLater(() -> controllerStatusLabel.setText(state.isConnected ? "index 0" : "waiting on controller..."));
            }

            // update ui status info
            currentTime = System.currentTimeMillis();
            if ((currentTime - lastUIUpdate) > 10) {
                lastUIUpdate = currentTime;


                Platform.runLater(() -> {
                    if (Controller.getHackQuad() != null)
                        lastStatusLabel.setText(Controller.getHackQuad().isConnectionStable() ? "YES" : "NO");

                    // update ui
                    throttleLabel.setText(String.format("%.1f", throttle * 100.0f));
                    setPitchLabel.setText(String.format("%.1f", desiredPitch));
                    setRollLabel.setText(String.format("%.1f", desiredRoll));
                    setYawLabel.setText(String.format("%.1f", desiredYawRate));
                });
            }

            // time for ctx switch
            try {
                Thread.sleep(1);
            } catch (InterruptedException e) { }
        } while (controllerThreadActive);

        // cleanup
        controllerManager.quitSDLGamepad();
    }
}
