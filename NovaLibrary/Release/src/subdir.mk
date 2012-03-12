################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Config.cpp \
../src/FeatureSet.cpp \
../src/Logger.cpp \
../src/NovaUtil.cpp \
../src/Point.cpp \
../src/Suspect.cpp \
../src/SuspectTable.cpp \
../src/SuspectTableIterator.cpp 

OBJS += \
./src/Config.o \
./src/FeatureSet.o \
./src/Logger.o \
./src/NovaUtil.o \
./src/Point.o \
./src/Suspect.o \
./src/SuspectTable.o \
./src/SuspectTableIterator.o 

CPP_DEPS += \
./src/Config.d \
./src/FeatureSet.d \
./src/Logger.d \
./src/NovaUtil.d \
./src/Point.d \
./src/Suspect.d \
./src/SuspectTable.d \
./src/SuspectTableIterator.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0  `pkg-config --libs --cflags libnotify` -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


