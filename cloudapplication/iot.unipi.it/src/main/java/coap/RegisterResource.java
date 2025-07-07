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
        
        DeviceRegistration reg = parseRegistrationJSON(payload);
        if (reg != null) {
            try {
                if(!Database.saveDeviceRegistration(nodeIP, reg.deviceId, reg.services)){
                    exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Failed to save registration");
                    System.err.println("[Registration] DB save failed: " + reg.deviceId);
                    return;
                }
            } catch (Exception e) {
                exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Database error: " + e.getMessage());
                System.err.println("[Registration] DB error for " + reg.deviceId + ": " + e.getMessage());
                return;
            }

            exchange.respond(ResponseCode.CREATED, "Registration successful");
            
            for (String resource : reg.services) {
                final Observer observerClient = new Observer(nodeIP, resource);
                Thread observerThread = new Thread(observerClient);
                observerThread.start();
            }
        
        } else {
            exchange.respond(ResponseCode.BAD_REQUEST, "Invalid JSON payload");
            System.err.println("[Registration] Invalid JSON from " + nodeIP);
        }
    }

    private DeviceRegistration parseRegistrationJSON(String json) {
        try {
            json = json.trim();
            
            String deviceId = extractStringField(json, "\"s\"");
            if (deviceId == null) return null;
            
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
            
            while (startIndex < json.length() && json.charAt(startIndex) != '"') {
                startIndex++;
            }
            startIndex++;
            
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
            
            while (startIndex < json.length() && json.charAt(startIndex) != '[') {
                startIndex++;
            }
            startIndex++;
            
            int endIndex = startIndex;
            int bracketCount = 1;
            while (endIndex < json.length() && bracketCount > 0) {
                if (json.charAt(endIndex) == '[') bracketCount++;
                if (json.charAt(endIndex) == ']') bracketCount--;
                endIndex++;
            }
            endIndex--;
            
            String arrayContent = json.substring(startIndex, endIndex);
            
            String[] elements = arrayContent.split(",");
            for (int i = 0; i < elements.length; i++) {
                elements[i] = elements[i].trim().replaceAll("\"", "");
            }
            
            return elements;
        } catch (Exception e) {
            return null;
        }
    }

    private static class DeviceRegistration {
        String deviceId;
        String[] services;
        
        DeviceRegistration(String deviceId, String[] services) {
            this.deviceId = deviceId;
            this.services = services;
        }
    }
}
