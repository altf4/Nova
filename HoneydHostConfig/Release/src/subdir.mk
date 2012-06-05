################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/HoneydHostConfig.cpp \
../src/Personality.cpp \
../src/PersonalityNode.cpp \
../src/PersonalityTable.cpp \
../src/PersonalityTree.cpp \
../src/ScriptTable.cpp 

OBJS += \
./src/HoneydHostConfig.o \
./src/Personality.o \
./src/PersonalityNode.o \
./src/PersonalityTable.o \
./src/PersonalityTree.o \
./src/ScriptTable.o 

CPP_DEPS += \
./src/HoneydHostConfig.d \
./src/Personality.d \
./src/PersonalityNode.d \
./src/PersonalityTable.d \
./src/PersonalityTree.d \
./src/ScriptTable.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/addison/Code/Nova/NovaLibrary/src" -I"/home/addison/Code/Nova/Nova_UI_Core/src" -O3 -Wall -c -fmessage-length=0 -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


