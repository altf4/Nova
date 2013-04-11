################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ClassificationAggregator.cpp \
../src/ClassificationEngine.cpp \
../src/ClassificationEngineFactory.cpp \
../src/Control.cpp \
../src/KnnClassification.cpp \
../src/Main.cpp \
../src/Novad.cpp \
../src/ProtocolHandler.cpp \
../src/Threads.cpp \
../src/ThresholdTriggerClassification.cpp 

OBJS += \
./src/ClassificationAggregator.o \
./src/ClassificationEngine.o \
./src/ClassificationEngineFactory.o \
./src/Control.o \
./src/KnnClassification.o \
./src/Main.o \
./src/Novad.o \
./src/ProtocolHandler.o \
./src/Threads.o \
./src/ThresholdTriggerClassification.o 

CPP_DEPS += \
./src/ClassificationAggregator.d \
./src/ClassificationEngine.d \
./src/ClassificationEngineFactory.d \
./src/Control.d \
./src/KnnClassification.d \
./src/Main.d \
./src/Novad.d \
./src/ProtocolHandler.d \
./src/Threads.d \
./src/ThresholdTriggerClassification.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O0 -g3 -Wall -c -fmessage-length=0 -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


