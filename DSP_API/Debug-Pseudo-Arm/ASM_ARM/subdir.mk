################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../ASM_ARM/cohpsk_neon_kernel.gnu.S 

OBJS += \
./ASM_ARM/cohpsk_neon_kernel.gnu.o 


# Each subdirectory must supply rules for building sources it contributes
ASM_ARM/%.o: ../ASM_ARM/%.S
	@echo 'Building file: $<'
	@echo 'Invoking: GCC Assembler'
	as  -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


