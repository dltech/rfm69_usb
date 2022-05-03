#paths
TARGET = bin/modem
USBFS_DIR = USB_FS
USBFS2_DIR = USB_FS/STM32_USB_FS_Device_Driver/src
USBFS2_INC = USB_FS/STM32_USB_FS_Device_Driver/inc
STD_DIR	= STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/STM32F10x_StdPeriph_Driver/src
STD_INC	= STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/STM32F10x_StdPeriph_Driver/inc
RFM_DIR = RFM69
ODIR = bin/lib
BINDIR = bin

# code is here
SOURCES = main.c
SOURCES += $(USBFS_DIR)/hw_config.c
SOURCES += $(USBFS_DIR)/stm32_it.c
SOURCES += $(USBFS_DIR)/usb_desc.c
SOURCES += $(USBFS_DIR)/usb_endp.c
SOURCES += $(USBFS_DIR)/usb_istr.c
SOURCES += $(USBFS_DIR)/usb_prop.c
SOURCES += $(USBFS_DIR)/usb_pwr.c
SOURCES += $(USBFS2_DIR)/usb_core.c
SOURCES += $(USBFS2_DIR)/usb_init.c
SOURCES += $(USBFS2_DIR)/usb_int.c
SOURCES += $(USBFS2_DIR)/usb_mem.c
SOURCES += $(USBFS2_DIR)/usb_regs.c
SOURCES += $(USBFS2_DIR)/usb_sil.c
SOURCES += $(STD_DIR)/misc.c
SOURCES += $(STD_DIR)/stm32f10x_cec.c
SOURCES += $(STD_DIR)/stm32f10x_dma.c
SOURCES += $(STD_DIR)/stm32f10x_gpio.c
SOURCES += $(STD_DIR)/stm32f10x_rcc.c
SOURCES += $(STD_DIR)/stm32f10x_tim.c
SOURCES += $(STD_DIR)/stm32f10x_adc.c  
SOURCES += $(STD_DIR)/stm32f10x_crc.c
SOURCES += $(STD_DIR)/stm32f10x_exti.c
SOURCES += $(STD_DIR)/stm32f10x_i2c.c
SOURCES += $(STD_DIR)/stm32f10x_rtc.c
SOURCES += $(STD_DIR)/stm32f10x_usart.c
SOURCES += $(STD_DIR)/stm32f10x_bkp.c
SOURCES += $(STD_DIR)/stm32f10x_dac.c
SOURCES += $(STD_DIR)/stm32f10x_flash.c
SOURCES += $(STD_DIR)/stm32f10x_iwdg.c
SOURCES += $(STD_DIR)/stm32f10x_sdio.c
SOURCES += $(STD_DIR)/stm32f10x_wwdg.c
SOURCES += $(STD_DIR)/stm32f10x_can.c
SOURCES += $(STD_DIR)/stm32f10x_dbgmcu.c
SOURCES += $(STD_DIR)/stm32f10x_fsmc.c
SOURCES += $(STD_DIR)/stm32f10x_pwr.c
SOURCES += $(STD_DIR)/stm32f10x_spi.c
SOURCES += $(RFM_DIR)/rfm69.c

INCLUDES   = -I $(USBFS_DIR)/
INCLUDES  += -I $(USBFS2_INC)/
INCLUDES  += -I $(STD_INC)/
INCLUDES  += -I $(RFM_DIR)/
INCLUDES  += -I ./

# VENDOR SPECIFIC CODE
CMSIS_DIR = STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/CMSIS/CM3/CoreSupport
VENDOR_DIR = STM32F10x_StdPeriph_Lib_V3.5.0/Libraries/CMSIS/CM3/DeviceSupport/ST/STM32F10x
STARTUP = $(VENDOR_DIR)/startup/gcc_ride7/startup_stm32f10x_md.s
SOURCES += $(VENDOR_DIR)/system_stm32f10x.c
INCLUDES  += -I $(VENDOR_DIR)
INCLUDES  += -I $(CMSIS_DIR)
LD_SCRIPT = stm32f10x_md.ld
DEFINES = -DARMCM3 -DSTM32F10X_MD

# Parameters
CFLAGS +=-ffunction-sections -fdata-sections
CFLAGS +=-Wall -Wextra -Wformat=2 -Wformat-truncation
CFLAGS +=-Wdouble-promotion -Wshadow 
CFLAGS +=-fno-common -Os -ffreestanding
CFLAGS +=-mcpu=cortex-m3 -mthumb
CFLAGS +=-mfloat-abi=soft -std=gnu2x
CFLAGS += $(DEFINES)
CFLAGS +=-ggdb3
LFLAGS  = -T"$(LD_SCRIPT)" --specs=nano.specs -Wl,--gc-sections
LFLAGS += -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group
#utilities
CC = arm-none-eabi-gcc
LD = arm-none-eabi-gcc
OBJCOPY	= arm-none-eabi-objcopy
OBJDUMP	= arm-none-eabi-objdump

### making-taking aaaa #########################################################

#object files list
OBJECTS = $(SOURCES:%.c=$(ODIR)/%.o)

#main rule
all: $(TARGET).elf $(TARGET).bin $(TARGET).lst
#compile
$(ODIR)/%.o: %.c
	@echo "[CC] $@"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(INCLUDES)  -c $< -o $@
#compile startup
	@$(CC) $(CFLAGS) $(INCLUDES)  -c $(STARTUP) -o $(ODIR)/start.o

#link
$(TARGET).elf: $(OBJECTS)
	@echo "[LD] $@"
	@$(LD) $(CFLAGS) $(LFLAGS) $(OBJECTS) $(ODIR)/start.o -o $@

$(TARGET).bin: $(TARGET).elf
	@echo "[OBJCOPY] $@"
	@$(OBJCOPY) -O binary  $< $@

$(TARGET).lst: $(TARGET).elf
	@echo "[OBJDUMP] $@"
	@$(OBJDUMP) -S $< > $@

# Clean rule
#.PHONY:
clean:
	@rm -rf $(BINDIR)
	@rm -r $(TARGET).elf
	@rm -r $(TARGET).bin
	@rm -r $(TARGET).lst
