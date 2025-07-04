import coap.MyServer;
import coap.RegisterResource;
import coap.ServiceResource;

public class Main {
    public static void main(String[] args) {
        MyServer server = new MyServer();
        server.add(new ServiceResource("service"));
        server.add(new RegisterResource("registration"));
        server.start();
        System.out.println("[Main] CoAP Server started.");
    }
}