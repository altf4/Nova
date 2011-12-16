################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/FeatureSet.cpp \
../src/GUIMsg.cpp \
../src/Point.cpp \
../src/Suspect.cpp \
../src/TrafficEvent.cpp 

OBJS += \
./src/FeatureSet.o \
./src/GUIMsg.o \
./src/Point.o \
./src/Suspect.o \
./src/TrafficEvent.o 

CPP_DEPS += \
./src/FeatureSet.d \
./src/GUIMsg.d \
./src/Point.d \
./src/Suspect.d \
./src/TrafficEvent.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


