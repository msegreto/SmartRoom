import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.*;

public class DBSupport {
    private static final String JDBC_URL = "jdbc:mysql://localhost:3306/smartroom";
    private static final String JDBC_USER = "admin";
    private static final String JDBC_PASSWORD = "iotubuntu";

    private static Connection connection = null;

    // Initializes the connection to the database
    public static void connectToDatabase() {
        try {
            connection = DriverManager.getConnection(JDBC_URL, JDBC_USER, JDBC_PASSWORD);
            System.out.println("Database connection successful.");
        } catch (SQLException e) {
            System.err.println("Error during database connection: " + e.getMessage());
            System.exit(1);
        }
    }

    // Returns the list of unique services available on the network
    public static List<String> getAvailableServices() {
        List<String> services = new ArrayList<>();
        String query = "SELECT DISTINCT resource FROM ipv6_addresses";

        try (Statement stmt = connection.createStatement();
             ResultSet rs = stmt.executeQuery(query)) {

            while (rs.next()) {
                services.add(rs.getString("resource"));
            }

        } catch (SQLException e) {
            System.err.println("Error reading IPv6 services: " + e.getMessage());
        }

        return services;
    }

    // Returns the detailed list of all IPv6 nodes and services
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
            System.err.println("Error loading IPv6 services: " + e.getMessage());
        }

        return list;
    }
}
