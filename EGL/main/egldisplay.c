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


/**
 * Functions related to EGLDisplay.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "c11/threads.h"

#include "eglcontext.h"
#include "eglcurrent.h"
#include "eglsurface.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "egllog.h"
#include "eglimage.h"
#include "eglsync.h"

/* Includes for _eglNativePlatformDetectNativeDisplay */
#ifdef HAVE_MINCORE
#include <unistd.h>
#include <sys/mman.h>
#endif
#ifdef HAVE_WAYLAND_PLATFORM
#include <wayland-client.h>
#endif
#ifdef HAVE_DRM_PLATFORM
#include <gbm.h>
#endif


/**
 * Map --with-egl-platforms names to platform types.
 */
static const struct {
   _EGLPlatformType platform;
   const char *name;
} egl_platforms[_EGL_NUM_PLATFORMS] = {
   { _EGL_PLATFORM_X11, "x11" },
   { _EGL_PLATFORM_WAYLAND, "wayland" },
   { _EGL_PLATFORM_DRM, "drm" },
   { _EGL_PLATFORM_ANDROID, "android" },
   { _EGL_PLATFORM_HAIKU, "haiku" },
   { _EGL_PLATFORM_SURFACELESS, "surfaceless" },
};


/**
 * Return the native platform by parsing EGL_PLATFORM.
 */
static _EGLPlatformType
_eglGetNativePlatformFromEnv(void)
{
   _EGLPlatformType plat = _EGL_INVALID_PLATFORM;
   const char *plat_name;
   EGLint i;

   plat_name = getenv("EGL_PLATFORM");
   /* try deprecated env variable */
   if (!plat_name || !plat_name[0])
      plat_name = getenv("EGL_DISPLAY");
   if (!plat_name || !plat_name[0])
      return _EGL_INVALID_PLATFORM;

   for (i = 0; i < _EGL_NUM_PLATFORMS; i++) {
      if (strcmp(egl_platforms[i].name, plat_name) == 0) {
         plat = egl_platforms[i].platform;
         break;
      }
   }

   return plat;
}


/**
 * Perform validity checks on a generic pointer.
 */
static EGLBoolean
_eglPointerIsDereferencable(void *p)
{
#ifdef HAVE_MINCORE
   uintptr_t addr = (uintptr_t) p;
   unsigned char valid = 0;
   const long page_size = getpagesize();

   if (p == NULL)
      return EGL_FALSE;

   /* align addr to page_size */
   addr &= ~(page_size - 1);

   if (mincore((void *) addr, page_size, &valid) < 0) {
      _eglLog(_EGL_DEBUG, "mincore failed: %m");
      return EGL_FALSE;
   }

   return (valid & 0x01) == 0x01;
#else
   return (EGLBoolean )(p != NULL);
#endif
}


/**
 * Try detecting native platform with the help of native display characteristcs.
 */
static _EGLPlatformType
_eglNativePlatformDetectNativeDisplay(void *nativeDisplay)
{
   if (nativeDisplay == EGL_DEFAULT_DISPLAY)
      return _EGL_INVALID_PLATFORM;

   if (_eglPointerIsDereferencable(nativeDisplay)) {
      void *first_pointer = *(void **) nativeDisplay;

      _eglLog(_EGL_DEBUG,"Native display printer is a generic pointer. \
the first pointer is : %p",first_pointer);

      (void) first_pointer; /* silence unused var warning */

#ifdef HAVE_WAYLAND_PLATFORM
      /* wl_display is a wl_proxy, which is a wl_object.
       * wl_object's first element points to the interfacetype. */
      if (first_pointer == &wl_display_interface)
         return _EGL_PLATFORM_WAYLAND;
#endif

#ifdef HAVE_DRM_PLATFORM
      /* gbm has a pointer to its constructor as first element. */
      if (first_pointer == gbm_create_device)
         return _EGL_PLATFORM_DRM;
#endif

#ifdef HAVE_X11_PLATFORM
      /* If not matched to any other platform, fallback to x11. */
      return _EGL_PLATFORM_X11;
#endif

#ifdef HAVE_HAIKU_PLATFORM
	return _EGL_PLATFORM_HAIKU;
#endif
   }

   return _EGL_INVALID_PLATFORM;
}


