LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_CFLAGS += -DMMAP64
endif


LOCAL_SRC_FILES:=\
        gpio.cpp

LOCAL_MODULE_TAGS := debug

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE:=rgb_led_matrix

include $(BUILD_EXECUTABLE)
                             
include $(CLEAR_VARS)

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_CFLAGS += -DMMAP64
endif


LOCAL_SRC_FILES:=\
	gpio.cpp

LOCAL_MODULE_TAGS := debug

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE:=rgb_led_matrix

include $(BUILD_EXECUTABLE)
