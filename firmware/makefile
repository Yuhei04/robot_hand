# cd firmware
proj ?= xiao_esp32c3_five_motor_angle_control

FQBN := esp32:esp32:esp32c3:CDCOnBoot=cdc
BAUD := 115200
PORT ?= $(shell ls /dev/cu.usbmodem* 2>/dev/null | head -n 1)

.PHONY: build upload monitor flash ports

build:
	arduino-cli compile --fqbn $(FQBN) $(proj)

upload:
	@if [ -z "$(PORT)" ]; then \
		echo "No /dev/cu.usbmodem* port found"; \
		exit 1; \
	fi
	@echo "Using port: $(PORT)"
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) $(proj)

monitor:
	@if [ -z "$(PORT)" ]; then \
		echo "No /dev/cu.usbmodem* port found"; \
		exit 1; \
	fi
	@echo "Using port: $(PORT)"
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)

flash: build upload

ports:
	arduino-cli board list
