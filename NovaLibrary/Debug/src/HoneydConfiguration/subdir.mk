################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/HoneydConfiguration/HoneydConfiguration.cpp \
../src/HoneydConfiguration/NodeManager.cpp \
../src/HoneydConfiguration/OsPersonalityDb.cpp \
../src/HoneydConfiguration/ScriptsTable.cpp \
../src/HoneydConfiguration/VendorMacDb.cpp 

OBJS += \
./src/HoneydConfiguration/HoneydConfiguration.o \
./src/HoneydConfiguration/NodeManager.o \
./src/HoneydConfiguration/OsPersonalityDb.o \
./src/HoneydConfiguration/ScriptsTable.o \
./src/HoneydConfiguration/VendorMacDb.o 

CPP_DEPS += \
./src/HoneydConfiguration/HoneydConfiguration.d \
./src/HoneydConfiguration/NodeManager.d \
./src/HoneydConfiguration/OsPersonalityDb.d \
./src/HoneydConfiguration/ScriptsTable.d \
./src/HoneydConfiguration/VendorMacDb.d 


# Each subdirectory must supply rules for building sources it contributes
src/HoneydConfiguration/%.o: ../src/HoneydConfiguration/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0  `pkg-config --libs --cflags libnotify` -pthread -std=c++0x -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


