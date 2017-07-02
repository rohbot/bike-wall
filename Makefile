BOARD_TAG    = mega
MONITOR_PORT = /dev/ttyACM0
USER_LIB_PATH = libraries
ARDUINO_LIBS += bitBangedSPI \
                SPI \
                MAX7219_Dot_Matrix \
                FastLED
#include /usr/roh/arduino-1.6.8/Arduino.mk
include /usr/share/arduino/Arduino.mk