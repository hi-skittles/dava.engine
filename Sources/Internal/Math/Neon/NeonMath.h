/*
 NEON math library for the iPhone / iPod touch
 
 Copyright (c) 2009 Justin Saunders
 
 This software is provided 'as-is', without any express or implied warranty.
 In no event will the authors be held liable for any damages arising
 from the use of this software.
 Permission is granted to anyone to use this software for any purpose,
 including commercial applications, and to alter it and redistribute it freely,
 subject to the following restrictions:
 
 1. The origin of this software must not be misrepresented; you must
 not claim that you wrote the original software. If you use this
 software in a product, an acknowledgment in the product documentation
 would be appreciated but is not required.
 
 2. Altered source versions must be plainly marked as such, and must
 not be misrepresented as being the original software.
 
 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef __NEON_MATRIX_IMPL_H__
#define __NEON_MATRIX_IMPL_H__

#include "Base/BaseTypes.h"

#ifdef __arm__
    #if defined(__DAVAENGINE_IPHONE__)
        #include "arm/arch.h"
        #ifdef _ARM_ARCH_7
            #define __DAVAENGINE_ARM_7__
        #endif //#ifdef _ARM_ARCH_7
    #endif //#if defined(__DAVAENGINE_IPHONE__)

    #if defined(__DAVAENGINE_ANDROID__)
        #include <machine/cpu-features.h>
        #if __ARM_ARCH__ == 7
            #ifdef USE_NEON
                #define __DAVAENGINE_ARM_7__
            #endif
        #endif //#if _ARM_ARCH_ == 7
    #endif //#if defined(__DAVAENGINE_ANDROID__)
#endif //#ifdef __arm__

// Matrixes are assumed to be stored in column major format according to OpenGL
// specification.

namespace DAVA
{
// Multiplies two 4x4 matrices (a,b) outputing a 4x4 matrix (output)
void NEON_Matrix4Mul(const float* a, const float* b, float* output);

// Multiplies a 4x4 matrix (m) with a vector 4 (v), outputing a vector 4
void NEON_Matrix4Vector4Mul(const float* m, const float* v, float* output);
};

#endif // __NEON_MATRIX_IMPL_H__