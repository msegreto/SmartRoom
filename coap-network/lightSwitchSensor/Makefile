CONTIKI_PROJECT = main
PROJECT_SOURCEFILES += \
  main.c \
  button-handler.c \
  monitoring-process.c \
  sensor-simulation.c \
  res-light.c \
  prova.c

all: prebuild $(CONTIKI_PROJECT)

CONTIKI = ../..

# Include per CoAP
CFLAGS += -I$(CONTIKI)/os/net/app-layer/coap

# Moduli esterni (es. cJSON se necessario)
# MODULES_REL += ../cJSON-master  # Decommenta se usi cJSON

MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap
MODULES_REL += ../cJSON-master

# Crea le cartelle obj per il build
prebuild:
	@mkdir -p build/cooja/obj

include $(CONTIKI)/Makefile.include
