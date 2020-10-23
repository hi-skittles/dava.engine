#pragma once

#include "Base/Platform.h"

#if defined(__DAVAENGINE_WIN32__)

namespace rhi
{
struct ResetParam;
struct InitParam;
}

void win_gl_reset(const rhi::ResetParam&);
void win32_gl_init(const rhi::InitParam& param);
void win32_gl_end_frame();
void win32_gl_acquire_context();
void win32_gl_release_context();

#endif
