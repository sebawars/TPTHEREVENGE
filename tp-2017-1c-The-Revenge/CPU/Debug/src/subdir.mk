################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/CPU.c \
../src/configCpu.c \
../src/primitivasAnsisop.c 

OBJS += \
./src/CPU.o \
./src/configCpu.o \
./src/primitivasAnsisop.o 

C_DEPS += \
./src/CPU.d \
./src/configCpu.d \
./src/primitivasAnsisop.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2017-1c-The-Revenge/tr_library" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