/**
 * Return the native platform.  It is the platform of the EGL native types.
 */
_EGLPlatformType
_eglGetNativePlatform(void *nativeDisplay)
{
   static _EGLPlatformType native_platform;
   char *detection_method;

   native_platform = _eglGetNativePlatformFromEnv();
   detection_method = "environment overwrite";

   if (native_platform == _EGL_INVALID_PLATFORM) {
      native_platform = _eglNativePlatformDetectNativeDisplay(nativeDisplay);
      detection_method = "auto detected by application calls.";
   }

   if (native_platform == _EGL_INVALID_PLATFORM) {
      native_platform = _EGL_NATIVE_PLATFORM;  //_EGL_PLATFORM_X11
      detection_method = "build-time configuration";
   }

   _eglLog(_EGL_DEBUG, "Native platform type: %s (%s)",
           egl_platforms[native_platform].name, detection_method);

   return native_platform;
}


/**
 * Finish display management.
 */
void
_eglFiniDisplay(void)
{
   _EGLDisplay *dpyList, *dpy;

   /* atexit function is called with global mutex locked */
   dpyList = _eglGlobal.DisplayList;
   while (dpyList) {
      EGLint i;

      /* pop list head */
      dpy = dpyList;
      dpyList = dpyList->Next;

      for (i = 0; i < _EGL_NUM_RESOURCES; i++) {
         if (dpy->ResourceLists[i]) {
            _eglLog(_EGL_DEBUG, "Display %p is destroyed with resources", dpy);
            break;
         }
      }

      free(dpy);
   }
   _eglGlobal.DisplayList = NULL;
}


/**
 * Find the display corresponding to the specified native display, or create a
 * new one.
 */
_EGLDisplay *
_eglFindDisplay(_EGLPlatformType plat, void *plat_dpy)
{
   //plat : _EGL_PLATFORM_X11
   //plat_dpy : 0 或者 Window(Xlib.h) 的指针
   _EGLDisplay *dpy;

   if (plat == _EGL_INVALID_PLATFORM)
      return NULL;

   mtx_lock(_eglGlobal.Mutex);

   /* search the display list first */
   dpy = _eglGlobal.DisplayList;
   while (dpy) {//空指针是false..因此此时display结构体(链表)不为空.
                            //  ↓均满足
      if (dpy->Platform == plat && dpy->PlatformDisplay == plat_dpy)
         break;//如果有同样的display就停止检索，使用这个display。
      dpy = dpy->Next;//没有就指到下一个结构体（链表结构)
       //一直指到空指针（没有下一个结构体）while不满足。。。
   }

   /* create a new display */
   if (!dpy) {  //如果dpy是空指针就执行以下代码。
      _eglLog(_EGL_DEBUG,"create a new display with PlatformDisplay : %p",plat_dpy);
      dpy = calloc(1, sizeof(_EGLDisplay));//申请一个长度为_EGLDisplay的内存空间。
      if (dpy) {//不是空指针（申请成功）执行以下代码
         mtx_init(&dpy->Mutex, mtx_plain);
         dpy->Platform = plat;  //为新的结构体装东西: _EGL_PLATFORM_X11
         dpy->PlatformDisplay = plat_dpy;

         /* add to the display list */ 
         dpy->Next = _eglGlobal.DisplayList;//指向上一个？
         _eglGlobal.DisplayList = dpy;//替代原来上一个的位置
      } else{//在前面添加结构体型链表
          _eglLog(_EGL_FATAL,"Fail to create a new Display");
      }
   }

   mtx_unlock(_eglGlobal.Mutex);

   return dpy;//返回新的or相同的display
}


/**
 * Destroy the contexts and surfaces that are linked to the display.
 */
