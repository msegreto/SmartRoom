package coap;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.coap.MediaTypeRegistry;
import org.eclipse.californium.core.server.resources.CoapExchange;

import db.Database;

public class ServiceResource extends CoapResource {

    public ServiceResource(String name) {
        super(name);
        setObservable(true);
    }
    
    @Override
    public void handleGET(CoapExchange exchange) {
        try {
            String resourceName = null;
            
            // Estrai parametro resource dalla query string
            if (exchange.getRequestOptions().getUriQuery().size() > 0) {
                String query = exchange.getRequestOptions().getUriQuery().get(0);
                if (query != null && query.startsWith("resource=")) {
                    resourceName = query.substring("resource=".length());
                }
            }
            
            if (resourceName == null || resourceName.trim().isEmpty()) {
                exchange.respond(ResponseCode.BAD_REQUEST, 
                    "Missing resource parameter. Use: GET /service?resource=<resource_name>");
                return;
            }
            
            // Cerca la risorsa nel database
            String resourceIP = Database.getResourceIP(resourceName);
            
            if (resourceIP != null) {
                String fullResourceURI = "coap://[" + resourceIP + "]:5683/" + resourceName;
                exchange.respond(ResponseCode.CONTENT, fullResourceURI, MediaTypeRegistry.TEXT_PLAIN);
                System.out.println("[ServiceResource] Found " + resourceName + " at " + resourceIP);
            } else {
                exchange.respond(ResponseCode.NOT_FOUND, "Resource not found: " + resourceName);
                System.out.println("[ServiceResource] Resource not found: " + resourceName);
            }
            
        } catch (Exception e) {
            System.err.println("[ServiceResource] Error: " + e.getMessage());
            exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Server error");
        }
    }

}
