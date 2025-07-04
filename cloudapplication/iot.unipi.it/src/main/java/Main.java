import coap.MyServer;
import coap.RegisterResource;
import coap.ServiceResource;
import db.Database;

public class Main {
    static Database db = new Database();
    public static void main(String[] args) {
        Database.deleteDatabase();
        Database.createDatabase();
        Database.createTableIPV6();
        MyServer server = new MyServer();
        server.add(new ServiceResource("service"));
        server.add(new RegisterResource("registration"));
        server.start();
        System.out.println("[Main] CoAP Server started.");
    }
}