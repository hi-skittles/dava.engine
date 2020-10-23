#pragma once

#include "Reflection/Reflection.h"
#include "FileSystem/FilePath.h"
#include <sstream>

namespace DAVA
{
class UIDataBindingReflectionDumper final
{
public:
    UIDataBindingReflectionDumper();
    ~UIDataBindingReflectionDumper();

    void DumpToFile(const Reflection& ref, const FilePath& path);

private:
    void AddDescription(const String& name, const Reflection& ref, int32 indent = 0);
    void AddFields(const Vector<Reflection::Field>& fields, bool map, int32 indent);

    void WriteToFile(const FilePath& path);

    std::stringstream stream;
};
}
