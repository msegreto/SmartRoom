CONTIKI_PROJECT = main
all: prebuild $(CONTIKI_PROJECT)

CONTIKI = /home/iot_ubuntu_arm/contiki-ng

# File sorgenti del progetto
PROJECT_SOURCEFILES += \
  main.c \
  logic.c \
  res-control.c

# Include delle directory
CFLAGS += -I.             # per logic.h, res-control.h, ecc.
CFLAGS += -I../cJSON-master

# Moduli Contiki
MODULES += os/net/app-layer/coap
MODULES_REL += ../cJSON-master

PLATFORMS_EXCLUDE = sky z1

# Prebuild: creazione cartelle di output
prebuild:
	@mkdir -p build/cooja/obj
	@mkdir -p build/cooja/obj/cJSON-master
	@mkdir -p build/cooja/obj/os/net/app-layer/coap

include $(CONTIKI)/Makefile.include
