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


#ifndef EGLAPI_INCLUDED
#define EGLAPI_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif

/**
 * A generic function ptr type
 */
typedef void (*_EGLProc)(void);

struct wl_display;
struct mesa_glinterop_device_info;
struct mesa_glinterop_export_in;
struct mesa_glinterop_export_out;

/**
 * The API dispatcher jumps through these functions
 */
struct _egl_api
{
   /* driver funcs */
   //egl_dri2.c : dri2_initialize
   EGLBoolean (*Initialize)(_EGLDriver *, _EGLDisplay *dpy);
   //egl_dri2.c : dri2_terminate
   EGLBoolean (*Terminate)(_EGLDriver *, _EGLDisplay *dpy);

   /* config funcs */
   //eglconfig.c : _eglGetConfigs
   EGLBoolean (*GetConfigs)(_EGLDriver *drv, _EGLDisplay *dpy,
                            EGLConfig *configs, EGLint config_size,
                            EGLint *num_config);

   //eglconfig.c : _eglChooseConfig
   EGLBoolean (*ChooseConfig)(_EGLDriver *drv, _EGLDisplay *dpy,
                              const EGLint *attrib_list, EGLConfig *configs,
                              EGLint config_size, EGLint *num_config);

   //eglconfig.c : _eglGetConfigs
   EGLBoolean (*GetConfigAttrib)(_EGLDriver *drv, _EGLDisplay *dpy,
                                 _EGLConfig *config, EGLint attribute,
                                 EGLint *value);

   /* context funcs */
   //egl_dri2.c : dri2_create_context
   _EGLContext *(*CreateContext)(_EGLDriver *drv, _EGLDisplay *dpy,
                                 _EGLConfig *config, _EGLContext *share_list,
                                 const EGLint *attrib_list);
   //egl_dri2.c : dri2_destroy_context
   EGLBoolean (*DestroyContext)(_EGLDriver *drv, _EGLDisplay *dpy,
                                _EGLContext *ctx);


   /* this is the only function (other than Initialize) that may be called
    * with an uninitialized display
    */
   //egl_dri2.c : dri2_make_current
   EGLBoolean (*MakeCurrent)(_EGLDriver *drv, _EGLDisplay *dpy,
                             _EGLSurface *draw, _EGLSurface *read,
                             _EGLContext *ctx);
   //eglcontext.c : _eglQueryContext
   EGLBoolean (*QueryContext)(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLContext *ctx, EGLint attribute,
                              EGLint *value);

   /* surface funcs */
   //egl_dri2.c : dri2_create_window_surface
   _EGLSurface *(*CreateWindowSurface)(_EGLDriver *drv, _EGLDisplay *dpy,
                                       _EGLConfig *config, void *native_window,
                                       const EGLint *attrib_list);
   //egl_dri2.c : dri2_create_pixmap_surface
   _EGLSurface *(*CreatePixmapSurface)(_EGLDriver *drv, _EGLDisplay *dpy,
                                       _EGLConfig *config, void *native_pixmap,
                                       const EGLint *attrib_list);
    //egl_dri2.c : dri2_create_pbuffer_surface
   _EGLSurface *(*CreatePbufferSurface)(_EGLDriver *drv, _EGLDisplay *dpy,
                                        _EGLConfig *config,
                                        const EGLint *attrib_list);
    //egl_dri2.c : dri2_destroy_surface
   EGLBoolean (*DestroySurface)(_EGLDriver *drv, _EGLDisplay *dpy,
                                _EGLSurface *surface);
    //egl_dri2.c : dri2_query_surface
   EGLBoolean (*QuerySurface)(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLSurface *surface, EGLint attribute,
                              EGLint *value);
    //eglsurface.c : _eglSurfaceAttrib
   EGLBoolean (*SurfaceAttrib)(_EGLDriver *drv, _EGLDisplay *dpy,
                               _EGLSurface *surface, EGLint attribute,
                               EGLint value);

