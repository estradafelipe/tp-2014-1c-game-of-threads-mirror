################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/socket/cliente.c \
../src/socket/sockets.c 

OBJS += \
./src/socket/cliente.o \
./src/socket/sockets.o 

C_DEPS += \
./src/socket/cliente.d \
./src/socket/sockets.d 


# Each subdirectory must supply rules for building sources it contributes
src/socket/%.o: ../src/socket/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/commons" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


