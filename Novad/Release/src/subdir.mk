################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ClassificationEngine.cpp \
../src/DoppelgangerModule.cpp \
../src/Haystack.cpp \
../src/LocalTrafficMonitor.cpp \
../src/Novad.cpp 

OBJS += \
./src/ClassificationEngine.o \
./src/DoppelgangerModule.o \
./src/Haystack.o \
./src/LocalTrafficMonitor.o \
./src/Novad.o 

CPP_DEPS += \
./src/ClassificationEngine.d \
./src/DoppelgangerModule.d \
./src/Haystack.d \
./src/LocalTrafficMonitor.d \
./src/Novad.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src/ -O3 -Wall -c -fmessage-length=0 `pkg-config --libs --cflags libnotify` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