   //egl_dri2.c : dri2_bind_tex_image
   EGLBoolean (*BindTexImage)(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLSurface *surface, EGLint buffer);
   //egl_dri2.c : dri2_release_tex_image
   EGLBoolean (*ReleaseTexImage)(_EGLDriver *drv, _EGLDisplay *dpy,
                                 _EGLSurface *surface, EGLint buffer);
   //egl_dri2.c : dri2_swap_interval
   EGLBoolean (*SwapInterval)(_EGLDriver *drv, _EGLDisplay *dpy,
                              _EGLSurface *surf, EGLint interval);
   //egl_dri2.c : dri2_swap_buffers
   EGLBoolean (*SwapBuffers)(_EGLDriver *drv, _EGLDisplay *dpy,
                             _EGLSurface *draw);
   //egl_dri2.c : dri2_copy_buffers
   EGLBoolean (*CopyBuffers)(_EGLDriver *drv, _EGLDisplay *dpy,
                             _EGLSurface *surface, void *native_pixmap_target);

   /* misc functions */
    //egl_dri2.c : dri2_wait_client
   EGLBoolean (*WaitClient)(_EGLDriver *drv, _EGLDisplay *dpy,
                            _EGLContext *ctx);
    //egl_dri2.c : dri2_wait_native
   EGLBoolean (*WaitNative)(_EGLDriver *drv, _EGLDisplay *dpy,
                            EGLint engine);

   /* this function may be called from multiple threads at the same time */
    //egl_dri2.c : dri2_get_proc_address
   _EGLProc (*GetProcAddress)(_EGLDriver *drv, const char *procname);

    //eglfallbacks.c : _eglReturnFalse
   _EGLSurface *(*CreatePbufferFromClientBuffer)(_EGLDriver *drv,
                                                 _EGLDisplay *dpy,
                                                 EGLenum buftype,
                                                 EGLClientBuffer buffer,
                                                 _EGLConfig *config,
                                                 const EGLint *attrib_list);

    //egl_dri2.c : dri2_create_image
   _EGLImage *(*CreateImageKHR)(_EGLDriver *drv, _EGLDisplay *dpy,
                                _EGLContext *ctx, EGLenum target,
                                EGLClientBuffer buffer,
                                const EGLint *attr_list);
    //egl_dri2.c : dri2_destroy_image_khr
   EGLBoolean (*DestroyImageKHR)(_EGLDriver *drv, _EGLDisplay *dpy,
                                 _EGLImage *image);

    //egl_dri2.c : dri2_create_sync
   _EGLSync *(*CreateSyncKHR)(_EGLDriver *drv, _EGLDisplay *dpy, EGLenum type,
                              const EGLAttrib *attrib_list);
    //egl_dri2.c : dri2_destroy_sync
   EGLBoolean (*DestroySyncKHR)(_EGLDriver *drv, _EGLDisplay *dpy,
                                _EGLSync *sync);
    //egl_dri2.c : dri2_client_wait_sync
   EGLint (*ClientWaitSyncKHR)(_EGLDriver *drv, _EGLDisplay *dpy,
                               _EGLSync *sync, EGLint flags, EGLTime timeout);
    //egl_dri2.c : dri2_server_wait_sync
   EGLint (*WaitSyncKHR)(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSync *sync);

