/*==================================================================================
    Copyright (c) 2008, binaryzebra
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.
    * Neither the name of the binaryzebra nor the
    names of its contributors may be used to endorse or promote products
    derived from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE binaryzebra AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL binaryzebra BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
=====================================================================================*/

    #include "../Common/rhi_Private.h"
    #include "../Common/rhi_Pool.h"
    #include "rhi_Metal.h"

    #include "Debug/DVAssert.h"
    #include "Logger/Logger.h"
using DAVA::Logger;

    #include "_metal.h"

namespace rhi
{
//==============================================================================

static Handle metal_PerfQuery_Create()
{
    return InvalidHandle;
}

static void metal_PerfQuery_Delete(Handle handle)
{
}

static void metal_PerfQuery_Reset(Handle handle)
{
}

static bool metal_PerfQuery_IsReady(Handle handle)
{
    return true;
}

static uint64 metal_PerfQueryValue(Handle handle)
{
    return 0;
}

namespace PerfQueryMetal
{
void SetupDispatch(Dispatch* dispatch)
{
    dispatch->impl_PerfQuery_Create = &metal_PerfQuery_Create;
    dispatch->impl_PerfQuery_Delete = &metal_PerfQuery_Delete;
    dispatch->impl_PerfQuery_Reset = &metal_PerfQuery_Reset;
    dispatch->impl_PerfQuery_IsReady = &metal_PerfQuery_IsReady;
    dispatch->impl_PerfQuery_Value = &metal_PerfQueryValue;
}
}

//==============================================================================
} // namespace rhi
