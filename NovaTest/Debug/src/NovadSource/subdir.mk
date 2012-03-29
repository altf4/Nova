################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/NovadSource/ClassificationEngine.cpp \
../src/NovadSource/Control.cpp \
../src/NovadSource/Novad.cpp \
../src/NovadSource/ProtocolHandler.cpp 

OBJS += \
./src/NovadSource/ClassificationEngine.o \
./src/NovadSource/Control.o \
./src/NovadSource/Novad.o \
./src/NovadSource/ProtocolHandler.o 

CPP_DEPS += \
./src/NovadSource/ClassificationEngine.d \
./src/NovadSource/Control.d \
./src/NovadSource/Novad.d \
./src/NovadSource/ProtocolHandler.d 


# Each subdirectory must supply rules for building sources it contributes
src/NovadSource/%.o: ../src/NovadSource/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src -I../../../gtest-1.6.0/include -I../../Novad/src -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


