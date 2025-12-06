LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

# Add your application source files here...
LOCAL_SRC_FILES :=  \
  src/main.c src/utils.c src/logging.c src/svcs.c \
  src/sdlx_misc.c src/sdlx_video.c src/sdlx_audio.c src/sdlx_sensor.c src/sdlx_event.c \
  src/utils_android.cpp \
  cJSON/cJSON.c \
  lodepng/lodepng.c \
  picoc/picoc_ezapp.c \
  picoc/clibrary.c \
  picoc/debug.c \
  picoc/expression.c \
  picoc/heap.c \
  picoc/include.c \
  picoc/lex.c \
  picoc/parse.c \
  picoc/platform.c \
  picoc/table.c \
  picoc/type.c \
  picoc/variable.c \
  picoc/cstdlib/ctype.c \
  picoc/cstdlib/errno.c \
  picoc/cstdlib/math.c \
  picoc/cstdlib/stdbool.c \
  picoc/cstdlib/string.c \
  picoc/cstdlib/stdio.c \
  picoc/cstdlib/stdlib.c \
  picoc/cstdlib/time.c \
  picoc/cstdlib/unistd.c \
  picoc/platform/library_unix.c \
  picoc/platform/platform_unix.c

SDL_PATH := ../SDL
SDL_TTF_PATH := ../SDL3_ttf-3.2.2 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
                    $(LOCAL_PATH)/$(SDL_TTF_PATH) \
                    $(LOCAL_PATH)/src $(LOCAL_PATH)/picoc

LOCAL_SHARED_LIBRARIES := SDL3 SDL3_ttf

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid

include $(BUILD_SHARED_LIBRARY)
