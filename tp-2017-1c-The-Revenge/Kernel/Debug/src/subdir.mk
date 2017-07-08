################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Kernel.c \
../src/capaFileSystem.c \
../src/capaMemoria.c \
../src/conexiones.c \
../src/configInicialKernel.c \
../src/consolaKernel.c \
../src/planificador.c 

OBJS += \
./src/Kernel.o \
./src/capaFileSystem.o \
./src/capaMemoria.o \
./src/conexiones.o \
./src/configInicialKernel.o \
./src/consolaKernel.o \
./src/planificador.o 

C_DEPS += \
./src/Kernel.d \
./src/capaFileSystem.d \
./src/capaMemoria.d \
./src/conexiones.d \
./src/configInicialKernel.d \
./src/consolaKernel.d \
./src/planificador.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/tp-2017-1c-The-Revenge/tr_library" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


