#ifndef CONFIG_H
#define CONFIG_H

#define CLOUD_SERVER_EP "coap://[fd00::1]:5683"
#define REGISTRATION_RESOURCE_PATH "registration"
#define REGISTRATION_ACK_CODE CREATED_2_01
#define MAX_REGISTRATION_RETRY 300
#define REGISTRATION_WAIT_SECONDS 5

// Path e query separati per la discovery
#define SERVICE_DISCOVERY_PATH "service"
#define QUERY_TEMP "resource=predt"
#define QUERY_HUM  "resource=predh"

#define DEFAULT_THRESHOLD_MIN 20.0
#define DEFAULT_THRESHOLD_MAX 28.0

#endif // CONFIG_H