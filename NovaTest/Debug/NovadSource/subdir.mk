################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../NovadSource/Control.cpp \
../NovadSource/FilePacketCapture.cpp \
../NovadSource/InterfacePacketCapture.cpp \
../NovadSource/Novad.cpp \
../NovadSource/PacketCapture.cpp \
../NovadSource/ProtocolHandler.cpp \
../NovadSource/Threads.cpp 

OBJS += \
./NovadSource/Control.o \
./NovadSource/FilePacketCapture.o \
./NovadSource/InterfacePacketCapture.o \
./NovadSource/Novad.o \
./NovadSource/PacketCapture.o \
./NovadSource/ProtocolHandler.o \
./NovadSource/Threads.o 

CPP_DEPS += \
./NovadSource/Control.d \
./NovadSource/FilePacketCapture.d \
./NovadSource/InterfacePacketCapture.d \
./NovadSource/Novad.d \
./NovadSource/PacketCapture.d \
./NovadSource/ProtocolHandler.d \
./NovadSource/Threads.d 


# Each subdirectory must supply rules for building sources it contributes
NovadSource/%.o: ../NovadSource/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src -I../../Novad/src -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 -pthread -std=c++0x -fprofile-arcs -ftest-coverage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


