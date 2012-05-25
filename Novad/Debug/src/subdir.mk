################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ClassificationEngine.cpp \
../src/Control.cpp \
../src/Doppelganger.cpp \
../src/EvidenceTable.cpp \
../src/Main.cpp \
../src/Novad.cpp \
../src/ProtocolHandler.cpp \
../src/SuspectTable.cpp \
../src/Threads.cpp 

OBJS += \
./src/ClassificationEngine.o \
./src/Control.o \
./src/Doppelganger.o \
./src/EvidenceTable.o \
./src/Main.o \
./src/Novad.o \
./src/ProtocolHandler.o \
./src/SuspectTable.o \
./src/Threads.o 

CPP_DEPS += \
./src/ClassificationEngine.d \
./src/Control.d \
./src/Doppelganger.d \
./src/EvidenceTable.d \
./src/Main.d \
./src/Novad.d \
./src/ProtocolHandler.d \
./src/SuspectTable.d \
./src/Threads.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 `pkg-config --libs --cflags libnotify` -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


