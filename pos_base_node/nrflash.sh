west build -b nrf52840dongle_nrf52840
nrfutil pkg generate --hw-version 52 --sd-req=0x00 \
        --application build/zephyr/zephyr.hex \
        --application-version 1 bsu.zip

nrfutil dfu usb-serial -pkg bsu.zip -p /dev/ttyACM0