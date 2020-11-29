//
// Created by SnowNF on 20-9-20.
//

#ifndef EGL_EGL_CONFIG_H
#define EGL_EGL_CONFIG_H
//定义我自己的debug宏
#define MY_DEBUG

#define _EGL_NATIVE_PLATFORM _EGL_PLATFORM_X11
#define HAVE_X11_PLATFORM

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

//去除一些原有的检查来提升fps，尤其是eglSwapBuffers()
#define FAST_FUNC

//已删除的，永远不会再使用的函数，取消定义此宏可能会引起编译错误
#define DELETED_FUNC

//通过gl4es能够运行glxgears，minecraft，particles的最少函数，速度最快，取消定义此宏会影响速度，但函数完整
#define MINIMAL_FUNC

#endif //EGL_EGL_CONFIG_H
