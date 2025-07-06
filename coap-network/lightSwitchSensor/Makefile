# Imposta il nome utente
USER := $(shell whoami)

# Percorso Contiki in base al nome utente
ifeq ($(USER),iot_ubuntu_intel)
    CONTIKI := /home/iot_ubuntu_intel/contiki-ng
else ifeq ($(USER),iot_ubuntu_arm)
    CONTIKI := /home/iot_ubuntu_arm/contiki-ng
else
    $(error Utente non riconosciuto: $(USER))
endif

# Progetto e sorgenti
CONTIKI_PROJECT = main
PROJECT_SOURCEFILES += main.c res-light.c res-control.c

# Flags e moduli
CFLAGS += -I$(CONTIKI_NG)/os/net/app-layer/coap

MODULES_REL += ../cJSON-master
MODULES += $(CONTIKI_NG_APP_LAYER_DIR)/coap

# Inclusione Makefile di Contiki
include $(CONTIKI)/Makefile.include
