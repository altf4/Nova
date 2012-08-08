################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ClassificationEngine.cpp \
../src/Config.cpp \
../src/Database.cpp \
../src/Doppelganger.cpp \
../src/Evidence.cpp \
../src/EvidenceTable.cpp \
../src/FeatureSet.cpp \
../src/HaystackControl.cpp \
../src/HoneydConfiguration.cpp \
../src/Logger.cpp \
../src/NodeManager.cpp \
../src/NovaUtil.cpp \
../src/Point.cpp \
../src/Suspect.cpp \
../src/SuspectTable.cpp \
../src/VendorMacDb.cpp \
../src/WhitelistConfiguration.cpp 

OBJS += \
./src/ClassificationEngine.o \
./src/Config.o \
./src/Database.o \
./src/Doppelganger.o \
./src/Evidence.o \
./src/EvidenceTable.o \
./src/FeatureSet.o \
./src/HaystackControl.o \
./src/HoneydConfiguration.o \
./src/Logger.o \
./src/NodeManager.o \
./src/NovaUtil.o \
./src/Point.o \
./src/Suspect.o \
./src/SuspectTable.o \
./src/VendorMacDb.o \
./src/WhitelistConfiguration.o 

CPP_DEPS += \
./src/ClassificationEngine.d \
./src/Config.d \
./src/Database.d \
./src/Doppelganger.d \
./src/Evidence.d \
./src/EvidenceTable.d \
./src/FeatureSet.d \
./src/HaystackControl.d \
./src/HoneydConfiguration.d \
./src/Logger.d \
./src/NodeManager.d \
./src/NovaUtil.d \
./src/Point.d \
./src/Suspect.d \
./src/SuspectTable.d \
./src/VendorMacDb.d \
./src/WhitelistConfiguration.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0  `pkg-config --libs --cflags libnotify` -std=c++0x -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


