#pragma once

#include "Base/BaseTypes.h"
#include "FileSystem/VariantType.h"

namespace DAVA
{
class ProgramOptions final
{
public:
    ProgramOptions(const String& _commandName, bool parseByName = true);

    void AddOption(const String& optionName, const VariantType& defaultValue, const String& description = nullptr, bool canBeMultiple = false);
    void AddArgument(const String& argumentName, bool required = true);

    bool IsOptionExists(const String& optionName) const;
    uint32 GetOptionValuesCount(const String& optionName) const;
    VariantType GetOption(const String& optionName, uint32 pos = 0) const;

    String GetArgument(const String& argumentName) const;
    const String& GetCommand() const;

    bool Parse(uint32 argc, char* argv[]);
    bool Parse(const Vector<String>& commandLine);
    String GetUsageString() const;

private:
    bool ParseOption(size_type& argIndex, const Vector<String>& commandLine);

    struct Option
    {
        void SetValue(const VariantType& value);

        String name;
        String alias;
        String descr;
        bool multipleValuesSuported = false;
        VariantType defaultValue;
        Vector<VariantType> values;
    };

    struct Argument
    {
        bool required = false;
        bool set = false;
        String name;
        String value;
    };

    Vector<Argument> arguments;
    Vector<Option> options;

    String commandName;
    bool parseByName = true;
};

} //END of DAVA