    //egl_dri2.c : dri2_signal_sync
   EGLBoolean (*SignalSyncKHR)(_EGLDriver *drv, _EGLDisplay *dpy,
                               _EGLSync *sync, EGLenum mode);
    //eglsync.c : _eglGetSyncAttrib
   EGLBoolean (*GetSyncAttrib)(_EGLDriver *drv, _EGLDisplay *dpy,
                               _EGLSync *sync, EGLint attribute,
                               EGLAttrib *value);
    //egl_dri2.c : dri2_swap_buffers_region
   EGLBoolean (*SwapBuffersRegionNOK)(_EGLDriver *drv, _EGLDisplay *disp,
                                      _EGLSurface *surf, EGLint numRects,
                                      const EGLint *rects);
    //eglfallbacks.c : NULL
   _EGLImage *(*CreateDRMImageMESA)(_EGLDriver *drv, _EGLDisplay *disp,
                                    const EGLint *attr_list);
    //eglfallbacks.c : NULL
   EGLBoolean (*ExportDRMImageMESA)(_EGLDriver *drv, _EGLDisplay *disp,
                                    _EGLImage *img, EGLint *name,
                                    EGLint *handle, EGLint *stride);
    //NULL : NULL
   EGLBoolean (*BindWaylandDisplayWL)(_EGLDriver *drv, _EGLDisplay *disp,
                                      struct wl_display *display);
    //NULL : NULL
   EGLBoolean (*UnbindWaylandDisplayWL)(_EGLDriver *drv, _EGLDisplay *disp,
                                        struct wl_display *display);
    //NULL : NULL
   EGLBoolean (*QueryWaylandBufferWL)(_EGLDriver *drv, _EGLDisplay *displ,
                                      struct wl_resource *buffer,
                                      EGLint attribute, EGLint *value);

    //egl_dri2.c : dri2_create_wayland_buffer_from_image
   struct wl_buffer *(*CreateWaylandBufferFromImageWL)(_EGLDriver *drv,
                                                       _EGLDisplay *disp,
                                                       _EGLImage *img);

    //egl_dri2.c : dri2_swap_buffers_with_damage
   EGLBoolean (*SwapBuffersWithDamageEXT)(_EGLDriver *drv, _EGLDisplay *dpy,
                                          _EGLSurface *surface,
                                          const EGLint *rects, EGLint n_rects);

    //egl_dri2.c : dri2_post_sub_buffer
   EGLBoolean (*PostSubBufferNV)(_EGLDriver *drv, _EGLDisplay *disp,
                                 _EGLSurface *surface, EGLint x, EGLint y,
                                 EGLint width, EGLint height);

    //egl_dri2.c : dri2_query_buffer_age
   EGLint (*QueryBufferAge)(_EGLDriver *drv,
                            _EGLDisplay *dpy, _EGLSurface *surface);
    //egl_dri2.c : dri2_get_sync_values_chromium
   EGLBoolean (*GetSyncValuesCHROMIUM)(_EGLDisplay *dpy, _EGLSurface *surface,
                                       EGLuint64KHR *ust, EGLuint64KHR *msc,
                                       EGLuint64KHR *sbc);

    //eglfallbacks.c : NULL
   EGLBoolean (*ExportDMABUFImageQueryMESA)(_EGLDriver *drv, _EGLDisplay *disp,
                                            _EGLImage *img, EGLint *fourcc,
                                            EGLint *nplanes,
                                            EGLuint64KHR *modifiers);
    //eglfallbacks.c : NULL
   EGLBoolean (*ExportDMABUFImageMESA)(_EGLDriver *drv, _EGLDisplay *disp,
                                       _EGLImage *img, EGLint *fds,
                                       EGLint *strides, EGLint *offsets);

    //egl_dri2.c : dri2_interop_query_device_info
   int (*GLInteropQueryDeviceInfo)(_EGLDisplay *dpy, _EGLContext *ctx,
                                   struct mesa_glinterop_device_info *out);
    //egl_dri2.c : dri2_interop_export_object
   int (*GLInteropExportObject)(_EGLDisplay *dpy, _EGLContext *ctx,
                                struct mesa_glinterop_export_in *in,
                                struct mesa_glinterop_export_out *out);
};

EGLint _eglConvertIntsToAttribs(const EGLint *int_list,
                                EGLAttrib **out_attrib_list);

#ifdef __cplusplus
}
#endif

#endif /* EGLAPI_INCLUDED */
