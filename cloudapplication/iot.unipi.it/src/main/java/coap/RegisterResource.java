package coap;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.server.resources.CoapExchange;

import db.Database;

public class RegisterResource extends CoapResource {

    public RegisterResource(String name) {
        super(name);
        setObservable(true);
    }

    @Override
    public void handlePOST(CoapExchange exchange) {
        System.out.println("[RegisterResource] === INCOMING POST REQUEST ===");
        
        String nodeIP = exchange.getSourceAddress().getHostAddress();
        System.out.println("[RegisterResource] Node IP: " + nodeIP);
        
        String payload = exchange.getRequestText();
        System.out.println("[RegisterResource] Payload: " + payload);
        
        DeviceRegistration reg = parseRegistrationJSON(payload);
        if (reg != null) {
            if(!Database.saveDeviceRegistration(nodeIP, reg.deviceId, reg.services)){
                exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Failed to save registration");
                System.err.println("[RegisterResource] Failed to save registration for device " + reg.deviceId);
                return;
            };
            
            exchange.respond(ResponseCode.CREATED, "Registration successful");
            System.out.println("[RegisterResource] Device " + reg.deviceId + 
                            " registered at " + nodeIP + " with " + reg.services.length + " resources");
            
            // Stampa le risorse registrate
            for (String resource : reg.services) {
                System.out.println("[RegisterResource]   Resource: coap://[" + nodeIP + "]:5683/" + resource);
            }
        } else {
            exchange.respond(ResponseCode.BAD_REQUEST, "Invalid JSON payload");
            System.err.println("[RegisterResource] Failed to parse registration JSON");
        }
    }
    
    /**
     * Parse manuale del JSON di registrazione (senza librerie esterne)
     */
    private DeviceRegistration parseRegistrationJSON(String json) {
        try {
            // Rimuovi spazi e caratteri di controllo
            json = json.trim();
            
            // Estrai il campo "s" (device ID)
            String deviceId = extractStringField(json, "\"s\"");
            if (deviceId == null) return null;
            
            // Estrai il campo "ss" (services array)
            String[] services = extractArrayField(json, "\"ss\"");
            if (services == null) return null;
            
            return new DeviceRegistration(deviceId, services);
            
        } catch (Exception e) {
            System.err.println("[RegisterResource] JSON parsing error: " + e.getMessage());
            return null;
        }
    }
    
    private String extractStringField(String json, String fieldName) {
        try {
            int startIndex = json.indexOf(fieldName + ":") + fieldName.length() + 1;
            // Salta spazi e cerca la virgoletta di apertura
            while (startIndex < json.length() && json.charAt(startIndex) != '"') {
                startIndex++;
            }
            startIndex++; // Salta la virgoletta di apertura
            
            int endIndex = startIndex;
            while (endIndex < json.length() && json.charAt(endIndex) != '"') {
                endIndex++;
            }
            
            return json.substring(startIndex, endIndex);
        } catch (Exception e) {
            return null;
        }
    }
    
    private String[] extractArrayField(String json, String fieldName) {
        try {
            int startIndex = json.indexOf(fieldName + ":") + fieldName.length() + 1;
            // Trova la parentesi quadra di apertura
            while (startIndex < json.length() && json.charAt(startIndex) != '[') {
                startIndex++;
            }
            startIndex++; // Salta '['
            
            int endIndex = startIndex;
            int bracketCount = 1;
            while (endIndex < json.length() && bracketCount > 0) {
                if (json.charAt(endIndex) == '[') bracketCount++;
                if (json.charAt(endIndex) == ']') bracketCount--;
                endIndex++;
            }
            endIndex--; // Torna indietro alla ']'
            
            String arrayContent = json.substring(startIndex, endIndex);
            
            // Parse degli elementi dell'array
            String[] elements = arrayContent.split(",");
            for (int i = 0; i < elements.length; i++) {
                elements[i] = elements[i].trim().replaceAll("\"", "");
            }
            
            return elements;
        } catch (Exception e) {
            return null;
        }
    }

    /**
     * Classe interna per rappresentare una registrazione
     */
    private static class DeviceRegistration {
        String deviceId;
        String[] services;
        
        DeviceRegistration(String deviceId, String[] services) {
            this.deviceId = deviceId;
            this.services = services;
        }
    }
}
