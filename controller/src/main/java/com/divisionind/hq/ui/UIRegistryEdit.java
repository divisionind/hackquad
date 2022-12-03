package com.divisionind.hq.ui;

import com.divisionind.hq.api.registry.EnumRenderer;
import com.divisionind.hq.api.registry.ex.RegistryRendererException;
import javafx.geometry.Insets;
import javafx.geometry.Pos;
import javafx.scene.Node;
import javafx.scene.Scene;
import javafx.scene.control.*;
import javafx.scene.input.KeyCode;
import javafx.scene.layout.GridPane;
import javafx.stage.Modality;
import javafx.stage.Stage;

public class UIRegistryEdit {

    private RemoteRegistryEntry entry;

    private Stage stage;
    private Node elementValue;
    private Object ret;

    public UIRegistryEdit(RemoteRegistryEntry entry) {
        this.entry = entry;
    }

    public Object open() {
        stage = new Stage();
        stage.initModality(Modality.APPLICATION_MODAL);
        stage.setResizable(false);
        GridPane pane = new GridPane();
        pane.setAlignment(Pos.CENTER);
        pane.setPadding(new Insets(15, 15, 15, 15));
        pane.setVgap(8);
        pane.setHgap(8);

        // init ui
        Label labelValue = new Label("Value:");
        Button buttonSave = new Button("Save");

        if (entry.getBase().getRenderer() instanceof EnumRenderer) {
            ComboBox<String> box = new ComboBox<>();
            elementValue = box;

            EnumRenderer renderer = (EnumRenderer) entry.getBase().getRenderer();
            box.getItems().addAll(renderer.values());
            box.getSelectionModel().select(renderer.render((int) entry.getBase().getValue()));
        } else {
            TextField field = new TextField(entry.getBase().getRenderer().render(entry.getBase().getValue()));
            elementValue = field;

            field.setOnKeyPressed(event -> {
                if (event.getCode() == KeyCode.ENTER)
                    submitValue();
            });
        }

        buttonSave.setOnAction(event -> submitValue());

        pane.add(labelValue, 0, 0);
        pane.add(elementValue, 1, 0);
        pane.add(buttonSave, 1, 1);

        // show ui
        Scene scene = new Scene(pane, 400, 100);
        scene.setOnKeyPressed(event -> {
            if (event.getCode() == KeyCode.ESCAPE)
                stage.close();
        });
        stage.setTitle(String.format("Edit %s", entry.getKey()));
        stage.setScene(scene);
        stage.showAndWait();

        return ret;
    }

    private void submitValue() {
        String strValue;

        // get user-entered string representation
        if (entry.getBase().getRenderer() instanceof EnumRenderer) {
            ComboBox<String> box = (ComboBox<String>) elementValue;
            strValue = box.getSelectionModel().getSelectedItem();
        } else {
            TextField field = (TextField) elementValue;
            strValue = field.getText();
        }

        // attempt to parse string to proper type
        try {
            ret = entry.getBase().getRenderer().parse(strValue);
            stage.close();
        } catch (RegistryRendererException e) {
            Alert error = new Alert(Alert.AlertType.ERROR);
            error.setTitle("Error parsing value");
            error.setContentText("Failed to parse value to correct type. Check things like decimal places, " +
                    "extraneous characters, etc.");
            error.showAndWait();
        }
    }
}
