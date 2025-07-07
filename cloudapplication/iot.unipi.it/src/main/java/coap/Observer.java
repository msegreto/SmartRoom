package coap;

import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapHandler;
import org.eclipse.californium.core.CoapObserveRelation;
import org.eclipse.californium.core.CoapResponse;

import db.LoggerSaver;

public class Observer implements Runnable{
    private final LoggerSaver logger;
    private final String resourceURI;

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
                String responseText = response.getResponseText();
                if (responseText == null || responseText.trim().isEmpty()) {
                    return;
                }

                String payload = responseText.trim();
                
                try {
                    logger.saveLog(payload);
                    System.out.println("[Observer] " + resourceURI + " → " + payload);
                } catch (Exception e) {
                    System.err.println("[Observer] DB error for " + resourceURI + ": " + e.getMessage());
                }
            }
            @Override
            public void onError() {
                System.err.println("[Observer] Connection lost: " + resourceURI);
            }
        });
        
        if (relation != null && !relation.isCanceled()) {
            System.out.println("[Observer] Observing: " + resourceURI);
        } else {
            System.err.println("[Observer] Failed to observe: " + resourceURI);
        }
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
