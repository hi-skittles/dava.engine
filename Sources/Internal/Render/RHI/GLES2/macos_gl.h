#if !defined __MACOS_GL_H__
#define __MACOS_GL_H__

#include "../rhi_Public.h"

void macos_gl_init(const rhi::InitParam&);
void macos_gl_reset(const rhi::ResetParam&);
void macos_gl_end_frame();
void macos_gl_acquire_context();
void macos_gl_release_context();

#endif
