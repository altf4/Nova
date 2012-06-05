################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Personality.cpp \
../src/PersonalityNode.cpp \
../src/PersonalityTable.cpp \
../src/PersonalityTree.cpp \
../src/ScriptTable.cpp \
../src/honeydhostconfig.cpp 

OBJS += \
./src/Personality.o \
./src/PersonalityNode.o \
./src/PersonalityTable.o \
./src/PersonalityTree.o \
./src/ScriptTable.o \
./src/honeydhostconfig.o 

CPP_DEPS += \
./src/Personality.d \
./src/PersonalityNode.d \
./src/PersonalityTable.d \
./src/PersonalityTree.d \
./src/ScriptTable.d \
./src/honeydhostconfig.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I"/home/victim/Code/Nova/NovaLibrary/src" -I"/home/victim/Code/Nova/Nova_UI_Core/src" -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


