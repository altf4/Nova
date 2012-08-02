################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: honeydhostconfig

dependents:
	-cd /home/addison/Code/Nova/NovaLibrary/Release && $(MAKE) clean all
	-cd /home/addison/Code/Nova/Nova_UI_Core/Release && $(MAKE) clean all

# Tool invocations
honeydhostconfig: $(OBJS) $(USER_OBJS) /home/addison/Code/Nova/NovaLibrary/Release/libNovaLibrary.a /home/addison/Code/Nova/Nova_UI_Core/Release/libNova_UI_Core.so
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++ -L/Nova_UI_Core/Release -shared -o "honeydhostconfig" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C++_DEPS)$(C_DEPS)$(CC_DEPS)$(LIBRARIES)$(CPP_DEPS)$(CXX_DEPS)$(C_UPPER_DEPS) honeydhostconfig
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:
/home/addison/Code/Nova/NovaLibrary/Release/libNovaLibrary.a:
/home/addison/Code/Nova/Nova_UI_Core/Release/libNova_UI_Core.so:

-include ../makefile.targets
