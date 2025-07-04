package db;

import java.sql.Connection;
import java.sql.SQLException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Timestamp;
import java.util.ArrayList;
import java.util.List;

public class LoggerSaver {
    String resource = null;

    public LoggerSaver(String resource) {
        this.resource = resource;
        try {
            createLogTable();
        } catch (SQLException e) {
            System.err.println("[LoggerSaver] Error initializing log table: " + e.getMessage());
            e.printStackTrace();
        }
    }
    
    private void createLogTable() throws SQLException {
        deleteLogTable();
        String createTableSQL = "CREATE TABLE IF NOT EXISTS " + resource + "_log ("
                + "id INT AUTO_INCREMENT PRIMARY KEY, "
                + "value VARCHAR(255) NOT NULL,"
                + "timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP "
                + ")";
        
        try (Connection conn = Database.getConnection();
            Statement stmt = conn.createStatement()) {
            stmt.execute(createTableSQL);
            System.out.println("[LoggerSaver] Successfully created table: " + resource + "_log");
        } catch (SQLException e) {
            System.err.println("Error creating table: " + e.getMessage());
            e.printStackTrace();
        }
    }

    public void saveLog(String value) throws SQLException {
        String insertSQL = "INSERT INTO " + resource + "_log (value) VALUES (?)";
        
        try (Connection conn = Database.getConnection();
            PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            pstmt.setString(1, value);
            pstmt.executeUpdate();
            System.out.println("[LoggerSaver] Successfully saved log for resource: " + resource + " with value: " + value);
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

    public List<Log> showLogs() throws SQLException {
        String selectSQL = "SELECT * FROM " + resource + "_log";
        List<Log> logs = new ArrayList<>();
        
        try (Connection conn = Database.getConnection();
            Statement stmt = conn.createStatement();
            ResultSet rs = stmt.executeQuery(selectSQL)) {
            
            System.out.println("[LoggerSaver] Logs for resource: " + resource);
            while (rs.next()) {
                Log log = new Log();
                log.id = rs.getInt("id");
                log.value = rs.getString("value");
                log.timestamp = rs.getTimestamp("timestamp");
                logs.add(log);
            }
            return logs;

        } catch (SQLException e) {
            System.err.println("Error retrieving logs: " + e.getMessage());
            e.printStackTrace();
        }
        return null;
    }

    private class Log{
        private int id;
        private String value;
        private Timestamp timestamp;
    }
}
