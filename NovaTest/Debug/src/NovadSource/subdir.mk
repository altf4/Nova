################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/victim/Code/Nova/Novad/src/ClassificationEngine.cpp \
/home/victim/Code/Nova/Novad/src/Control.cpp \
/home/victim/Code/Nova/Novad/src/Novad.cpp \
/home/victim/Code/Nova/Novad/src/ProtocolHandler.cpp 

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
src/NovadSource/ClassificationEngine.o: /home/victim/Code/Nova/Novad/src/ClassificationEngine.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/NovadSource/Control.o: /home/victim/Code/Nova/Novad/src/Control.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/NovadSource/Novad.o: /home/victim/Code/Nova/Novad/src/Novad.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/NovadSource/ProtocolHandler.o: /home/victim/Code/Nova/Novad/src/ProtocolHandler.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I../../NovaLibrary/src -I../../Nova_UI_Core/src -O0 -g3 -Wall -c -fmessage-length=0 -pthread -std=c++0x -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


