package coap;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.coap.MediaTypeRegistry;
import org.eclipse.californium.core.server.resources.CoapExchange;

import db.Database;

public class ServiceResource extends CoapResource {

    private RegisterResource registerResource;

    public ServiceResource(String name) {
        super(name);
        setObservable(true);
    }
    
    /**
     * Imposta il riferimento alla RegisterResource per accedere al database
     */
    public void setRegisterResource(RegisterResource registerResource) {
        this.registerResource = registerResource;
    }

    @Override
    public void handleGET(CoapExchange exchange) {
        System.out.println("[ServiceResource] ======== GET HANDLER INVOKED ========");
        
        try {
            // Estrai il parametro query o path per il nome del servizio
            String serviceName = null;
            String queryString = exchange.getRequestOptions().getUriQueryString();
            
            System.out.println("[ServiceResource] Query string: " + queryString);
            
            if (queryString != null && !queryString.isEmpty()) {
                // Cerca parametro "name=service_name"
                String[] params = queryString.split("&");
                for (String param : params) {
                    if (param.startsWith("name=")) {
                        serviceName = param.substring(5); // Rimuovi "name="
                        break;
                    }
                }
            }
            
            if (serviceName == null || serviceName.trim().isEmpty()) {
                System.out.println("[ServiceResource] No service name provided, returning all services");
                handleGetAllServices(exchange);
                return;
            }
            
            System.out.println("[ServiceResource] Looking for service: " + serviceName);
            
            // Cerca il servizio nel database
            String serviceURI = findServiceURI(serviceName);
            
            if (serviceURI != null) {
                System.out.println("[ServiceResource] Found service URI: " + serviceURI);
                exchange.respond(serviceURI);
            } else {
                System.out.println("[ServiceResource] Service not found: " + serviceName);
                exchange.respond(ResponseCode.NOT_FOUND, "Service '" + serviceName + "' not found", MediaTypeRegistry.TEXT_PLAIN);
            }
            
        } catch (Exception e) {
            System.err.println("[ServiceResource] Error in handleGET: " + e.getMessage());
            exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Error processing request");
        }
        
        System.out.println("[ServiceResource] === GET HANDLER END ===");
    }
    
    /**
     * Gestisce la richiesta per ottenere tutti i servizi registrati
     */
    private void handleGetAllServices(CoapExchange exchange) {
        System.out.println("[ServiceResource] Returning all registered services");
        
        if (registerResource == null) {
            exchange.respond(ResponseCode.SERVICE_UNAVAILABLE, "Registration service not available");
            return;
        }
        
        try {
            // Accedi al database tramite reflection o metodo pubblico
            StringBuilder allServices = new StringBuilder();
            allServices.append("=== REGISTERED SERVICES ===\n");
            
            Database database = getRegistrationDatabase();
            if (database != null) {
                for (String ip : database.getAllKeys()) {
                    String deviceInfo = database.get(ip);
                    ServiceInfo info = parseDeviceInfo(deviceInfo);
                    
                    if (info != null) {
                        allServices.append("Service: ").append(info.deviceId).append("\n");
                        allServices.append("  Endpoint: ").append(info.endpoint).append("\n");
                        allServices.append("  Services: ");
                        for (int i = 0; i < info.services.length; i++) {
                            allServices.append(info.services[i]);
                            if (i < info.services.length - 1) {
                                allServices.append(", ");
                            }
                        }
                        allServices.append("\n");
                        allServices.append("  Interval: ").append(info.interval).append("s\n\n");
                    }
                }
            }
            
            if (allServices.length() == 0) {
                allServices.append("No services registered\n");
            }
            
            exchange.respond(ResponseCode.CONTENT, allServices.toString(), MediaTypeRegistry.TEXT_PLAIN);
            
        } catch (Exception e) {
            System.err.println("[ServiceResource] Error getting all services: " + e.getMessage());
            exchange.respond(ResponseCode.INTERNAL_SERVER_ERROR, "Error retrieving services");
        }
    }
    
    /**
     * Cerca l'URI di un servizio specifico nel database
     */
    private String findServiceURI(String serviceName) {
        if (registerResource == null) {
            System.err.println("[ServiceResource] RegisterResource not set");
            return null;
        }
        
        try {
            Database database = getRegistrationDatabase();
            if (database == null) {
                return null;
            }
            
            // Cerca in tutti i dispositivi registrati
            for (String ip : database.getAllKeys()) {
                String deviceInfo = database.get(ip);
                ServiceInfo info = parseDeviceInfo(deviceInfo);
                
                if (info != null) {
                    // Controlla se il nome del dispositivo corrisponde
                    if (info.deviceId.equals(serviceName)) {
                        return info.endpoint;
                    }
                    
                    // Controlla se è uno dei servizi offerti
                    for (String service : info.services) {
                        if (service.equals(serviceName)) {
                            return info.endpoint + "/" + service;
                        }
                    }
                }
            }
            
            return null;
            
        } catch (Exception e) {
            System.err.println("[ServiceResource] Error finding service URI: " + e.getMessage());
            return null;
        }
    }
    
    /**
     * Ottiene il database dalla RegisterResource usando reflection
     */
    private Database getRegistrationDatabase() {
        try {
            java.lang.reflect.Field databaseField = registerResource.getClass().getDeclaredField("database");
            databaseField.setAccessible(true);
            return (Database) databaseField.get(registerResource);
        } catch (Exception e) {
            System.err.println("[ServiceResource] Error accessing database: " + e.getMessage());
            return null;
        }
    }
    
    /**
     * Parse delle informazioni del dispositivo dal formato stringa
     */
    private ServiceInfo parseDeviceInfo(String deviceInfo) {
        try {
            String[] parts = deviceInfo.split(";");
            String deviceId = null;
            String[] services = null;
            int interval = 0;
            String endpoint = null;
            
            for (String part : parts) {
                if (part.startsWith("id:")) {
                    deviceId = part.substring(3);
                } else if (part.startsWith("services:")) {
                    String servicesStr = part.substring(9);
                    services = servicesStr.split(",");
                } else if (part.startsWith("interval:")) {
                    interval = Integer.parseInt(part.substring(9));
                } else if (part.startsWith("endpoint:")) {
                    endpoint = part.substring(9);
                }
            }
            
            if (deviceId != null && services != null && endpoint != null) {
                return new ServiceInfo(deviceId, services, interval, endpoint);
            }
            
            return null;
            
        } catch (Exception e) {
            System.err.println("[ServiceResource] Error parsing device info: " + e.getMessage());
            return null;
        }
    }

    @Override
    public void handlePOST(CoapExchange exchange) {
        System.out.println("[ServiceResource] POST request received");
        exchange.respond(ResponseCode.METHOD_NOT_ALLOWED, "POST not supported. Use GET with ?name=service_name");
    }
    
    /**
     * Classe per rappresentare le informazioni di un servizio
     */
    private static class ServiceInfo {
        String deviceId;
        String[] services;
        int interval;
        String endpoint;
        
        ServiceInfo(String deviceId, String[] services, int interval, String endpoint) {
            this.deviceId = deviceId;
            this.services = services;
            this.interval = interval;
            this.endpoint = endpoint;
        }
    }
}
