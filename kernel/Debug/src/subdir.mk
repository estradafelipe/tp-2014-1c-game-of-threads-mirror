################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/hilos.c \
../src/kernel.c \
../src/pcp.c \
../src/plp.c 

OBJS += \
./src/hilos.o \
./src/kernel.o \
./src/pcp.o \
./src/plp.o 

C_DEPS += \
./src/hilos.d \
./src/kernel.d \
./src/pcp.d \
./src/plp.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


