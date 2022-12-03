package com.divisionind.hq.ui;

import com.divisionind.hq.Controller;
import com.divisionind.hq.api.registry.RemoteRegister;
import javafx.collections.ObservableList;
import javafx.fxml.FXMLLoader;
import javafx.fxml.Initializable;
import javafx.scene.Scene;
import javafx.scene.control.TableColumn;
import javafx.scene.control.TableRow;
import javafx.scene.control.TableView;
import javafx.scene.control.cell.PropertyValueFactory;
import javafx.scene.input.KeyCode;
import javafx.stage.Modality;
import javafx.stage.Stage;

import java.io.IOException;
import java.net.URL;
import java.util.List;
import java.util.ResourceBundle;

public class UIRegistry implements Initializable {

    public static void open() throws IOException {
        Stage stage = new Stage();
        stage.setScene(new Scene(FXMLLoader.load(UIRegistry.class.getResource("/com/divisionind/hq/ui/uiregistry.fxml")), 650, 500));
        stage.setResizable(false);
        stage.getScene().setOnKeyPressed(event -> {
            // TODO ESCAPE code never makes it, idk why
            if (event.getCode() == KeyCode.ESCAPE)
                stage.close();
        });

        stage.setTitle(String.format("Remote Registry (http://%s/)", Controller.getHackQuad().getHost()));
        stage.initModality(Modality.APPLICATION_MODAL);
        stage.showAndWait();
    }

    public TableView<RemoteRegistryEntry> registryTable;
    public TableColumn<RemoteRegistryEntry, String> keyColumn;
    public TableColumn<RemoteRegistryEntry, String> valueColumn;

    private void onOpenEdit(RemoteRegistryEntry entry) {
        Object nValue = new UIRegistryEdit(entry).open();

        if (nValue != null) {
            entry.setValue(entry.getBase().getRenderer().render(nValue));
            entry.pushChanges();
            registryTable.refresh();
        }
    }

    @Override
    public void initialize(URL location, ResourceBundle resources) {
        // config table
        keyColumn.setCellValueFactory(new PropertyValueFactory<>("key"));
        valueColumn.setCellValueFactory(new PropertyValueFactory<>("value"));

        // query/load registry
        ObservableList<RemoteRegistryEntry> table = registryTable.getItems();
        List<RemoteRegister> remoteEntries = Controller.getHackQuad().getRegistry().queryRegistry();

        for (RemoteRegister ent : remoteEntries) {
            table.add(new RemoteRegistryEntry(ent));
        }

        // configure table for double-click-to-edit
        registryTable.setRowFactory(view -> {
            TableRow<RemoteRegistryEntry> row = new TableRow<>();

            // configure each row with our various event handlers
            row.setOnMouseClicked(event -> {
                if (!row.isEmpty() && event.getClickCount() == 2)
                    onOpenEdit(row.getItem());
            });

            return row;
        });
    }
}
