cmake_minimum_required(VERSION 3.7)
project(EGL C)

set(CMAKE_C_STANDARD 11)

include_directories(EGL/include/
        EGL/main/
        EGL/loader/
        EGL/
        )

#set(CMAKE_C_FLAGS "-pthread -lxcb -lxcb-dri2 -ldl -lxcb-xfixes -lX11-xcb")


#add_definitions(-DHAVE_X11_PLATFORM)
#add_definitions(-DHAVE_SURFACELESS_PLATFORM)
#add_definitions(-D_EGL_BUILT_IN_DRIVER_DRI2)
#add_definitions(-D_EGL_NATIVE_PLATFORM=_EGL_PLATFORM_X11)   #used for egldisplay.c (_eglGetNativePlatform)
#add_definitions(-DHAVE_DRI3)

add_library(EGL SHARED
#add_executable(EGL #used for debug
        #c heards
        EGL/include/c11/threads.h
        EGL/include/c11/threads_posix.h
        EGL/include/c99_alloca.h
        EGL/include/c99_compat.h
        EGL/include/c99_math.h
        EGL/include/no_extern_c.h
        EGL/include/GL
        EGL/include/GL/mesa_glinterop.h
        EGL/include/util
        EGL/include/util/macros.h
        EGL/include/util/u_atomic.h
        EGL/include/EGL
        EGL/include/EGL/egl.h
        EGL/include/EGL/eglext.h
        EGL/include/EGL/eglextchromium.h
        EGL/include/EGL/eglmesaext.h
        EGL/include/EGL/eglplatform.h
        EGL/EGL_config.h



        #c file
        EGL/drivers/dri2/egl_dri2.c
        EGL/drivers/dri2/egl_dri2.h
        EGL/drivers/dri2/egl_dri2_fallbacks.h
        EGL/drivers/dri2/platform_x11.c
        EGL/main/eglapi.c
        EGL/main/eglapi.h
        EGL/main/eglarray.c
        EGL/main/eglarray.h
        EGL/main/eglcompiler.h
        EGL/main/eglconfig.c
        EGL/main/eglconfig.h
        EGL/main/eglcontext.c
        EGL/main/eglcontext.h
        EGL/main/eglcurrent.c
        EGL/main/eglcurrent.h
        EGL/main/egldefines.h
        EGL/main/egldisplay.c
        EGL/main/egldisplay.h
        EGL/main/egldriver.c
        EGL/main/egldriver.h
        EGL/main/eglfallbacks.c
        EGL/main/eglglobals.c
        EGL/main/eglglobals.h
        EGL/main/eglimage.c
        EGL/main/eglimage.h
        EGL/main/egllog.c
        EGL/main/egllog.h
        EGL/main/eglsurface.c
        EGL/main/eglsurface.h
        EGL/main/eglsync.c
        EGL/main/eglsync.h
        EGL/main/egltypedefs.h

        )
target_link_libraries(EGL pthread xcb xcb-dri2 dl xcb-xfixes X11-xcb)