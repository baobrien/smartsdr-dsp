################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../circular_buffer.c \
../main.c \
../resampler.c 

OBJS += \
./circular_buffer.o \
./main.o \
./resampler.o 

C_DEPS += \
./circular_buffer.d \
./main.d \
./resampler.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -static -I"/home/baobrien/workspace/smartsdr-dsp/DSP_API" -I"/home/baobrien/workspace/smartsdr-dsp/DSP_API/CODEC2_FREEDV" -I"/home/baobrien/workspace/smartsdr-dsp/DSP_API/SmartSDR_Interface" -O3 -mcpu=cortex-a8 -mfloat-abi=softfp -mfpu=neon -ffast-math -funroll-loops -ftree-vectorize -g3 -p -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


