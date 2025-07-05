#ifndef CONFIG_H
#define CONFIG_H

#define CLOUD_SERVER_EP "coap://[fe80::202:2:2:2]:5683"
#define REGISTRATION_RESOURCE_PATH "registration"
#define REGISTRATION_ACK_CODE CREATED_2_01
#define MAX_REGISTRATION_RETRY 3
#define REGISTRATION_WAIT_SECONDS 5

// Path e query separati per la discovery
#define SERVICE_DISCOVERY_PATH "service"
#define QUERY_LIGHT "resource=light"

#endif // CONFIG_H