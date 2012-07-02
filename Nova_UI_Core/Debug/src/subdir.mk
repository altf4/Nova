################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/CallbackHandler.cpp \
../src/Connection.cpp \
../src/HoneydConfiguration.cpp \
../src/NovadControl.cpp \
../src/OsPersonalityDb.cpp \
../src/StatusQueries.cpp \
../src/TrainingData.cpp \
../src/TrainingDump.cpp \
../src/VendorMacDb.cpp 

OBJS += \
./src/CallbackHandler.o \
./src/Connection.o \
./src/HoneydConfiguration.o \
./src/NovadControl.o \
./src/OsPersonalityDb.o \
./src/StatusQueries.o \
./src/TrainingData.o \
./src/TrainingDump.o \
./src/VendorMacDb.o 

CPP_DEPS += \
./src/CallbackHandler.d \
./src/Connection.d \
./src/HoneydConfiguration.d \
./src/NovadControl.d \
./src/OsPersonalityDb.d \
./src/StatusQueries.d \
./src/TrainingData.d \
./src/TrainingDump.d \
./src/VendorMacDb.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O0 -g3 -Wall -c -fmessage-length=0 -std=c++0x -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


