package db;

import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;

public class Database {
    static final String JDBC_URL = "jdbc:mysql://localhost:3306/smartroom";
    static final String JDBC_USER = "admin";
    static final String JDBC_PASSWORD = "iotubuntu";

    public static void createDatabase() throws SQLException {
        String JDBC_URL1 = "jdbc:mysql://localhost:3306/";
        final String DATABASE_NAME = "smartroom";
        String createDatabaseSQL = "CREATE DATABASE IF NOT EXISTS " + DATABASE_NAME;   
    
        try (Connection conn = DriverManager.getConnection(JDBC_URL1, JDBC_USER, JDBC_PASSWORD);
             Statement stmt = conn.createStatement()) {
            stmt.execute(createDatabaseSQL);
            // Database creation is silent unless it fails
        } catch (SQLException e) {
            System.err.println("[Database] Failed to create database: " + e.getMessage());
        }
    }

    public static void deleteDatabase() throws SQLException {
        final String DATABASE_NAME = "smartroom";
        String deleteDatabaseSQL = "DROP DATABASE IF EXISTS " + DATABASE_NAME;

        try (Connection conn = DriverManager.getConnection(JDBC_URL, JDBC_USER, JDBC_PASSWORD);
             Statement stmt = conn.createStatement()) {
            stmt.execute(deleteDatabaseSQL);
            // Database deletion is silent unless it fails
        } catch (SQLException e) {
            System.err.println("[Database] Failed to delete database: " + e.getMessage());
        }
    }

    public static Connection getConnection() throws SQLException {
        return DriverManager.getConnection(JDBC_URL, JDBC_USER, JDBC_PASSWORD);
    }

    public static void createTableIPV6() throws SQLException {
        String createTableSQL = "CREATE TABLE IF NOT EXISTS ipv6_addresses (" +
                                "id INT AUTO_INCREMENT PRIMARY KEY, " +
                                "nodeip VARCHAR(89) NOT NULL, " +
                                "nodename VARCHAR(80) NOT NULL, " +
                                "resource VARCHAR(80) NOT NULL)";

        try (Connection conn = getConnection();
            Statement stmt = conn.createStatement()) {
            stmt.execute(createTableSQL);
            // Table creation is silent unless it fails
        } catch (SQLException e) {
            System.err.println("[Database] Table creation failed: " + e.getMessage());
        }
    }

    public static Boolean saveDeviceRegistration(String nodeIP, String nodeName, String[] resources) throws SQLException {
        String insertSQL = "INSERT INTO ipv6_addresses (nodeip, nodename, resource) VALUES (?, ?, ?)";
        
        try (Connection conn = getConnection();
            PreparedStatement pstmt = conn.prepareStatement(insertSQL)) {
            
            for (String resource : resources) {
                pstmt.setString(1, nodeIP);
                pstmt.setString(2, nodeName);
                pstmt.setString(3, resource);
                pstmt.addBatch();
            }
            
            pstmt.executeBatch();
            // Registration saved silently unless it fails
            return true;

        } catch (SQLException e) {
            System.err.println("[Database] Error saving node registration: " + e.getMessage());
            e.printStackTrace();
            return false;
        }
    }
    
    public static String getResourceIP(String resourceName) throws SQLException {
        String selectSQL = "SELECT nodeip FROM ipv6_addresses WHERE resource = ? LIMIT 1";
        
        try (Connection conn = getConnection();
             PreparedStatement pstmt = conn.prepareStatement(selectSQL)) {
            
            pstmt.setString(1, resourceName);
            ResultSet rs = pstmt.executeQuery();
            
            if (rs.next()) {
                String nodeIP = rs.getString("nodeip");
                return nodeIP;
            } else {
                return null;
            }
            
        } catch (SQLException e) {
            System.err.println("[Database] Error searching for resource: " + e.getMessage());
            e.printStackTrace();
            return null;
        }
    }
}
