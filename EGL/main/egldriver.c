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
 * Functions for choosing and opening/loading device drivers.
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "c11/threads.h"
#include <EGL/egl.h>

#include "egldefines.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "egllog.h"

typedef struct _egl_module {
   char *Name;
   _EGLMain_t BuiltIn;
   _EGLDriver *Driver;
} _EGLModule;

static mtx_t _eglModuleMutex = _MTX_INITIALIZER_NP;
static _EGLArray *_eglModules;

const struct {
   const char *name;
   _EGLMain_t main;
} _eglBuiltInDrivers[] = {
#ifdef _EGL_BUILT_IN_DRIVER_DRI2
   { "egl_dri2", _eglBuiltInDriverDRI2 },
#endif
#ifdef _EGL_BUILT_IN_DRIVER_HAIKU
   { "egl_haiku", _eglBuiltInDriverHaiku },
#endif
   { NULL, NULL }
};

/**
 * Load a module and create the driver object.
 */
static EGLBoolean
_eglLoadModule(_EGLModule *mod)
{
   _EGLDriver *drv;

   if (mod->Driver)
      return EGL_TRUE;

   if (!mod->BuiltIn)
         return EGL_FALSE;

   drv = mod->BuiltIn(NULL);
   if (!drv || !drv->Name)
      return EGL_FALSE;

   mod->Driver = drv;

   return EGL_TRUE;
}


/**
 * Unload a module.
 */
static void
_eglUnloadModule(_EGLModule *mod)
{
   /* destroy the driver */
   if (mod->Driver && mod->Driver->Unload)
      mod->Driver->Unload(mod->Driver);

   mod->Driver = NULL;
}


/**
 * Add a module to the module array.
 */
static _EGLModule *
_eglAddModule(const char *name)//name："egl_dri2"
{
   _EGLModule *mod;
   EGLint i;

   if (!_eglModules) {//空就创建一个新的
      _eglModules = _eglCreateArray("Module", 8);//name，length个_EGLArray
      if (!_eglModules)
         return NULL;//还是空就NULL。。。
   }

   /* find duplicates */
   for (i = 0; i < _eglModules->Size; i++) {
      mod = _eglModules->Elements[i];
      if (strcmp(mod->Name, name) == 0)
         return mod;//str cmp :比较字符串，相同返回0
   }///好像8个都不一样

   /* allocate a new one */
   mod = calloc(1, sizeof(*mod));
   if (mod) {///创建成功执行以下代码
      mod->Name = strdup(name);//str dup 复制字符串
      if (!mod->Name) {///Name为空执行以下
         free(mod);
         mod = NULL;
      }
   }
   if (mod) {
      _eglAppendArray(_eglModules, (void *) mod);//添加模块到_eglModules
      _eglLog(_EGL_DEBUG, "added %s to module array", mod->Name);
   }                          //egl_dri2

   return mod;
}


/**
 * Free a module.
 */
static void
_eglFreeModule(void *module)
{
   _EGLModule *mod = (_EGLModule *) module;

   _eglUnloadModule(mod);
   free(mod->Name);
   free(mod);
}


/**
 * Add the user driver to the module array.
 *
 * The user driver is specified by EGL_DRIVER.
 */
static EGLBoolean
_eglAddUserDriver(void)
{
   char *env;

   env = getenv("EGL_DRIVER");
   if (env) {///env不为空时执行↓
      EGLint i;
                // "egl_dri2"
      for (i = 0; _eglBuiltInDrivers[i].name; i++) {
         if (!strcmp(_eglBuiltInDrivers[i].name, env)) {
            _EGLModule *mod = _eglAddModule(env);
            if (mod)
               mod->BuiltIn = _eglBuiltInDrivers[i].main;

            return EGL_TRUE;
         }
      }
   }

   return EGL_FALSE;
}


/**
 * Add built-in drivers to the module array.
 */
static void
_eglAddBuiltInDrivers(void)
{
   _EGLModule *mod;
   EGLint i;
               //name："egl_dri2"
   for (i = 0; _eglBuiltInDrivers[i].name; i++) {
      mod = _eglAddModule(_eglBuiltInDrivers[i].name);
      if (mod)//如果mod不是空指针，执行以下代码
         mod->BuiltIn = _eglBuiltInDrivers[i].main;
   }
}


