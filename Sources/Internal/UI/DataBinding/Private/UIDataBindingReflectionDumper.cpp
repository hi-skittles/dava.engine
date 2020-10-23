#include "UI/DataBinding/UIDataBindingReflectionDumper.h"

#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlEmitter.h"
#include "Debug/DVAssert.h"
#include "Reflection/ReflectedMeta.h"
#include "Reflection/ReflectedTypeDB.h"
#include "Utils/StringFormat.h"

namespace DAVA
{
UIDataBindingReflectionDumper::UIDataBindingReflectionDumper()
{
}

UIDataBindingReflectionDumper::~UIDataBindingReflectionDumper()
{
}

void UIDataBindingReflectionDumper::DumpToFile(const Reflection& ref, const FilePath& path)
{
    stream.clear();

    Vector<Reflection::Field> fields = ref.GetFields();
    AddFields(fields, true, 0);
    WriteToFile(path);
}

void UIDataBindingReflectionDumper::AddDescription(const String& name, const Reflection& ref, int32 indent)
{
    Vector<Reflection::Field> fields = ref.GetFields();
    if (!name.empty())
    {
        stream << name << " = ";
    }

    bool isVector = ref.GetFieldsCaps().hasFlatStruct && ref.GetFieldsCaps().hasRangeAccess;
    stream << (isVector ? "[" : "{");

    if (!fields.empty())
    {
        stream << std::endl;
        AddFields(fields, !isVector, indent + 2);
        for (int32 i = 0; i < indent; i++)
        {
            stream << " ";
        }
    }

    stream << (isVector ? "]" : "}");

    if (indent == 0)
    {
        stream << std::endl;
    }
}

void UIDataBindingReflectionDumper::AddFields(const Vector<Reflection::Field>& fields, bool map, int32 indent)
{
    for (const Reflection::Field& field : fields)
    {
        for (int32 i = 0; i < indent; i++)
        {
            stream << " ";
        }

        if (map)
        {
            if (field.key.CanCast<String>())
            {
                stream << field.key.Cast<String>();
            }
            else if (field.key.CanGet<int32>())
            {
                stream << field.key.Get<int32>();
            }
            else if (field.key.CanGet<uint32>())
            {
                stream << field.key.Get<uint32>() << "U";
            }
            else if (field.key.CanGet<uint64>())
            {
                stream << field.key.Get<uint64>() << "UL";
            }
            else if (field.key.CanGet<int64>())
            {
                stream << field.key.Get<int64>() << "L";
            }
            else if (field.key.CanGet<bool>())
            {
                stream << (field.key.Get<bool>() ? "true" : "false");
            }
            else
            {
                stream << "?";
                DVASSERT(false);
            }
            stream << " = ";
        }

        Any val = field.ref.GetValue();
        if (field.ref.GetMeta<M::HiddenField>() != nullptr)
        {
            stream << "nil";
        }
        else if (val.CanGet<int32>())
        {
            stream << val.Get<int32>();
        }
        else if (val.CanGet<uint32>())
        {
            stream << val.Get<uint32>() << "U";
        }
        else if (val.CanGet<uint64>())
        {
            stream << val.Get<uint64>() << "UL";
        }
        else if (val.CanGet<int64>())
        {
            stream << val.Get<int64>() << "L";
        }
        else if (val.CanGet<uint16>())
        {
            stream << static_cast<int32>(val.Get<uint16>());
        }
        else if (val.CanGet<int16>())
        {
            stream << static_cast<int32>(val.Get<int16>());
        }
        else if (val.CanGet<uint8>())
        {
            stream << static_cast<int32>(val.Get<uint8>());
        }
        else if (val.CanGet<int8>())
        {
            stream << static_cast<int32>(val.Get<int8>());
        }
        else if (val.CanGet<float32>())
        {
            stream << val.Get<float32>();
        }
        else if (val.CanGet<String>())
        {
            stream << "\"" << val.Get<String>() << "\"";
        }
        else if (val.CanGet<FilePath>())
        {
            stream << "\"" << val.Get<FilePath>().GetFrameworkPath() << "\"";
        }
        else if (val.CanGet<bool>())
        {
            stream << (val.Get<bool>() ? "true" : "false");
        }
        else if (field.ref.GetMeta<M::Enum>() != nullptr)
        {
            const M::Enum* meta = field.ref.GetMeta<M::Enum>();
            stream << "\"" << meta->GetEnumMap()->ToString(val.Cast<int32>()) << "\"";
        }
        else if (field.ref.GetMeta<M::Flags>() != nullptr)
        {
            stream << val.Cast<int32>();
        }
        else if (field.ref.HasFields() || field.ref.GetFieldsCaps().hasFlatStruct)
        {
            AddDescription("", field.ref, indent);
        }
        else
        {
            stream << "nil";
        }
        stream << ";";
        stream << std::endl;
    }
}

void UIDataBindingReflectionDumper::WriteToFile(const FilePath& path)
{
    ScopedPtr<File> outFile(File::Create(path, File::CREATE | File::WRITE));
    if (outFile.get())
    {
        outFile->WriteString(stream.str());
    }
}
}
