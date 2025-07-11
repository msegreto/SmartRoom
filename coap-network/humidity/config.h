#ifndef CONFIG_H
#define CONFIG_H

#define SENSING_PERIOD_SECONDS 60*30 // 30 minutes
#define MEAN_HUMIDITY 41.1
#define STD_HUMIDITY 5.83
#define MAX_STEP 2.00
#define BUFFER_SIZE 24 // the model use the 12h before for next 30 minutes forecasting 

#define CLOUD_SERVER_EP "coap://[fd00::1]:5683"
#define REGISTRATION_RESOURCE_PATH "registration"
#define REGISTRATION_ACK_CODE CREATED_2_01
#define MAX_REGISTRATION_RETRY 300
#define REGISTRATION_WAIT_SECONDS 6

#endif // CONFIG_H