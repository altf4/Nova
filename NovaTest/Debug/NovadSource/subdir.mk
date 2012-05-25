################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../NovadSource/ClassificationEngine.cpp \
../NovadSource/Control.cpp \
../NovadSource/Doppelganger.cpp \
../NovadSource/EvidenceTable.cpp \
../NovadSource/Novad.cpp \
../NovadSource/ProtocolHandler.cpp \
../NovadSource/SuspectTable.cpp \
../NovadSource/Threads.cpp 

OBJS += \
./NovadSource/ClassificationEngine.o \
./NovadSource/Control.o \
./NovadSource/Doppelganger.o \
./NovadSource/EvidenceTable.o \
./NovadSource/Novad.o \
./NovadSource/ProtocolHandler.o \
./NovadSource/SuspectTable.o \
./NovadSource/Threads.o 

CPP_DEPS += \
./NovadSource/ClassificationEngine.d \
./NovadSource/Control.d \
./NovadSource/Doppelganger.d \
./NovadSource/EvidenceTable.d \
./NovadSource/Novad.d \
./NovadSource/ProtocolHandler.d \
./NovadSource/SuspectTable.d \
./NovadSource/Threads.d 


# Each subdirectory must supply rules for building sources it contributes
NovadSource/%.o: ../NovadSource/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src -I../../../gtest-1.6.0/include -I../../Novad/src -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


