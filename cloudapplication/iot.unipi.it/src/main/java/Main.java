import coap.MyServer;
import coap.RegisterResource;
import coap.ServiceResource;
import db.Database;

public class Main {
    static Database database = new Database();
    public static void main(String[] args) {
        System.out.println("[Main] Starting SmartRoom CoAP Server...");
        try {
            Database.deleteDatabase();

            Database.createDatabase();
            Database.createTableIPV6();
        } catch (Exception e) {
            System.err.println("[Main] Database initialization failed: " + e.getMessage());
            e.printStackTrace();
        }
        MyServer server = new MyServer();
        
        server.add(new ServiceResource("service"));
        server.add(new RegisterResource("registration"));
        
        server.start();

        // Shutdown hook to clean up observers on server stop
        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            RegisterResource.shutdownObservers();
        }));
        
        System.out.println("[Main] Server ready on port 5683");
    }
}