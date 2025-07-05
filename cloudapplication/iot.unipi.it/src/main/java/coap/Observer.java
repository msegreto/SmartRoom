package coap;

import java.util.logging.Logger;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapHandler;
import org.eclipse.californium.core.CoapObserveRelation;
import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.CoapResponse;

import db.LoggerSaver;

public class Observer implements Runnable{
    private final String nodeIP;
    private final String resource;
    private final LoggerSaver logger;
    private final String resourceURI;

    private CoapClient client;
    private CoapObserveRelation relation;

    public Observer(String nodeIP, String resource) {
        this.nodeIP = nodeIP;
        this.resource = resource;
        this.resourceURI = "coap://[" + nodeIP + "]:5683/" + resource;
        this.logger = new LoggerSaver(resource);
        this.client = new CoapClient(resourceURI);
    }

    public void startObserving() {
        System.out.println("[Observer] Starting observation for resource: " + resourceURI);
        relation = client.observe(new CoapHandler() {
            @Override
            public void onLoad(CoapResponse response) {
                String responseText = response.getResponseText();
                if (responseText == null || responseText.trim().isEmpty()) {
                    System.err.println("[Observer] Received empty or null response");
                    return;
                }
                Float payload = Float.parseFloat(responseText.trim());
                long timestamp = System.currentTimeMillis();
                
                // Log the received data
                try {
                    logger.saveLog(payload);
                } catch (Exception e) {
                    System.err.println("[Observer] Error saving log: " + e.getMessage());
                    e.printStackTrace();
                }
                
                System.out.println("[Observer] Received data: " + payload + " at " + timestamp);
            }
            @Override
            public void onError() {
                System.err.println("[Observer] Error observing resource: " + resourceURI);
            }
        });
    }

    public void stopObserving() {
        if (relation != null) {
            relation.proactiveCancel();
        }
        if (client != null) {
            client.shutdown();
        }
    }
    
    @Override
    public void run() {
        startObserving();
    }
}
