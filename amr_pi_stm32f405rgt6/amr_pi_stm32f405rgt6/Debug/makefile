################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include Drivers/STM32F4xx_HAL_Driver/Src/subdir.mk
-include Core/Startup/subdir.mk
-include Core/Src/subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

OPTIONAL_TOOL_DEPS := \
$(wildcard ../makefile.defs) \
$(wildcard ../makefile.init) \
$(wildcard ../makefile.targets) \


BUILD_ARTIFACT_NAME := amr_pi_stm32f405rgt6
BUILD_ARTIFACT_EXTENSION := elf
BUILD_ARTIFACT_PREFIX :=
BUILD_ARTIFACT := $(BUILD_ARTIFACT_PREFIX)$(BUILD_ARTIFACT_NAME)$(if $(BUILD_ARTIFACT_EXTENSION),.$(BUILD_ARTIFACT_EXTENSION),)

# Add inputs and outputs from these tool invocations to the build variables 
EXECUTABLES += \
amr_pi_stm32f405rgt6.elf \

MAP_FILES += \
amr_pi_stm32f405rgt6.map \

SIZE_OUTPUT += \
default.size.stdout \

OBJDUMP_LIST += \
amr_pi_stm32f405rgt6.list \


# All Target
all: main-build

# Main-build Target
main-build: amr_pi_stm32f405rgt6.elf secondary-outputs

# Tool invocations
amr_pi_stm32f405rgt6.elf amr_pi_stm32f405rgt6.map: $(OBJS) $(USER_OBJS) C:\Users\ADMIN\Desktop\uet\Ky\ 1\ nam\ 4\Ung_dung_may_tinh_trong_do_luong_va_dieu_khien\amr\amr_pi_stm32f405rgt6\amr_pi_stm32f405rgt6\STM32F405RGTX_FLASH.ld makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-gcc -o "amr_pi_stm32f405rgt6.elf" @"objects.list" $(USER_OBJS) $(LIBS) -mcpu=cortex-m4 -T"C:\Users\ADMIN\Desktop\uet\Ky 1 nam 4\Ung_dung_may_tinh_trong_do_luong_va_dieu_khien\amr\amr_pi_stm32f405rgt6\amr_pi_stm32f405rgt6\STM32F405RGTX_FLASH.ld" --specs=nosys.specs -Wl,-Map="amr_pi_stm32f405rgt6.map" -Wl,--gc-sections -static --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -u _printf_float -Wl,--start-group -lc -lm -Wl,--end-group
	@echo 'Finished building target: $@'
	@echo ' '

default.size.stdout: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-size  $(EXECUTABLES)
	@echo 'Finished building: $@'
	@echo ' '

amr_pi_stm32f405rgt6.list: $(EXECUTABLES) makefile objects.list $(OPTIONAL_TOOL_DEPS)
	arm-none-eabi-objdump -h -S $(EXECUTABLES) > "amr_pi_stm32f405rgt6.list"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) amr_pi_stm32f405rgt6.elf amr_pi_stm32f405rgt6.list amr_pi_stm32f405rgt6.map default.size.stdout
	-@echo ' '

secondary-outputs: $(SIZE_OUTPUT) $(OBJDUMP_LIST)

fail-specified-linker-script-missing:
	@echo 'Error: Cannot find the specified linker script. Check the linker settings in the build configuration.'
	@exit 2

warn-no-linker-script-specified:
	@echo 'Warning: No linker script specified. Check the linker settings in the build configuration.'

.PHONY: all clean dependents main-build fail-specified-linker-script-missing warn-no-linker-script-specified

-include ../makefile.targets
