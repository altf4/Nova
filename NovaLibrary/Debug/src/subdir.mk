################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/FeatureSet.cpp \
../src/GUIMsg.cpp \
../src/Logger.cpp \
../src/NOVAConfiguration.cpp \
../src/NovaUtil.cpp \
../src/Point.cpp \
../src/Suspect.cpp 

OBJS += \
./src/FeatureSet.o \
./src/GUIMsg.o \
./src/Logger.o \
./src/NOVAConfiguration.o \
./src/NovaUtil.o \
./src/Point.o \
./src/Suspect.o 

CPP_DEPS += \
./src/FeatureSet.d \
./src/GUIMsg.d \
./src/Logger.d \
./src/NOVAConfiguration.d \
./src/NovaUtil.d \
./src/Point.d \
./src/Suspect.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0  `pkg-config --libs --cflags glib-2.0 gtk+-3.0` -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/NovaUtil.o: ../src/NovaUtil.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/lib/i386-linux-gnu/glib-2.0/include -I/usr/include/libnotify -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/glib-2.0 -O0 -g3 -Wall -c -fmessage-length=0  `pkg-config --libs --cflags glib-2.0 gtk+-3.0` -MMD -MP -MF"$(@:%.o=%.d)" -MT"src/NovaUtil.d" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


