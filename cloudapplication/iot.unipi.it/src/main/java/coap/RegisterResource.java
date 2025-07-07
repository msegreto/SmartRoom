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
        String nodeIP = exchange.getSourceAddress().getHostAddress();
        String payload = exchange.getRequestText();
        
        System.out.println("[RegisterResource] Registration request from " + nodeIP);
        
        DeviceRegistration reg = parseRegistrationJSON(payload);
        if (reg != null) {
            try {
                // Save the registration in the database
                if(!Database.saveDeviceRegistration(nodeIP, reg.deviceId, reg.services)){
                    exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Failed to save registration");
                    System.err.println("[RegisterResource] Database save failed for device " + reg.deviceId);
                    return;
                }
            } catch (Exception e) {
                exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Database error: " + e.getMessage());
                System.err.println("[RegisterResource] Database exception for device " + reg.deviceId + ": " + e.getMessage());
                return;
            }
            
            exchange.respond(ResponseCode.CREATED, "Registration successful");
            System.out.println("[RegisterResource] Device " + reg.deviceId + " registered successfully");
            
            for (String resource : reg.services) {
                final Observer observerClient = new Observer(nodeIP, resource);
                Thread observertThread = new Thread(observerClient);
                observertThread.start();
                System.out.println("[RegisterResource] Observer started for " + resource);
            }
            
        } else {
            exchange.respond(ResponseCode.BAD_REQUEST, "Invalid JSON payload");
            System.err.println("[RegisterResource] Invalid JSON from " + nodeIP);
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