/**
 * Add drivers to the module array.  Drivers will be loaded as they are matched
 * to displays.
 */
static EGLBoolean
_eglAddDrivers(void)
{
   if (_eglModules)///这东西存在直接返回true
      return EGL_TRUE;

   if (!_eglAddUserDriver()) {///一般执行以下代码（不人工指定驱动）
      /*
       * Add other drivers only when EGL_DRIVER is not set.  The order here
       * decides the priorities.
       */
      _eglAddBuiltInDrivers();///一般添加 egl_dri2
   }

   return (EGLBoolean)(_eglModules != NULL);//_eglModules存在(添加成功)，返回true
}


/**
 * A helper function for _eglMatchDriver.  It finds the first driver that can
 * initialize the display and return.
 */
static _EGLDriver *
_eglMatchAndInitialize(_EGLDisplay *dpy)
{
   _EGLDriver *drv = NULL;
   EGLint i = 0;

   if (!_eglAddDrivers()) {///添加失败执行以下代码
      _eglLog(_EGL_WARNING, "failed to find any driver");
      return NULL;
   }

   if (dpy->Driver) {//第一次启动为空指针，不执行以下代码,貌似不执行。。
      drv = dpy->Driver;
      /* no re-matching? */
      if (!drv->API.Initialize(drv, dpy))
         drv = NULL;
      return drv;
   }

   while (i < _eglModules->Size) {
      _EGLModule *mod = (_EGLModule *) _eglModules->Elements[i];

      if (!_eglLoadModule(mod)) {
         /* remove invalid modules */
         _eglEraseArray(_eglModules, i, _eglFreeModule);
         continue;
      }

      //载入DRI2驱动...
      if (mod->Driver->API.Initialize(mod->Driver, dpy)) {
         drv = mod->Driver;
         break;
      }
      else {
         i++;
      }
   }

   return drv;
}


/**
 * Match a display to a driver.  The display is initialized unless test_only is
 * true.  The matching is done by finding the first driver that can initialize
 * the display.
 */
_EGLDriver *
_eglMatchDriver(_EGLDisplay *dpy, EGLBoolean test_only)
{
   _EGLDriver *best_drv;

   assert(!dpy->Initialized);//空指针直接退出

   mtx_lock(&_eglModuleMutex);

   /* set options */
   dpy->Options.TestOnly = test_only;//false
   dpy->Options.UseFallback = EGL_FALSE;

   best_drv = _eglMatchAndInitialize(dpy);//通过display匹配驱动
   if (!best_drv) {
      dpy->Options.UseFallback = EGL_TRUE;
      best_drv = _eglMatchAndInitialize(dpy);
   }

   mtx_unlock(&_eglModuleMutex);

   if (best_drv) {
      _eglLog(_EGL_DEBUG, "the best driver is %s%s",
            best_drv->Name, (test_only) ? " (test only) " : "");
      if (!test_only) {
         dpy->Driver = best_drv;
         dpy->Initialized = EGL_TRUE;
      }
   }

   return best_drv;
}


__eglMustCastToProperFunctionPointerType
_eglGetDriverProc(const char *procname)
{
   EGLint i;
   _EGLProc proc = NULL;

   if (!_eglModules) {
      /* load the driver for the default display */
      EGLDisplay egldpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
      _EGLDisplay *dpy = _eglLookupDisplay(egldpy);
      if (!dpy || !_eglMatchDriver(dpy, EGL_TRUE))
         return NULL;
   }

   for (i = 0; i < _eglModules->Size; i++) {
      _EGLModule *mod = (_EGLModule *) _eglModules->Elements[i];

      if (!mod->Driver)
         break;
      proc = mod->Driver->API.GetProcAddress(mod->Driver, procname);
      if (proc)
         break;
   }

   return proc;
}


/**
 * Unload all drivers.
 */
void
_eglUnloadDrivers(void)
{
   /* this is called at atexit time */
   if (_eglModules) {
      _eglDestroyArray(_eglModules, _eglFreeModule);
      _eglModules = NULL;
   }
}
