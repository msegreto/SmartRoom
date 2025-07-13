import java.util.*;
import java.util.concurrent.*;
import java.util.Locale;
import org.eclipse.californium.core.CoapClient;
import org.eclipse.californium.core.CoapResponse;
import org.eclipse.californium.core.coap.MediaTypeRegistry;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.google.gson.JsonSyntaxException;

public class Main {

    // List of required services for the system to be considered ready
    private static final List<String> REQUIRED_SERVICES = List.of(
            "led", "onlightact", "offlightact", "onh", "offh",
            "ont", "offt", "set_lim", "get_lim", "sts", "onhac", "offhac",
            "onlightsens", "offlightsens"
    );

    private static final int DISCOVERY_INTERVAL_MS = 3000;

    private static final Map<String, String> ipv6Addresses = new HashMap<>();

    private static void initializeIpv6Addresses() {
        List<Ipv6Service> services = DBSupport.getIpv6Services();
        for (Ipv6Service service : services) {
            ipv6Addresses.put(service.resource, "[" + service.nodeip + "]");
        }
    }

    public static String sendCoapGetRequest(String resource) {
        String nodeIp = ipv6Addresses.get(resource);
        if (nodeIp == null) {
            System.out.println("[DEBUG] IP not found for service: " + resource);
            return "null payload";
        }

        String uri =  "coap://" + nodeIp + ":5683/" + resource;
        System.out.println("[DEBUG] Built URI: " + uri);

        CoapClient client = new CoapClient(uri);
        CoapResponse response = client.get();

        if (response != null) {
            int code = response.getCode().value;
            String codeName = response.getCode().name();
            String payload = response.getResponseText();

            System.out.println("[DEBUG] Response code: " + code + " (" + codeName + ")");
            System.out.println("[DEBUG] Response payload (length " + payload.length() + "): \"" + payload + "\"");

            if (!payload.isEmpty()) {
                try {
                    JsonObject jsonObject = JsonParser.parseString(payload).getAsJsonObject();
                    String value = jsonObject.get("value").getAsString();
                    System.out.println("Response from " + resource + ": " + value);
                    return value;
                } catch (JsonSyntaxException e) {
                    System.out.println("Response from " + resource + ": " + payload);
                    return payload;
                }
            } else {
                System.out.println("Empty response from " + resource + ", but code: " + codeName);
                return "null payload";
            }
        } else {
            System.out.println("Error: null CoAP response from " + resource);
            return "null payload";
        }
    }

    public static void sendCoapPostRequest(String resource, String payload) {
        String nodeIp = ipv6Addresses.get(resource);
        if (nodeIp == null) {
            System.out.println("[DEBUG] IP not found for service: " + resource);
            return;
        }

        JsonObject jsonPayload = new JsonObject();
        jsonPayload.addProperty("value", payload);
        String jsonString = jsonPayload.toString();

        String uri = "coap://" + nodeIp + ":5683/" + resource;
        System.out.println("[DEBUG] Built URI: " + uri);
        System.out.println("[DEBUG] Payload to send (length " + jsonString.length() + "): \"" + jsonString + "\"");

        CoapClient client = new CoapClient(uri);

        CoapResponse response = client.post(jsonString, MediaTypeRegistry.APPLICATION_JSON);

        if (response != null) {
            int code = response.getCode().value;
            String codeName = response.getCode().name();
            String responsePayload = response.getResponseText();

            System.out.println("[DEBUG] Response code: " + code + " (" + codeName + ")");
            System.out.println("[DEBUG] Response payload (length " + responsePayload.length() + "): \"" + responsePayload + "\"");

            if (!responsePayload.isEmpty()) {
                try {
                    JsonObject jsonObject = JsonParser.parseString(responsePayload).getAsJsonObject();
                    String value = jsonObject.get("value").getAsString();
                    System.out.println("Response from " + resource + ": " + value);
                } catch (JsonSyntaxException e) {
                    System.out.println("Response from " + resource + ": " + responsePayload);
                }
            } else {
                System.out.println("Empty response from " + resource + ", but code: " + codeName);
            }
        } else {
            System.out.println("Error: null CoAP response from " + resource);
        }
    }

