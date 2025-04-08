LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)
LOCAL_C_INCLUDES += $(LOCAL_PATH)/src
# 获取当前架构类型
ifeq ($(TARGET_ARCH_ABI),x86)
    # x86 架构
    ARCH_ABI_NAME := x86
else ifeq ($(TARGET_ARCH_ABI),x86_64)
    # x86_64 架构
    ARCH_ABI_NAME := x86_64
else ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    # arm64-v8a 架构
    ARCH_ABI_NAME := arm64
else ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    # armeabi-v7a 架构
    ARCH_ABI_NAME := arm
else
    # 其他架构类型
    ARCH_ABI_NAME := unknown
endif

LOCAL_MODULE    := lrapi_$(ARCH_ABI_NAME)
LOCAL_SRC_FILES := src/lrapi.cpp
LOCAL_LDLIBS    := -lm -lz
LOCAL_CPPFLAGS += -std=gnu++0x
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_CFLAGS += -fvisibility-inlines-hidden
LOCAL_LDLIBS += -L$(SYSROOT)/usr/lib -llog
include $(BUILD_SHARED_LIBRARY)