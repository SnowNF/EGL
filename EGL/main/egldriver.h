/**************************************************************************
 *
 * Copyright 2008 VMware, Inc.
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
 * Copyright 2010-2011 LunarG, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef EGLDRIVER_INCLUDED
#define EGLDRIVER_INCLUDED


#include "c99_compat.h"

#include "egltypedefs.h"
#include "eglapi.h"
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Define an inline driver typecast function.
 *
 * Note that this macro defines a function and should not be ended with a
 * semicolon when used.
 */
#define _EGL_DRIVER_TYPECAST(drvtype, egltype, code)           \
   static inline struct drvtype *drvtype(const egltype *obj)   \
   { return (struct drvtype *) code; }


/**
 * Define the driver typecast functions for _EGLDriver, _EGLDisplay,
 * _EGLContext, _EGLSurface, and _EGLConfig.
 *
 * Note that this macro defines several functions and should not be ended with
 * a semicolon when used.
 */
#define _EGL_DRIVER_STANDARD_TYPECASTS(drvname)                            \
   _EGL_DRIVER_TYPECAST(drvname ## _driver, _EGLDriver, obj)               \
   /* note that this is not a direct cast
 *    只有dri2_egl_display是_EGLDisplay->DriverData,其余都是直接强转
 *    直接强转:_EGLDriver to
 * */                                                                      \
   _EGL_DRIVER_TYPECAST(drvname ## _display, _EGLDisplay, obj->DriverData) \
   _EGL_DRIVER_TYPECAST(drvname ## _context, _EGLContext, obj)             \
   _EGL_DRIVER_TYPECAST(drvname ## _surface, _EGLSurface, obj)             \
   _EGL_DRIVER_TYPECAST(drvname ## _config, _EGLConfig, obj)


typedef _EGLDriver *(*_EGLMain_t)(const char *args);


/**
 * Base class for device drivers.
 */
//初始化于： egl_dri2.c ； _eglBuiltInDriverDRI2
//随着父结构体（dri2_driver)一起被初始化
//传入base小指针，转换成原来的大指针struct dri2_egl_driver)
struct _egl_driver  //_EGLDriver
{

    // "DRI2"
    //初始化于： egl_dri2.c ： _eglBuiltInDriverDRI2
   const char *Name;  /**< name of this driver */

   /**
    * Release the driver resource.
    *
    * It is called before dlclose().
    */

    //初始化于： egl_dri2.c ： _eglBuiltInDriverDRI2
    //指向： egl_dri2.c ： dri2_unload()
   void (*Unload)(_EGLDriver *drv);

   //指向一大堆API(eglapi.h)
   _EGLAPI API;  /**< EGL API dispatch table */
};


extern _EGLDriver *
_eglBuiltInDriverDRI2(const char *args);


extern _EGLDriver*
_eglBuiltInDriverHaiku(const char* args);


extern _EGLDriver *
_eglMatchDriver(_EGLDisplay *dpy, EGLBoolean test_only);


extern __eglMustCastToProperFunctionPointerType
_eglGetDriverProc(const char *procname);


extern void
_eglUnloadDrivers(void);


/* defined in eglfallbacks.c */
extern void
_eglInitDriverFallbacks(_EGLDriver *drv);


extern void
_eglSearchPathForEach(EGLBoolean (*callback)(const char *, size_t, void *),
                      void *callback_data);


#ifdef __cplusplus
}
#endif


#endif /* EGLDRIVER_INCLUDED */
