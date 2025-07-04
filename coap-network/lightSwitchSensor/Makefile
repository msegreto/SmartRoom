ARCH := $(shell uname -m)

ifeq ($(ARCH),x86_64)
    CONTIKI_USER := iot_ubuntu_intel
else ifeq ($(ARCH),aarch64)
    CONTIKI_USER := iot_ubuntu_arm
else
    $(error Architettura non supportata: $(ARCH))
endif

CONTIKI = /home/$(CONTIKI_USER)/contiki-ng

CONTIKI_PROJECT = main
PROJECT_SOURCEFILES += main.c res-light.c

CFLAGS += -I$(CONTIKI_NG)/os/net/app-layer/coap

MODULES_REL += ../cJSON-master
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

include $(CONTIKI)/Makefile.include