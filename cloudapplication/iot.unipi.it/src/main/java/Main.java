import coap.MyServer;
import coap.RegisterResource;
import coap.ServiceResource;
import db.Database;

public class Main {
    public static void main(String[] args) {
        System.out.println("[Main] Starting SmartRoom CoAP Server...");
        
        Database.deleteDatabase();
        Database.createDatabase();
        Database.createTableIPV6();
        
        MyServer server = new MyServer();
        server.add(new ServiceResource("service"));
        server.add(new RegisterResource("registration"));
        server.start();
        
        System.out.println("[Main] Server ready on port 5683");
        System.out.println("[Main] Endpoints: /registration, /service");
    }
}