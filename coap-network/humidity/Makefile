CONTIKI_PROJECT = smart-thermometer-flat
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += \
  main.c \
  sensor/prediction.c \
  sensor/sensing.c \
  sensor/buffer.c \
  coap/res_latest.c \
  coap/res_prediction.c \
  coap/res_control.c


CFLAGS += -I$(CONTIKI_NG)/os/net/app-layer/coap
CFLAGS += -I./sensor
CFLAGS += -I./coap


PROJECTDIRS += sensor coap

MODULES_REL+=/home/iot_ubuntu_intel/.local/lib/python3.10/site-packages/emlearn
TARGET_LIBFILES+=-lm
INC+=/home/iot_ubuntu_intel/.local/lib/python3.10/site-packages/emlearn

MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
MAKE_NET = MAKE_NET_IPV6

CONTIKI = ../..
$(shell mkdir -p build/cooja/obj/sensor build/cooja/obj/coap)

# Include the CoAP implementation
include $(CONTIKI)/Makefile.dir-variables
MODULES+=$(CONTIKI_NG_APP_LAYER_DIR)/coap
include $(CONTIKI)/Makefile.include