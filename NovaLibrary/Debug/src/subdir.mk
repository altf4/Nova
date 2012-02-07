################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/FeatureSet.cpp \
../src/GUIMsg.cpp \
../src/NOVAConfiguration.cpp \
../src/NovaUtil.cpp \
../src/Point.cpp \
../src/Suspect.cpp 

OBJS += \
./src/FeatureSet.o \
./src/GUIMsg.o \
./src/NOVAConfiguration.o \
./src/NovaUtil.o \
./src/Point.o \
./src/Suspect.o 

CPP_DEPS += \
./src/FeatureSet.d \
./src/GUIMsg.d \
./src/NOVAConfiguration.d \
./src/NovaUtil.d \
./src/Point.d \
./src/Suspect.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


