#pragma once

#include "Base/Singleton.h"
#include "Base/FastName.h"
#include "Render/Shader.h"

namespace DAVA
{
namespace ShaderDescriptorCache
{
void Initialize();
void Uninitialize();
void Clear();
void ClearDynamicBindigs();

void ReloadShaders();

void SetLoadingNotifyEnabled(bool enable);
ShaderDescriptor* GetShaderDescriptor(const FastName& name, const UnorderedMap<FastName, int32>& defines);
Vector<size_t> BuildFlagsKey(const FastName& name, const UnorderedMap<FastName, int32>& defines);
size_t GetUniqueFlagKey(FastName flagName);
};
};
