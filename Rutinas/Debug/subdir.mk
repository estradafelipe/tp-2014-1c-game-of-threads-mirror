################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../colas.c \
../obtener_config.c \
../paquetes.c \
../serializadores.c \
../sockets.c \
../stack.c 

OBJS += \
./colas.o \
./obtener_config.o \
./paquetes.o \
./serializadores.o \
./sockets.o \
./stack.o 

C_DEPS += \
./colas.d \
./obtener_config.d \
./paquetes.d \
./serializadores.d \
./sockets.d \
./stack.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/commons" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


