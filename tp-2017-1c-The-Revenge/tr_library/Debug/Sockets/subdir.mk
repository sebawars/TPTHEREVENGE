################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Sockets/Serializer.c \
../Sockets/SocketsCliente.c \
../Sockets/socket.c 

OBJS += \
./Sockets/Serializer.o \
./Sockets/SocketsCliente.o \
./Sockets/socket.o 

C_DEPS += \
./Sockets/Serializer.d \
./Sockets/SocketsCliente.d \
./Sockets/socket.d 


# Each subdirectory must supply rules for building sources it contributes
Sockets/%.o: ../Sockets/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


