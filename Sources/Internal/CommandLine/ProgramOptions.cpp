#include "CommandLine/ProgramOptions.h"

#include "FileSystem/FileSystem.h"

#include "Logger/Logger.h"
#include "Utils/Utils.h"

namespace DAVA
{
void ProgramOptions::Option::SetValue(const VariantType& value)
{
    if (multipleValuesSuported || values.empty())
    {
        values.push_back(value);
    }
    else
    {
        values[0] = value;
    }
}

ProgramOptions::ProgramOptions(const String& commandName_, bool parseByName_)
    : commandName(commandName_)
    , parseByName(parseByName_)
{
}

void ProgramOptions::AddOption(const String& optionName, const VariantType& defaultValue, const String& description, bool canBeMultiple)
{
    Option op;
    op.name = optionName;
    op.multipleValuesSuported = canBeMultiple;
    op.defaultValue = defaultValue;
    op.descr = description;

    options.push_back(op);
}

void ProgramOptions::AddArgument(const String& argumentName, bool required)
{
    Argument ar;
    ar.name = argumentName;
    ar.required = required;
    ar.set = false;
    arguments.push_back(ar);
}

bool ProgramOptions::Parse(uint32 argc, char* argv[])
{
    Vector<String> commandLine(argv, argv + argc);
    return Parse(commandLine);
}
bool ProgramOptions::Parse(const Vector<String>& argv)
{
    size_type argc = argv.size();

    // if first argument equal command name we should skip it else we should stop parsing
    size_type argIndex = 1; //skip executable pathname in params
    if (argIndex < argc && (parseByName == true && commandName == String(argv[argIndex])))
    {
        argIndex++;
    }
    else if (parseByName == true)
    {
        return false;
    }

    size_type curParamPos = 0;
    while (argIndex < argc)
    {
        // search if there is options with such name
        if (!ParseOption(argIndex, argv))
        {
            // set required
            if (curParamPos < arguments.size())
            {
                arguments[curParamPos].value = argv[argIndex];
                arguments[curParamPos].set = true;
                curParamPos++;
            }
            else if (argIndex < argc)
            {
                Logger::Error("Unknown argument: [%d] %s", argIndex, argv[argIndex].c_str());
                return false;
            }
            else
            {
                Logger::Error("Command line parsing error");
                return false;
            }
        }

        argIndex++;
    }

    // check if there is all required parameters
    for (auto& arg : arguments)
    {
        if (arg.required && !arg.set)
        {
            Logger::Error("Required argument is not specified: %s", arg.name.c_str());
            return false;
        }
    }

    return true;
}

bool ProgramOptions::ParseOption(size_type& argIndex, const Vector<String>& argv)
{
    size_type argc = argv.size();

    const String argString = argv[argIndex];
    for (auto& opt : options)
    {
        if (opt.name == argString)
        {
            if (opt.defaultValue.GetType() == VariantType::TYPE_BOOLEAN)
            {
                // bool option don't need any arguments
                opt.SetValue(VariantType(true));
                return true;
            }
            else
            {
                argIndex++;
                if (argIndex < argc)
                {
                    const String valueStr = argv[argIndex];
                    Vector<String> tokens; //one or more params

                    if (opt.multipleValuesSuported)
                    {
                        Split(valueStr, ",", tokens, false, false);
                    }
                    else
                    {
                        tokens.push_back(valueStr);
                    }

                    const VariantType::eVariantType optionType = opt.defaultValue.GetType();
                    switch (optionType)
                    {
                    case VariantType::TYPE_STRING:
                    case VariantType::TYPE_NONE:
                    {
                        for (auto& t : tokens)
                        {
                            opt.SetValue(VariantType(t));
                        }
                        break;
                    }
                    case VariantType::TYPE_INT32:
                    {
                        for (auto& t : tokens)
                        {
                            int32 value = 0;
                            if (1 == sscanf(t.c_str(), "%d", &value))
                            {
                                opt.SetValue(VariantType(value));
                            }
                        }
                        break;
                    }
                    case VariantType::TYPE_UINT32:
                    {
                        for (auto& t : tokens)
                        {
                            uint32 value = 0;
                            if (1 == sscanf(t.c_str(), "%u", &value))
                            {
                                opt.SetValue(VariantType(value));
                            }
                        }
                        break;
                    }
                    case VariantType::TYPE_UINT64:
                    {
                        for (auto& t : tokens)
                        {
                            uint64 value = 0;
                            if (1 == sscanf(t.c_str(), "%llu", &value))
                            {
                                opt.SetValue(VariantType(value));
                            }
                        }
                        break;
                    }
                    case VariantType::TYPE_BOOLEAN:
                    {
                        for (auto& t : tokens)
                        {
                            if (strcmp(t.c_str(), "true"))
                            {
                                opt.SetValue(VariantType(true));
                            }
                            else if (strcmp(t.c_str(), "false"))
                            {
                                opt.SetValue(VariantType(false));
                            }
                        }
                        break;
                    }
                    default:
                        DVASSERT(0 && "Not implemented");
                        break;
                    }

                    return true;
                }
                break;
            }
        }
    }

    return false;
}

String ProgramOptions::GetUsageString() const
{
    std::stringstream ss;
    ss << "  " << commandName << " ";

    if (options.size() > 0)
    {
        ss << "[options] ";
    }

    for (auto& arg : arguments)
    {
        if (arg.required)
        {
            ss << "<" << arg.name << "> ";
        }
        else
        {
            ss << "[" << arg.name << "] ";
        }
    }

    ss << std::endl;

    for (auto& opt : options)
    {
        ss << "\t" << opt.name;

        int optionType = opt.defaultValue.GetType();
        if (optionType != VariantType::TYPE_BOOLEAN)
        {
            ss << " <value>";
            if (opt.multipleValuesSuported)
            {
                ss << ",[additional values...]";
            }
            ss << "\t";
        }
        else
        {
            ss << "\t\t";
        }

        if (!opt.descr.empty())
        {
            ss << "- " << opt.descr;
        }

        ss << std::endl;
    }

    return ss.str();
}

uint32 ProgramOptions::GetOptionValuesCount(const String& optionName) const
{
    for (auto& opt : options)
    {
        if (opt.name == optionName)
        {
            uint32 count = static_cast<uint32>(opt.values.size());
            return (count > 0) ? count : 1; //real arguments or default
        }
    }

    return 1; //default
}

bool ProgramOptions::IsOptionExists(const String& optionName) const
{
    for (auto& opt : options)
    {
        if (opt.name == optionName)
        {
            return true;
        }
    }

    return false;
}

VariantType ProgramOptions::GetOption(const String& optionName, uint32 pos) const
{
    for (auto& opt : options)
    {
        if (opt.name == optionName)
        {
            const auto count = opt.values.size();
            if (count > 0)
            {
                DVASSERT(pos < opt.values.size());
                if (pos < opt.values.size())
                {
                    return opt.values[pos];
                }
            }

            return opt.defaultValue;
        }
    }

    return VariantType();
}

String ProgramOptions::GetArgument(const String& argumentName) const
{
    for (auto& arg : arguments)
    {
        if (arg.name == argumentName)
        {
            return arg.value;
        }
    }

    return String();
}

const String& ProgramOptions::GetCommand() const
{
    return commandName;
}

} //END of DAVA
