package db;

import java.sql.Connection;
import java.sql.SQLException;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.Statement;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.List;

public class LoggerSaver {
    String resource = null;
    String dataType = null;

    public LoggerSaver(String resource) {
        this.resource = resource;

        if(isResourceFloatType())
            this.dataType = "FLOAT";
        else
            this.dataType = "VARCHAR(255)";

        try {
            createLogTable();
        } catch (SQLException e) {
            System.err.println("[LoggerSaver] Error initializing log table: " + e.getMessage());
            e.printStackTrace();
        }
    }
    
    private void createLogTable() throws SQLException {
        String createTableSQL = "CREATE TABLE IF NOT EXISTS " + resource + "_log ("
                + "id INT AUTO_INCREMENT PRIMARY KEY, "
                + "value " + dataType + " NOT NULL,"
                + "timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP "
                + ")";
        
        try (Connection conn = Database.getConnection();
            Statement stmt = conn.createStatement()) {
            stmt.execute(createTableSQL);
            // Table creation is silent unless it fails
        } catch (SQLException e) {
            System.err.println("[LoggerSaver] Error creating table: " + e.getMessage());
            e.printStackTrace();
        }
    }

    public void saveLog(String value) throws SQLException {
        String insertSQL = "INSERT INTO " + resource + "_log (value) VALUES (?)";
        
        try (Connection conn = Database.getConnection();
            PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            if(isResourceFloatType()){
                value = value.replace(",", ".");
                pstmt.setFloat(1, Float.parseFloat(value));
            }
            else
                pstmt.setString(1, value);
            pstmt.executeUpdate();
            // Log save is silent unless it fails
        } catch (SQLException e) {
            System.err.println("Error saving log: " + e.getMessage());
            e.printStackTrace();
        }
    }

    public void deleteLogTable() throws SQLException {
        String deleteTableSQL = "DROP TABLE IF EXISTS " + resource + "_log";
        
        try (Connection conn = Database.getConnection();
            Statement stmt = conn.createStatement()) {
            stmt.execute(deleteTableSQL);
            System.out.println("[LoggerSaver] Successfully deleted table: " + resource + "_log");
        } catch (SQLException e) {
            System.err.println("Error deleting table: " + e.getMessage());
            e.printStackTrace();
        }
    }

    private Boolean isResourceFloatType() {
        String[] floatResources = {
            "temp", "hum", "predt", "predh"
        };
        
        return List.of(floatResources).contains(resource);
    }
}
