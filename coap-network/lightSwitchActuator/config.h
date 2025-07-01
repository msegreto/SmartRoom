#ifndef CONFIG_H
#define CONFIG_H

// CoAP Cloud server endpoint
#define CLOUD_SERVER_EP "coap://[fe80::202:2:2:2]:5683"

// CoAP registration resource path
#define REGISTRATION_RESOURCE_PATH "registration"

// Registration settings
#define MAX_REGISTRATION_RETRY 3
#define REGISTRATION_WAIT_SECONDS 5
#define REGISTRATION_ACK_CODE 65 // 2.01 Created

// Sample interval for sensors/actuators
#define SENSOR_SAMPLE_INTERVAL 10

#endif // CONFIG_H