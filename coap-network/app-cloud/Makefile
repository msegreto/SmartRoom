CONTIKI_PROJECT = appCloud
all: $(CONTIKI_PROJECT)

CONTIKI = /home/iot_ubuntu_intel/contiki-ng

# Escludi piattaforme leggere (come sky) se non ti servono
PLATFORMS_EXCLUDE = sky z1

# Abilita il supporto CoAP
MODULES += os/net/app-layer/coap
MODULES_REL+=../cJSON-master

include $(CONTIKI)/Makefile.include