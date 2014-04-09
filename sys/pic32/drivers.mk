ifeq ($(DRIVER_GPIO),yes)
	KERNOBJ += gpio.o
	DEFS    += -DGPIO_ENABLED
endif

ifeq ($(DRIVER_POWER),yes)
        KERNOBJ += power_control.o
        POWER_LED_PORT ?= TRISG
        POWER_LED_PIN ?= 14
        POWER_SWITCH_PORT ?= TRISE
        POWER_SWITCH_PIN ?= 9
        POWER_CONTROL_PORT ?= TRISE
        POWER_CONTROL_PIN ?= 9
        DEFS    += -DPOWER_ENABLED -DPOWER_LED_PORT=$(POWER_LED_PORT) -DPOWER_LED_PIN=$(POWER_LED_PIN)
        DEFS    += -DPOWER_SWITCH_PORT=$(POWER_SWITCH_PORT) -DPOWER_SWITCH_PIN=$(POWER_SWITCH_PIN)
        DEFS    += -DPOWER_CONTROL_PORT=$(POWER_CONTROL_PORT) -DPOWER_CONTROL_PIN=$(POWER_CONTROL_PIN)
endif

ifeq ($(DRIVER_ADC),yes)
	KERNOBJ += adc.o
	DEFS    += -DADC_ENABLED
endif

ifeq ($(DRIVER_SPI),yes)
	KERNOBJ += spi.o
	DEFS    += -DSPI_ENABLED
endif

ifeq ($(DRIVER_GLCD),yes)
	KERNOBJ += glcd.o
	DEFS	+= -DGLCD_ENABLED
endif

ifeq ($(DRIVER_OC),yes)
	KERNOBJ += oc.o
	DEFS 	+= -DOC_ENABLED
endif

ifeq ($(DRIVER_SDRAMP),yes)
        NEEDS_FEATURE_KERNEL_EXECUTABLE_RAM = yes
        KERNOBJ += sdram.o rd_sdramp.o
	DEFS	+= -DSDRAMP_ENABLED
endif