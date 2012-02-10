################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/LocalTrafficMonitor.cpp 

OBJS += \
./src/LocalTrafficMonitor.o 

CPP_DEPS += \
./src/LocalTrafficMonitor.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src -O0 -g3 -Wall -c -fmessage-length=0 -lpthread `pkg-config --libs --cflags libnotify` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


