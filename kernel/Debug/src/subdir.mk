################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/kernel.c \
../src/paquetes.c \
../src/pcp.c \
../src/plp.c \
../src/serializadores.c \
../src/sockets.c 

OBJS += \
./src/kernel.o \
./src/paquetes.o \
./src/pcp.o \
./src/plp.o \
./src/serializadores.o \
./src/sockets.o 

C_DEPS += \
./src/kernel.d \
./src/paquetes.d \
./src/pcp.d \
./src/plp.d \
./src/serializadores.d \
./src/sockets.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/commons" -I"/home/utnso/workspace/parser" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


