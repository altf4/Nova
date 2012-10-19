################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/HaystackAutoConfig.cpp \
../src/Personality.cpp \
../src/PersonalityNode.cpp \
../src/PersonalityTable.cpp \
../src/PersonalityTree.cpp 

OBJS += \
./src/HaystackAutoConfig.o \
./src/Personality.o \
./src/PersonalityNode.o \
./src/PersonalityTable.o \
./src/PersonalityTree.o 

CPP_DEPS += \
./src/HaystackAutoConfig.d \
./src/Personality.d \
./src/PersonalityNode.d \
./src/PersonalityTable.d \
./src/PersonalityTree.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


