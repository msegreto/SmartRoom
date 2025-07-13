package coap;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapHandler;
import org.eclipse.californium.core.CoapObserveRelation;
import org.eclipse.californium.core.CoapResponse;

import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;

import db.LoggerSaver;

public class Observer implements Runnable{
    private final LoggerSaver logger;
    private final String resourceURI;
    private volatile boolean running = true;

    private CoapClient client;
    private CoapObserveRelation relation;

    public Observer(String nodeIP, String resource) {
        this.resourceURI = "coap://[" + nodeIP + "]:5683/" + resource;
        this.logger = new LoggerSaver(resource);
        this.client = new CoapClient(resourceURI);
    }

    public void startObserving() {
        relation = client.observe(new CoapHandler() {
            @Override
            public void onLoad(CoapResponse response) {

                if (!running) return;

                String responseText = response.getResponseText();
                if (responseText == null || responseText.trim().isEmpty()) {
                    return;
                }

                String payload = responseText.trim();
                
                try {
                    JsonObject jsonObject = JsonParser.parseString(payload).getAsJsonObject();
                    String value = jsonObject.get("value").getAsString();
                    
                    logger.saveLog(value);
                    System.out.println("[Observer] " + resourceURI + " → value: " + value);
                    
                } catch (JsonSyntaxException e) {
                    System.err.println("[Observer] Invalid JSON for " + resourceURI + ": " + payload);
                } catch (Exception e) {
                    System.err.println("[Observer] Error processing " + resourceURI + ": " + e.getMessage());
                }
            }
            @Override
            public void onError() {
                System.err.println("[Observer] Connection lost: " + resourceURI);

                running = false;
            }
        });
        
        if (relation != null && !relation.isCanceled()) {
            System.out.println("[Observer] Observing: " + resourceURI);
        } else {
            System.err.println("[Observer] Failed to observe: " + resourceURI);
            running = false;
        }
    }

    public void stopObserving() {
        running = false;
        if (relation != null) {
            relation.proactiveCancel();
        }
        if (client != null) {
            client.shutdown();
        }
    }
    
    @Override
    public void run() {
        try {
            startObserving();

            // Keep thread alive while observing
            while (running && !Thread.currentThread().isInterrupted()) {
                try {
                    Thread.sleep(1000);
                } catch (InterruptedException e) {
                    System.out.println("[Observer] Thread interrupted: " + resourceURI);
                    Thread.currentThread().interrupt();
                    break;
                }
            }
            
        } catch (Exception e) {
            System.err.println("[Observer] Error in observer " + resourceURI + ": " + e.getMessage());
        } finally {
            stopObserving();
            System.out.println("[Observer] Observer thread terminated: " + resourceURI);
        }
    }
}
