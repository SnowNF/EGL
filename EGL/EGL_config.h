//
// Created by SnowNF on 20-9-20.
//

#ifndef EGL_EGL_CONFIG_H
#define EGL_EGL_CONFIG_H
//定义我自己的debug宏
#define MY_DEBUG

#define _EGL_NATIVE_PLATFORM _EGL_PLATFORM_X11
#define HAVE_X11_PLATFORM
#define _EGL_BUILT_IN_DRIVER_DRI2

//指定默认dri驱动搜索目录
#define DEFAULT_DRIVER_DIR "/usr/local/lib/dri"

//set build-time logger level,this will overwrite EGL_LOG_LEVEL environment variable value.
//设置编译时log级别,将会覆盖环境变量: EGL_LOG_LEVEL
/*
 _EGL_FATAL   0    unrecoverable error
 _EGL_WARNING 1    recoverable error/problem (default)
 _EGL_INFO    2    just useful info
 _EGL_DEBUG   3    useful info for debugging
 */
#define EGL_BUILD_LOG_LEVEL 3

#endif //EGL_EGL_CONFIG_H
