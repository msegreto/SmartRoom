package coap;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.coap.MediaTypeRegistry;
import org.eclipse.californium.core.server.resources.CoapExchange;

import db.Database;

public class RegisterResource extends CoapResource {

    private Database database = new Database();

    public RegisterResource(String name) {
        super(name);
        setObservable(true);
    }

    @Override
    public void handleGET(CoapExchange exchange) {
        System.out.println("[RegisterResource] ======== GET HANDLER INVOKED ========");
        
        try {
            // Estrai l'IP del client (source address)
            String clientIP = exchange.getSourceAddress().getHostAddress();
            System.out.println("[RegisterResource] Checking registration for IP: " + clientIP);
            
            // Controlla se l'IP è già registrato nel database
            if (database.containsKey(clientIP)) {
                String deviceInfo = database.get(clientIP);
                System.out.println("[RegisterResource] Device already registered: " + deviceInfo);
                
                String response = "registered_device:" + deviceInfo;
                exchange.respond(ResponseCode.CONTENT, response, MediaTypeRegistry.TEXT_PLAIN);
                
            } else {
                System.out.println("[RegisterResource] Device not registered");
                exchange.respond(ResponseCode.CONTENT, "not_registered", MediaTypeRegistry.TEXT_PLAIN);
            }
            
        } catch (Exception e) {
            System.err.println("[RegisterResource] Error in handleGET: " + e.getMessage());
            exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Error checking registration");
        }
        
        System.out.println("[RegisterResource] === GET HANDLER END ===");
    }

    @Override
    public void handlePOST(CoapExchange exchange) {
        System.out.println("[RegisterResource] ======== POST HANDLER INVOKED ========");
        System.out.println("[RegisterResource] Request received at " + System.currentTimeMillis());
        
        try {
            // Estrai il payload
            String payload = exchange.getRequestText();
            System.out.println("[RegisterResource] Payload length: " + payload.length());
            System.out.println("[RegisterResource] Received payload: '" + payload + "'");
            
            if (payload == null || payload.trim().isEmpty()) {
                System.err.println("[RegisterResource] Empty or NULL payload");
                exchange.respond(ResponseCode.BAD_REQUEST, "Empty payload");
                return;
            }
            
            // Estrai l'indirizzo IPv6 del client
            String clientIP = exchange.getSourceAddress().getHostAddress();
            int clientPort = exchange.getSourcePort();
            String ipv6Endpoint = "coap://[" + clientIP + "]:" + clientPort;
            
            System.out.println("[RegisterResource] Device IPv6 address: " + clientIP);
            System.out.println("[RegisterResource] Device endpoint: " + ipv6Endpoint);
            
            // Parse del JSON manuale (senza librerie esterne)
            DeviceRegistration registration = parseRegistrationJSON(payload);
            
            if (registration == null) {
                System.err.println("[RegisterResource] Failed to parse registration JSON");
                exchange.respond(ResponseCode.BAD_REQUEST, "Invalid JSON format");
                return;
            }
            
            System.out.println("[RegisterResource] Parsed device ID: " + registration.deviceId);
            System.out.println("[RegisterResource] Number of services: " + registration.services.length);
            
            // Controlla se il dispositivo è già registrato
            if (database.containsKey(clientIP)) {
                System.out.println("[RegisterResource] Device '" + registration.deviceId + "' already registered");
                exchange.respond(ResponseCode.CONTENT, "already_registered", MediaTypeRegistry.TEXT_PLAIN);
                return;
            }
            
            // Crea le informazioni del dispositivo da salvare
            String deviceInfo = createDeviceInfo(registration, ipv6Endpoint);
            
            // Salva nel database usando l'IP come chiave
            database.insert(clientIP, deviceInfo);
            
            System.out.println("[RegisterResource] Successfully registered device: " + registration.deviceId);
            System.out.println("[RegisterResource] Total devices: " + database.size());
            
            // Log di tutti i servizi registrati
            for (int i = 0; i < registration.services.length; i++) {
                System.out.println("[RegisterResource] Service " + i + ": " + registration.services[i]);
            }
            
            // Risposta di successo
            exchange.respond(ResponseCode.CREATED, "registration_successful", MediaTypeRegistry.TEXT_PLAIN);
            
            // Stampa tutto il database
            database.printAll();
            
            // Notifica agli osservatori
            changed();
            
        } catch (Exception e) {
            System.err.println("[RegisterResource] Error in handlePOST: " + e.getMessage());
            e.printStackTrace();
            exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Registration failed: " + e.getMessage());
        }
        
        System.out.println("[RegisterResource] === POST HANDLER END ===");
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
    
    private Integer extractNumberField(String json, String fieldName) {
        try {
            int startIndex = json.indexOf(fieldName + ":") + fieldName.length() + 1;
            // Salta spazi
            while (startIndex < json.length() && Character.isWhitespace(json.charAt(startIndex))) {
                startIndex++;
            }
            
            int endIndex = startIndex;
            while (endIndex < json.length() && 
                   (Character.isDigit(json.charAt(endIndex)) || json.charAt(endIndex) == '-')) {
                endIndex++;
            }
            
            return Integer.parseInt(json.substring(startIndex, endIndex));
        } catch (Exception e) {
            return null;
        }
    }
    
    private String createDeviceInfo(DeviceRegistration registration, String endpoint) {
        StringBuilder info = new StringBuilder();
        info.append("id:").append(registration.deviceId).append(";");
        info.append("services:");
        for (int i = 0; i < registration.services.length; i++) {
            info.append(registration.services[i]);
            if (i < registration.services.length - 1) {
                info.append(",");
            }
        }
        info.append(";");
        info.append("endpoint:").append(endpoint).append(";");
        info.append("timestamp:").append(System.currentTimeMillis());
        
        return info.toString();
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
    
    /**
     * Metodo di debug per stampare tutti i dispositivi registrati
     */
    public void printAllRegisteredDevices() {
        System.out.println("[RegisterResource] === DISPOSITIVI REGISTRATI ===");
        database.printAll();
    }
    
    /**
     * Ottieni il numero di dispositivi registrati
     */
    public int getRegisteredDevicesCount() {
        return database.size();
    }
}