    public static void main(String[] args) {
        System.out.println("UserApp started. Beginning discovery...");
        DBSupport.connectToDatabase();

        ScheduledExecutorService scheduler = Executors.newSingleThreadScheduledExecutor();
        scheduler.scheduleAtFixedRate(() -> {
            List<String> availableServices = DBSupport.getAvailableServices();
            if (availableServices.containsAll(REQUIRED_SERVICES)) {
                System.out.println("[Setup phase] All services are available. You can proceed.");
                scheduler.shutdown();
                showMenu();
            } else {
                System.out.println("[Setup phase] Services not yet complete. Waiting...");
            }
        }, 0, DISCOVERY_INTERVAL_MS, TimeUnit.MILLISECONDS);
    }

    private static void showMenu() {
        initializeIpv6Addresses();
        Scanner scanner = new Scanner(System.in);
        while (true) {
            System.out.println("\n== MENU ==");
            System.out.println("1. Show full list of active services");
            System.out.println("2. Sensor ON/OFF status");
            System.out.println("3. View or set HAC thresholds");
            System.out.println("4. HAC status");
            System.out.println("5. LED status");
            System.out.println("0. Exit");
            System.out.print("Select option: ");

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
                    System.out.println("Closing application...");
                    System.exit(0);
                default:
                    System.out.println("Invalid option.");
            }
        }
    }

    private static void handleServiceList() {
        List<Ipv6Service> services = DBSupport.getIpv6Services();
        System.out.println("Active IPv6 services:");
        services.forEach(s -> System.out.println(" - " + s));
    }

    private static void handleSensorToggle() {
        Map<String, String> sensors = Map.of(
                "1", "onlightsens",
                "2", "offlightsens",
                "3", "onlightact",
                "4", "offlightact",
                "5", "onh",
                "6", "offh",
                "7", "ont",
                "8", "offt",
                "9", "onhac",
                "10", "offhac"
        );

        System.out.println("lightsens stands for LightSwitchSensor,\nlightact for LightSwitchActuator,\nh for Humidity, \nt for Thermometer,  \nhac for HACSystem");
        System.out.println("Select a sensor to activate/deactivate (number):");
        sensors.forEach((k, v) -> System.out.println(k + ". " + v));
        System.out.print("Choice: ");
        Scanner scanner = new Scanner(System.in);
        String choice = scanner.nextLine();

        String sensor = sensors.get(choice);
        String status;
        if (sensor != null) {
            switch (sensor) {

                case "ont":
                    System.out.println("Activating Thermometer with custom behavior...");
                    sendCoapPostRequest(sensor, "1");
                    status = sendCoapGetRequest("onhac");
                    if (status != null && status.contains("ON")) {
                        System.out.println("Resetting HacSystem...");
                        sendCoapPostRequest("offhac", "1");
                        System.out.println("Restarting HacSystem...");
                        sendCoapPostRequest("onhac", "1");
                    } else {
                        System.out.println("Attuatore non attivo, nessuna azione eseguita.");
                    }
                    break;

                case "onh":
                    System.out.println("Activating Humidity with custom behavior...");
                    sendCoapPostRequest(sensor, "1");
                    status = sendCoapGetRequest("onhac");
                    if (status != null && status.contains("ON")) {
                        System.out.println("Resetting HacSystem...");
                        sendCoapPostRequest("offhac", "1");
                        System.out.println("Restarting HacSystem...");
                        sendCoapPostRequest("onhac", "1");
                    } else {
                        System.out.println("Attuatore non attivo, nessuna azione eseguita.");
                    }
                    status = sendCoapGetRequest("ont");
                    if (status != null && status.contains("ON")) {
                        System.out.println("Resetting Thermometer...");
                        sendCoapPostRequest("offt", "1");
                        System.out.println("Restarting Thermometer...");
                        sendCoapPostRequest("ont", "1");
                    } else {
                        System.out.println("Attuatore non attivo, nessuna azione eseguita.");
                    }
                    break;

                case "onhac":
                    System.out.println("Activating HAC system with custom behavior...");
                    sendCoapPostRequest(sensor, "1");
                    status = sendCoapGetRequest("ont");
                    if (status != null && status.contains("ON")) {
                        System.out.println("Resetting Thermometer...");
                        sendCoapPostRequest("offt", "1");
                        System.out.println("Restarting Thermometer...");
                        sendCoapPostRequest("ont", "1");
                    } else {
                        System.out.println("Attuatore non attivo, nessuna azione eseguita.");
                    }
                    break;

                case "onlightsens":
                    System.out.println("Activating LightSwitchSensor with custom behavior...");
                    sendCoapPostRequest(sensor, "1");
                    status = sendCoapGetRequest("onlightsens");
                    if (status != null && status.contains("ON")) {
                        System.out.println("Resetting LightSwitchActuator...");
                        sendCoapPostRequest("offlightsens", "1");
                        System.out.println("Restarting LightSwitchActuator...");
                        sendCoapPostRequest("onlightsens", "1");
                    } else {
                        System.out.println("Attuatore non attivo, nessuna azione eseguita.");
                    }

                    break;

                default:
                    System.out.println("Toggling sensor state: " + sensor);
                    sendCoapPostRequest(sensor, "1");
                    break;
            }
        } else {
            System.out.println("Invalid sensor.");
        }
    }

    private static void handleThreshold() {
        Scanner scanner = new Scanner(System.in);

        System.out.println("What would you like to do?");
        System.out.println("1. View HAC thresholds");
        System.out.println("2. Modify HAC thresholds");
        System.out.print("Choice: ");
        String input = scanner.nextLine();

        if (input.equals("1")) {
            // Command to read current thresholds
            System.out.println("Executing command: get_lim");
            sendCoapGetRequest("get_lim");

        } else if (input.equals("2")) {
            System.out.println("Fetching current thresholds...");
            sendCoapGetRequest("get_lim");

            // Input new values
            System.out.print("Enter new minimum threshold: ");
            String nuovaMin = scanner.nextLine().replace(",", "."); // also accepts comma from user input

            System.out.print("Enter new maximum threshold: ");
            String nuovaMax = scanner.nextLine().replace(",", ".");

            try {
                float min = Float.parseFloat(nuovaMin);
                float max = Float.parseFloat(nuovaMax);

                String payload = String.format(Locale.ITALY, "%.1f,%.1f", min, max);

                System.out.println("Executing command: set_lim with payload:");
                System.out.println(payload);

                sendCoapPostRequest("set_lim", payload);

            } catch (NumberFormatException e) {
                System.out.println("Invalid input values.");
            }

        } else {
            System.out.println("Invalid choice.");
        }
    }

    private static void handleHacStatus() {
        sendCoapGetRequest("sts");
    }

    private static void handleLedStatus() {

        Scanner scanner = new Scanner(System.in);

        System.out.println("What would you like to do?");
        System.out.println("1. View led");
        System.out.println("2. Toggle led");
        System.out.print("Choice: ");
        String input = scanner.nextLine();

        if (input.equals("1")) {
            // Command to read current thresholds
            System.out.println("Executing command: lad(get)");
            sendCoapGetRequest("led");

        } else if (input.equals("2")) {
            System.out.println("Fetching current led status...");
            sendCoapGetRequest("led");

            // Input new values
            System.out.print("Enter new new status(0->off, 1->on): ");
            String newStatus = scanner.nextLine();

            // Validate input
            if (!newStatus.equals("0") && !newStatus.equals("1")) {
                System.out.println("Invalid input. Please enter 0 or 1.");
                return;
            }

            // Send the new status via POST
            sendCoapPostRequest("led", newStatus);

        } else {
            System.out.println("Invalid choice.");
        }

    }
}
