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
        System.out.println("[ServiceResource] === INCOMING GET REQUEST ===");
        
        try {
            // Estrai il parametro della risorsa dalla query string
            // Esempio: GET /service?resource=temp
            String resourceName = null;
            
            // Prova a ottenere la risorsa dai query parameters
            if (exchange.getRequestOptions().getUriQuery().size() > 0) {
                String query = exchange.getRequestOptions().getUriQuery().get(0);
                if (query != null && query.startsWith("resource=")) {
                    resourceName = query.substring("resource=".length());
                }
            }
            
            if (resourceName == null || resourceName.trim().isEmpty()) {
                System.err.println("[ServiceResource] Missing resource parameter");
                exchange.respond(ResponseCode.BAD_REQUEST, 
                    "Missing resource parameter. Use: GET /service?resource=<resource_name>\n" +
                    "Example: GET /service?resource=temp");
                return;
            }
            
            System.out.println("[ServiceResource] Looking for resource: " + resourceName);
            
            // Cerca la risorsa nel database
            String resourceIP = Database.getResourceIP(resourceName);
            
            if (resourceIP != null) {
                // Costruisci l'URI completo della risorsa
                String fullResourceURI = "coap://[" + resourceIP + "]:5683/";
                
                System.out.println("[ServiceResource] Found resource: " + fullResourceURI);
                exchange.respond(ResponseCode.CONTENT, fullResourceURI, MediaTypeRegistry.TEXT_PLAIN);
                
            } else {
                System.out.println("[ServiceResource] Resource not found: " + resourceName);
                exchange.respond(ResponseCode.NOT_FOUND, "Resource not found: " + resourceName);
            }
            
        } catch (Exception e) {
            System.err.println("[ServiceResource] Error processing request: " + e.getMessage());
            e.printStackTrace();
            exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Server error");
        }
        
        System.out.println("[ServiceResource] === GET REQUEST END ===");
    }

}
