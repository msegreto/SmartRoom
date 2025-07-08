import java.sql.*;
import java.util.*;

import Ipv6Service;

public class DBSupport {
    private static final String JDBC_URL = "jdbc:mysql://localhost:3306/";
    private static final String JDBC_USER = "admin";
    private static final String JDBC_PASSWORD = "iotubuntu";

    private static Connection connection = null;

    // Inizializza la connessione al database
    public static void connectToDatabase() {
        try {
            connection = DriverManager.getConnection(JDBC_URL, JDBC_USER, JDBC_PASSWORD);
            System.out.println("Connessione al database riuscita.");
        } catch (SQLException e) {
            System.err.println("Errore durante la connessione al database: " + e.getMessage());
            System.exit(1);
        }
    }

    // Chiude la connessione al database (opzionale, buona pratica)
    public static void closeDatabase() {
        if (connection != null) {
            try {
                connection.close();
                System.out.println("Connessione al database chiusa.");
            } catch (SQLException e) {
                System.err.println("Errore nella chiusura della connessione: " + e.getMessage());
            }
        }
    }

    // Restituisce la lista dei servizi unici disponibili nella rete
    public static List<String> getAvailableServices() {
        List<String> services = new ArrayList<>();
        String query = "SELECT DISTINCT resource FROM ipv6_addresses";
    
        try (Statement stmt = connection.createStatement();
             ResultSet rs = stmt.executeQuery(query)) {

            while (rs.next()) {
                services.add(rs.getString("resource"));
            }

        } catch (SQLException e) {
            System.err.println("Errore nella lettura dei servizi IPv6: " + e.getMessage());
        }

        return services;
    }

    // Restituisce l'elenco dettagliato di tutti i nodi e servizi IPv6
    public static List<Ipv6Service> getIpv6Services() {
        List<Ipv6Service> list = new ArrayList<>();
        String query = "SELECT nodeip, nodename, resource FROM ipv6_addresses";

        try (Statement stmt = connection.createStatement();
             ResultSet rs = stmt.executeQuery(query)) {

            while (rs.next()) {
                list.add(new Ipv6Service(
                    rs.getString("nodeip"),
                    rs.getString("nodename"),
                    rs.getString("resource")
                ));
            }

        } catch (SQLException e) {
            System.err.println("Errore caricamento servizi IPv6: " + e.getMessage());
        }

        return list;
    }
}
