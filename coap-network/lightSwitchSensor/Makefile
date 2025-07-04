CONTIKI_PROJECT = main
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += \
  main.c \
  res-light.c

MAKE_NET = MAKE_NET_IPV6

CONTIKI = /home/iot_ubuntu_arm/contiki-ng

MODULES_REL += ../cJSON-master

include $(CONTIKI)/Makefile.include
