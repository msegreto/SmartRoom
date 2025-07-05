#ifndef CONFIG_H
#define CONFIG_H

// CoAP Cloud server endpoint
#define CLOUD_SERVER_EP "coap://[fd00::1]:5683"

// CoAP registration resource path
#define REGISTRATION_RESOURCE_PATH "registration"

// Registration settings
#define REGISTRATION_ACK_CODE CREATED_2_01
#define REGISTRATION_WAIT_SECONDS 7

// Sample interval for sensors/actuators
#define MAX_REGISTRATION_RETRY 5
#define SENSOR_SAMPLE_INTERVAL 10


#endif // CONFIG_H
