#ifndef CONFIG_H
#define CONFIG_H

#define SENSING_PERIOD_SECONDS 5
#define MEAN_TEMPERATURE 26.34
#define STD_TEMPERATURE 1.85
#define MAX_STEP 0.50
#define BUFFER_SIZE 60

#define CLOUD_SERVER_EP "coap://[fd00::1]:5683"
#define REGISTRATION_RESOURCE_PATH "registration"
#define REGISTRATION_ACK_CODE CREATED_2_01
#define MAX_REGISTRATION_RETRY 300
#define REGISTRATION_WAIT_SECONDS 5

// Path e query separati per la discovery
#define SERVICE_DISCOVERY_PATH "service"
#define QUERY_HAC "resource=sts"

#endif // CONFIG_H