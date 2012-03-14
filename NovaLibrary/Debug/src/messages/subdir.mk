################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/messages/CallbackMessage.cpp \
../src/messages/ControlMessage.cpp \
../src/messages/UI_Message.cpp 

OBJS += \
./src/messages/CallbackMessage.o \
./src/messages/ControlMessage.o \
./src/messages/UI_Message.o 

CPP_DEPS += \
./src/messages/CallbackMessage.d \
./src/messages/ControlMessage.d \
./src/messages/UI_Message.d 


# Each subdirectory must supply rules for building sources it contributes
src/messages/%.o: ../src/messages/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0  `pkg-config --libs --cflags libnotify` -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


