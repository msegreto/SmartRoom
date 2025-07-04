CONTIKI_PROJECT = main
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += \
  main.c \
  res-light.c

CFLAGS += -I$(CONTIKI_NG)/os/net/app-layer/coap

MAKE_NET = MAKE_NET_IPV6

CONTIKI = /home/iot_ubuntu_arm/contiki-ng

MODULES_REL += ../cJSON-master

include $(CONTIKI)/Makefile.include
