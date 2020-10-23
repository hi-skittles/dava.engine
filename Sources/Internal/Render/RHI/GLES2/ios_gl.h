#ifndef __DAVAENGINE_IOSGL_H__
#define __DAVAENGINE_IOSGL_H__

void ios_gl_init(void* nativeLayer);
bool ios_gl_check_layer();
void ios_gl_begin_frame();
void ios_gl_reset(void* nativeLayer, GLint width, GLint height);
void ios_gl_end_frame();
void ios_gl_acquire_context();
void ios_gl_release_context();
void ios_gl_resolve_multisampling(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                                  GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                                  GLbitfield mask, GLenum filter);
void ios_gl_discard_framebuffer(bool discardColor, bool discardDepthStencil);

#endif // __DAVAENGINE_IOSGL_H__
