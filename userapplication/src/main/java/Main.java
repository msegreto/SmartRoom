import java.util.*;
import java.util.concurrent.*;
import java.util.Locale;
import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapResponse;

import Ipv6Service;
import DBSupport;


public class UserApp {

    // Lista dei servizi richiesti per considerare il sistema pronto
    private static final List<String> REQUIRED_SERVICES = List.of(
        "led", "onlightactuator", "offlightact", "onh", "offh",
        "ont", "offt", "set_lim", "get_lim", "sts", "onhac", "offhac",
        "onlightsensor", "offlightsens"
    );

    private static final int DISCOVERY_INTERVAL_MS = 3000;

    private static final Map<String, String> ipv6Addresses = new HashMap<>();

    private static void initializeIpv6Addresses() {
        List<Ipv6Service> services = DBSupport.getIpv6Services();
        for (Ipv6Service service : services) {
            ipv6Addresses.put(service.resource, "[" + service.nodeip + "]");
        }
    }

    public static void sendCoapGetRequest(String resource) {
        String nodeIp = ipv6Addresses.get(resource);
        if (nodeIp == null) {
            System.out.println("IP non trovato per il servizio: " +resource);
            return;
        }

        String uri = "coap://" + nodeIp + "/service?resource=" + resource;
        CoapClient client = new CoapClient(uri);
        CoapResponse response = client.get();

        if (response != null) {
            System.out.println("Risposta da " + resource + ": " + response.getResponseText());
        } else {
            System.out.println("Errore nella richiesta CoAP a " + resource);
        }
    }

    public static void sendCoapPostRequest(String resource, String payload) {
        String nodeIp = ipv6Addresses.get(resource);
        if (nodeIp == null) {
            System.out.println("IP non trovato per il servizio: " + resource);
            return;
        }

        String uri = "coap://" + nodeIp + "/service?resource=" + resource;
        CoapClient client = new CoapClient(uri);
        CoapResponse response = client.post(payload, 0); // 0 = text/plain (media type)

        if (response != null) {
            System.out.println("Risposta da " + resource + ": " + response.getResponseText());
        } else {
            System.out.println("Errore nella richiesta CoAP POST a " + resource);
        }
    }

    public static void main(String[] args) {
        System.out.println("UserApp avviata. Inizio discovery...");
        DBSupport.connectToDatabase();

        ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor();
        scheduler.scheduleAtFixedRate(() -> {
            List<String> availableServices = DBSupport.getAvailableServices();
            if (availableServices.containsAll(REQUIRED_SERVICES)) {
                System.out.println("[Setup phase] Tutti i servizi sono disponibili. Puoi procedere.");
                scheduler.shutdown();
                showMenu();
            } else {
                System.out.println("[Setup phase] Servizi non ancora completi. In attesa...");
            }
        }, 0, DISCOVERY_INTERVAL_MS, TimeUnit.MILLISECONDS);
    }

    private static void showMenu() {
        Scanner scanner = new Scanner(System.in);
        while (true) {
            System.out.println("\n== MENU ==");
            System.out.println("1. Mostra lista completa servizi attivi");
            System.out.println("2. Stato ON/OFF sensore");
            System.out.println("3. Visualizza o setta threshold HAC");
            System.out.println("4. Stato HAC");
            System.out.println("5. Stato LED");
            System.out.println("0. Esci");
            System.out.print("Seleziona opzione: ");

            String input = scanner.nextLine();
            switch (input) {
                case "1":
                    handleServiceList();
                    break;
                case "2":
                    handleSensorToggle();
                    break;
                case "3":
                    handleThreshold();
                    break;
                case "4":
                    handleHacStatus();
                    break;
                case "5":
                    handleLedStatus();
                    break;
                case "0":
                    System.out.println("Chiusura applicazione...");
                    System.exit(0);
                default:
                    System.out.println("Opzione non valida.");
            }
        }
    }

    private static void handleServiceList() {
        List<Ipv6Service> services = DBSupport.getIpv6Services();
        System.out.println("Servizi IPv6 attivi:");
        services.forEach(s -> System.out.println(" - " + s));
    }

    private static void handleSensorToggle() {
        // Placeholder: lista statica per ora
        Map<String, String> sensors = Map.of(
            "1", "onlightsensor",
            "2", "offlightsens",
            "3", "onlightactuator", 
            "4","offlightact", 
            "5","onh", 
            "6","offh",
            "7","ont",
            "8", "offt",
            "9","onhac", 
            "10","offhac"
        );
        System.out.println("lightsensor stands for LightSwitchSensor,\nlightactuator for LightSwitchActuator,\nh for Humidity, \nt for Thermometer,  \nhac for HACSystem");
        System.out.println("Seleziona un sensore da attivare/disattivare:");
        sensors.forEach((k, v) -> System.out.println(k + ". " + v));
        System.out.print("Scelta: ");
        Scanner scanner = new Scanner(System.in);
        String choice = scanner.nextLine();

        String sensor = sensors.get(choice);
        if (sensor != null) {
            System.out.println("Toggle stato sensore: " + sensor);
            sendCoapGetRequest(sensor);
        } else {
            System.out.println("Sensore non valido.");
        }
    }

    private static void handleThreshold() {
        Scanner scanner = new Scanner(System.in);

        System.out.println("Cosa vuoi fare?");
        System.out.println("1. Visualizza soglie HAC");
        System.out.println("2. Modifica soglie HAC");
        System.out.print("Scelta: ");
        String input = scanner.nextLine();

        if (input.equals("1")) {
            // Comando per leggere le soglie attuali
            System.out.println("Eseguo comando: get_lim");
            sendCoapGetRequest("get_lim");

        } else if (input.equals("2")) {
            System.out.println("Recupero soglie attuali...");
            sendCoapGetRequest("get_lim");

            // Input nuovi valori
            System.out.print("Inserisci nuova soglia minima: ");
            String nuovaMin = scanner.nextLine().replace(",", "."); // accetta anche la virgola per input utente

            System.out.print("Inserisci nuova soglia massima: ");
            String nuovaMax = scanner.nextLine().replace(",", ".");

            try {
                float min = Float.parseFloat(nuovaMin);
                float max = Float.parseFloat(nuovaMax);

                // Costruzione del payload nel formato richiesto: "min,max" con virgola
                String payload = String.format(Locale.US, "%.2f,%.2f", min, max);

                System.out.println("Eseguo comando: set_lim con payload:");
                System.out.println(payload);

                // Invio POST a set_lim
                sendCoapPostRequest("set_lim", payload);

            } catch (NumberFormatException e) {
                System.out.println("Valori inseriti non validi.");
            }

        } else {
            System.out.println("Scelta non valida.");
        }
    }

    private static void handleHacStatus() {
        sendCoapGetRequest("sts");
    }

    private static void handleLedStatus() {
        sendCoapGetRequest("led");
    }
}
