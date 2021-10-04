LOCAL_PATH:= $(call my-dir)
OPENCV3_PATH:=$(LOCAL_PATH)/../opencv3

include $(CLEAR_VARS)

ifeq (1,$(strip $(shell expr $(PLATFORM_VERSION) \>= 5.0)))
LOCAL_CFLAGS += -DMMAP64
endif
LOCAL_CLANG := true
LOCAL_CPP_EXTENSION := .cc

LOCAL_CLANG_CFLAGS += -Wno-c++11-narrowing

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/include\
    $(LOCAL_PATH)/lib \
    $(OPENCV3_PATH) \
    $(OPENCV3_PATH)/include \
    $(OPENCV3_PATH)/modules/core/include \
    $(OPENCV3_PATH)/modules/hal/include \
    $(OPENCV3_PATH)/opencv2 \
    $(OPENCV3_PATH)/modules/highgui/include \
    $(OPENCV3_PATH)/modules/superres \
    $(OPENCV3_PATH)/modules/video/include \
    $(OPENCV3_PATH)/modules/imgproc/include \
    $(OPENCV3_PATH)/modules/videoio/include \
    $(OPENCV3_PATH)/modules/superres/include \
    $(OPENCV3_PATH)/modules/superres/src \
    $(OPENCV3_PATH)/modules/photo \
    $(OPENCV3_PATH)/modules/photo/include \
    $(OPENCV3_PATH)/modules/flann/include \
    $(OPENCV3_PATH)/modules/imgcodecs/include \
    $(OPENCV3_PATH)/modules/stitching \
    $(OPENCV3_PATH)/modules/flann/include \
    $(OPENCV3_PATH)/modules/features2d/include \
    $(OPENCV3_PATH)/modules/calib3d/include \
    $(OPENCV3_PATH)/modules/stitching/include \
    $(OPENCV3_PATH)/modules/stitching \
    $(OPENCV3_PATH)/modules/objdetect \
    $(OPENCV3_PATH)/modules/objdetect/src \
    $(OPENCV3_PATH)/modules/objdetect/include \
    $(OPENCV3_PATH)/modules/objdetect \
    $(OPENCV3_PATH)/modules/ml/include \
    $(OPENCV3_PATH)/modules/imgcodecs/include/



LOCAL_SRC_FILES:=\
    lib/bdf-font.cc \
    lib/content-streamer.cc \
    lib/framebuffer.cc \
    lib/gpio_rk3288.cc \
    lib/graphics.cc \
    lib/hardware-mapping.c \
    lib/led-matrix.cc \
    lib/led-matrix-c.cc \
    lib/multiplex-mappers.cc \
    lib/options-initialize.cc \
    lib/pixel-mapper.cc \
    lib/thread.cc \
    examples-api-use/c-example.c\
    examples-api-use/scrolling-text-example.cc\
    examples-api-use/input-example.cc\
    examples-api-use/clock.cc\
    examples-api-use/pixel-mover.cc\
    examples-api-use/text-example.cc\
    examples-api-use/minimal-example.cc\
    examples-api-use/demo-main.cc\
    utils/led-image-viewer.cc\
    test.cc

LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS +=-W -Wall -Wextra -Wno-unused-parameter -O3 -g -fPIC
LOCAL_CFLAGS += -fexceptions -D__OPENCV_BUILD=1 -DCVAPI_EXPORTS


LOCAL_CPPFLAGS := \
        -std=c++11 
#LOCAL_CXXFLAGS=$(LOCAL_CFLAGS) -fno-exceptions -std=c++11

LOCAL_SHARED_LIBRARIES += libopencv_core libopencv_imgproc libopencv_video libopencv_imgcodecs libopencv_videoio libopencv_photo libopencv_stitching libopencv_objdetect libopencv_ml

LOCAL_STATIC_LIBRARIES += libopencv_hal


LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)

LOCAL_MODULE:=rgb_led_matrix

include $(BUILD_EXECUTABLE)