void
_eglReleaseDisplayResources(_EGLDriver *drv, _EGLDisplay *display)
{
   _EGLResource *list;

   list = display->ResourceLists[_EGL_RESOURCE_CONTEXT];
   while (list) {
      _EGLContext *ctx = (_EGLContext *) list;
      list = list->Next;

      _eglUnlinkContext(ctx);
      drv->API.DestroyContext(drv, display, ctx);//used
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_CONTEXT]);

   list = display->ResourceLists[_EGL_RESOURCE_SURFACE];
   while (list) {
      _EGLSurface *surf = (_EGLSurface *) list;
      list = list->Next;

      _eglUnlinkSurface(surf);
      drv->API.DestroySurface(drv, display, surf);//used
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_SURFACE]);

   //no imagine or sync was loaded
   /*
   list = display->ResourceLists[_EGL_RESOURCE_IMAGE];
   while (list) {
      _EGLImage *image = (_EGLImage *) list;
      list = list->Next;

      _eglUnlinkImage(image);
      drv->API.DestroyImageKHR(drv, display, image);//never used
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_IMAGE]);

   list = display->ResourceLists[_EGL_RESOURCE_SYNC];
   while (list) {
      _EGLSync *sync = (_EGLSync *) list;
      list = list->Next;

      _eglUnlinkSync(sync);
      drv->API.DestroySyncKHR(drv, display, sync);//never used
   }
   assert(!display->ResourceLists[_EGL_RESOURCE_SYNC]);*/
}


/**
 * Free all the data hanging of an _EGLDisplay object, but not
 * the object itself.
 */
void
_eglCleanupDisplay(_EGLDisplay *disp)
{
   if (disp->Configs) {
      _eglDestroyArray(disp->Configs, free);
      disp->Configs = NULL;
   }

   /* XXX incomplete */
}


/**
 * Return EGL_TRUE if the given handle is a valid handle to a display.
 */
EGLBoolean
_eglCheckDisplayHandle(EGLDisplay dpy)
{
   _EGLDisplay *cur;

   mtx_lock(_eglGlobal.Mutex);
   cur = _eglGlobal.DisplayList;
   while (cur) {
      if (cur == (_EGLDisplay *) dpy)
         break;//如果在链表中发现相同的就break,否则一直指到NULL
      cur = cur->Next;
   }
   mtx_unlock(_eglGlobal.Mutex);
   return (EGLBoolean)(cur != NULL);//cur为NULL返回false
}//有效返回TRUE


/**
 * Return EGL_TRUE if the given resource is valid.  That is, the display does
 * own the resource.
 */
EGLBoolean
_eglCheckResource(void *res, _EGLResourceType type, _EGLDisplay *dpy)
{
   _EGLResource *list = dpy->ResourceLists[type];
   
   if (!res)
      return EGL_FALSE;

   while (list) {
      if (res == (void *) list) {
         assert(list->Display == dpy);
         break;
      }
      list = list->Next;
   }

   return (EGLBoolean)(list != NULL);
}


/**
 * Initialize a display resource.  The size of the subclass object is
 * specified.
 *
 * This is supposed to be called from the initializers of subclasses, such as
 * _eglInitContext or _eglInitSurface.
 */
void
_eglInitResource(_EGLResource *res, EGLint size, _EGLDisplay *dpy)
{
   memset(res, 0, (size_t)size);
   res->Display = dpy;
   res->RefCount = 1;
}


/**
 * Increment reference count for the resource.
 */
void
_eglGetResource(_EGLResource *res)
{
   assert(res && res->RefCount > 0);
   /* hopefully a resource is always manipulated with its display locked */
   res->RefCount++;
}


/**
 * Decrement reference count for the resource.
 */
EGLBoolean
_eglPutResource(_EGLResource *res)
{
   assert(res && res->RefCount > 0);
   res->RefCount--;
   return (EGLBoolean)(!res->RefCount);
}


/**
 * Link a resource to its display.
 */
void
_eglLinkResource(_EGLResource *res, _EGLResourceType type)
{
   assert(res->Display);

   res->IsLinked = EGL_TRUE;
   res->Next = res->Display->ResourceLists[type];
   res->Display->ResourceLists[type] = res;
   _eglGetResource(res);
}


