################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../rutinas/colas.c \
../rutinas/obtener_config.c \
../rutinas/paquetes.c \
../rutinas/serializadores.c \
../rutinas/sockets.c \
../rutinas/stack.c 

OBJS += \
./rutinas/colas.o \
./rutinas/obtener_config.o \
./rutinas/paquetes.o \
./rutinas/serializadores.o \
./rutinas/sockets.o \
./rutinas/stack.o 

C_DEPS += \
./rutinas/colas.d \
./rutinas/obtener_config.d \
./rutinas/paquetes.d \
./rutinas/serializadores.d \
./rutinas/sockets.d \
./rutinas/stack.d 


# Each subdirectory must supply rules for building sources it contributes
rutinas/%.o: ../rutinas/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


