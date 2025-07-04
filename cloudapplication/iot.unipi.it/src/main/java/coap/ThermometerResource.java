package coap;

import org.eclipse.californium.core.CoapResource;
import org.eclipse.californium.core.coap.CoAP.ResponseCode;
import org.eclipse.californium.core.server.resources.CoapExchange;

public class ThermometerResource extends CoapResource {

    public ThermometerResource(String name) {
        super(name);
        setObservable(true);
    }

    @Override
    public void handleGET(CoapExchange exchange) {
        exchange.respond("Ciao, sono un termometro, che caldo!");
    }

    @Override
    public void handlePOST(CoapExchange exchange) {
        // Handle POST requests if needed
        exchange.respond(ResponseCode.CREATED);
    }
}