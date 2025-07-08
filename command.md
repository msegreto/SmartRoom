cd examples/rpl-border-router
make TARGET=cooja connect-router-cooja
make TARGET=nrf52840 BOARD=dongle PORT=/dev/ttyACM0 connect-router

make TARGET=nrf52840 BOARD=dongle main.dfu-upload PORT=/dev/ttyACMx
make login TARGET=nrf52840 BOARD=dongle PORT=/dev/ttyACMx

