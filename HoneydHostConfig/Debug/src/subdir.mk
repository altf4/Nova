################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Personality.cpp \
../src/PersonalityTable.cpp \
../src/honeydhostconfig.cpp 

OBJS += \
./src/Personality.o \
./src/PersonalityTable.o \
./src/honeydhostconfig.o 

CPP_DEPS += \
./src/Personality.d \
./src/PersonalityTable.d \
./src/honeydhostconfig.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/addison/Code/Nova/NovaLibrary/src" -I"/home/addison/Code/Nova/Nova_UI_Core/src" -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


