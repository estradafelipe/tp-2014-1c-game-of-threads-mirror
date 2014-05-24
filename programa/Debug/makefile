################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: programa

dependents:
	-cd /home/utnso/workspace/Rutinas/Debug && $(MAKE) all

# Tool invocations
programa: $(OBJS) $(USER_OBJS) /home/utnso/workspace/Rutinas/Debug/libRutinas.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C Linker'
	gcc -L"/home/utnso/workspace/Rutinas/Debug" -L"/home/utnso/workspace/commons/Debug" -o "programa" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) programa
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/utnso/workspace/Rutinas/Debug/libRutinas.so:

-include ../makefile.targets
