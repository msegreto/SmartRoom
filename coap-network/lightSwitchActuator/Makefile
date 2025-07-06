USER_NAME := $(shell whoami)

CONTIKI_PROJECT = led-actuator
PROJECT_SOURCEFILES += res-led.c res-control.c

all: $(CONTIKI_PROJECT)

CFLAGS += -I$(CONTIKI_NG)/os/net/app-layer/coap

CONTIKI = /home/$(USER_NAME)/contiki-ng

MODULES_REL += ../cJSON-master
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

include $(CONTIKI)/Makefile.include