/**
 * Unlink a linked resource from its display.
 */
void
_eglUnlinkResource(_EGLResource *res, _EGLResourceType type)
{
   _EGLResource *prev;

   prev = res->Display->ResourceLists[type];
   if (prev != res) {
      while (prev) {
         if (prev->Next == res)
            break;
         prev = prev->Next;
      }
      assert(prev);
      prev->Next = res->Next;
   }
   else {
      res->Display->ResourceLists[type] = res->Next;
   }

   res->Next = NULL;
   res->IsLinked = EGL_FALSE;
   _eglPutResource(res);

   /* We always unlink before destroy.  The driver still owns a reference */
   assert(res->RefCount);
}

#ifdef HAVE_X11_PLATFORM
static EGLBoolean
_eglParseX11DisplayAttribList(const EGLint *attrib_list)
{
    //never used
    //FIXME: need more test.
    fprintf(stderr,"libEGL: deleted method : %s\n",__func__);/*
   int i;

   if (attrib_list == NULL) {
      return EGL_TRUE;
   }

   for (i = 0; attrib_list[i] != EGL_NONE; i += 2) {
      EGLint attrib = attrib_list[i];
      EGLint value = attrib_list[i + 1];

      /* EGL_EXT_platform_x11 recognizes exactly one attribute,
       * EGL_PLATFORM_X11_SCREEN_EXT, which is optional.
       * 
       * Mesa supports connecting to only the default screen, so we reject
       * screen != 0.
       *//*
      if (attrib != EGL_PLATFORM_X11_SCREEN_EXT || value != 0) {
         _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
         return EGL_FALSE;
      }
   }

   return EGL_TRUE;*/
    return EGL_FALSE;
}

_EGLDisplay*
_eglGetX11Display(Display *native_display,
                  const EGLint *attrib_list)
{    //never used
    //FIXME: need more test.
    fprintf(stderr,"libEGL: deleted method : %s\n",__func__);
    /*
   if (!_eglParseX11DisplayAttribList(attrib_list)) {
      return NULL;
   }

   return _eglFindDisplay(_EGL_PLATFORM_X11, native_display);*/
    return NULL;
}
#endif /* HAVE_X11_PLATFORM */

#ifdef HAVE_DRM_PLATFORM
_EGLDisplay*
_eglGetGbmDisplay(struct gbm_device *native_display,
                  const EGLint *attrib_list)
{
   /* EGL_MESA_platform_gbm recognizes no attributes. */
   if (attrib_list != NULL && attrib_list[0] != EGL_NONE) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
      return NULL;
   }

   return _eglFindDisplay(_EGL_PLATFORM_DRM, native_display);
}
#endif /* HAVE_DRM_PLATFORM */

#ifdef HAVE_WAYLAND_PLATFORM
_EGLDisplay*
_eglGetWaylandDisplay(struct wl_display *native_display,
                      const EGLint *attrib_list)
{
   /* EGL_EXT_platform_wayland recognizes no attributes. */
   if (attrib_list != NULL && attrib_list[0] != EGL_NONE) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
      return NULL;
   }

   return _eglFindDisplay(_EGL_PLATFORM_WAYLAND, native_display);
}
#endif /* HAVE_WAYLAND_PLATFORM */

#ifdef HAVE_SURFACELESS_PLATFORM
_EGLDisplay*
_eglGetSurfacelessDisplay(void *native_display,
                          const EGLint *attrib_list)
{
   /* This platform has no native display. */
   if (native_display != NULL) {
      _eglError(EGL_BAD_PARAMETER, "eglGetPlatformDisplay");
      return NULL;
   }

   /* This platform recognizes no display attributes. */
   if (attrib_list != NULL && attrib_list[0] != EGL_NONE) {
      _eglError(EGL_BAD_ATTRIBUTE, "eglGetPlatformDisplay");
      return NULL;
   }

   return _eglFindDisplay(_EGL_PLATFORM_SURFACELESS, native_display);
}
#endif /* HAVE_SURFACELESS_PLATFORM */
