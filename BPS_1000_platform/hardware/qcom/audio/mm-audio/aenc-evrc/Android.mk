ifneq ($(filter arm aarch64 arm64, $(TARGET_ARCH)),)

AENC_EVRC_PATH:= $(call my-dir)

include $(AENC_EVRC_PATH)/qdsp6/Android.mk

endif
