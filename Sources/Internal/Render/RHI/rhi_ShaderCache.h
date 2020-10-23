#ifndef __RHI_SHADERCACHE_H__
#define __RHI_SHADERCACHE_H__

 #include "rhi_Type.h"

namespace rhi
{
typedef bool (*ShaderBuilder)(Api targetApi, ProgType progType, const char* uid, const char* srcText, std::vector<uint8>* bin);

namespace ShaderCache
{
bool Initialize(ShaderBuilder builder = nullptr);
void Unitialize();

void Clear();
void Load(const char* binFileName);

const std::vector<uint8>& GetProg(const DAVA::FastName& uid);
void UpdateProg(Api targetApi, ProgType progType, const DAVA::FastName& uid, const char* srcText);
void UpdateProgBinary(Api targetApi, ProgType progType, const DAVA::FastName& uid, const void* bin, unsigned binSize);

} // namespace ShaderCache
} // namespace rhi

#endif // __RHI_SHADERCACHE_H__
