LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_CFLAGS += -DMMAP64
endif

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include\
    $(LOCAL_PATH)/lib


LOCAL_SRC_FILES:=\
    lib/bdf-font.cc \
    lib/content-streamer.cc \
    lib/framebuffer.cc \
    lib/gpio.cc \
    lib/graphics.cc \
    lib/hardware-mapping.c \
    lib/led-matrix.cc \
    lib/led-matrix-c.cc \
    lib/multiplex-mappers.cc \
    lib/options-initialize.cc \
    lib/pixel-mapper.cc \
    lib/thread.cc

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS +=-W -Wall -Wextra -Wno-unused-parameter -O3 -g -fPIC
#LOCAL_CXXFLAGS=$(LOCAL_CFLAGS) -fno-exceptions -std=c++11

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE:=librgbledmatrix

include $(BUILD_STATIC_LIBRARY)
                             
include $(CLEAR_VARS)

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_CFLAGS += -DMMAP64
endif


LOCAL_SRC_FILES:=\
	gpio.cpp

LOCAL_MODULE_TAGS := debug

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE:=rgb_led_matrix

LOCAL_STATIC_LIBRARIES :=\
	librgbledmatrix

include $(BUILD_EXECUTABLE)
