CONTIKI_PROJECT = main
PROJECT_SOURCEFILES += main.c res-light.c

CFLAGS += -I$(CONTIKI_NG)/os/net/app-layer/coap

CONTIKI = /home/iot_ubuntu_intel/contiki-ng

MODULES_REL += ../cJSON-master
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

include $(CONTIKI)/Makefile.include
