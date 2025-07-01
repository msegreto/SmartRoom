// config.h
#ifndef CONFIG_H
#define CONFIG_H

#define CLOUD_SERVER_EP "coap://[fe80::202:2:2:2]:5683"
#define REGISTRATION_RESOURCE_PATH "registration"
#define REGISTRATION_ACK_CODE CONTENT_2_05
#define MAX_REGISTRATION_RETRY 3
#define REGISTRATION_WAIT_SECONDS 5
#define OBS_TEMP_URI "coap://[fd00::100]:5683/predictionTemp"
#define OBS_HUM_URI  "coap://[fd00::101]:5683/predictionHum"

#define DEFAULT_THRESHOLD_MIN 20.0
#define DEFAULT_THRESHOLD_MAX 28.0

#endif // CONFIG_H