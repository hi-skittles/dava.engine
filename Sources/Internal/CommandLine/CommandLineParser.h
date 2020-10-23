#ifndef __DAVAENGINE_COMMANDLINEPARSER_H__
#define __DAVAENGINE_COMMANDLINEPARSER_H__

#include "Base/BaseTypes.h"
#include "Base/StaticSingleton.h"

namespace DAVA
{
class CommandLineParser : public StaticSingleton<CommandLineParser>
{
public:
    CommandLineParser();
    virtual ~CommandLineParser();

    void SetVerbose(bool isVerbose);
    bool GetVerbose() const;

    void SetExtendedOutput(bool isExO);
    bool IsExtendedOutput() const;

    void SetUseTeamcityOutput(bool use);
    bool UseTeamcityOutput() const;

    void SetFlags(const Vector<String>& arguments);
    void ClearFlags();

    bool IsFlagSet(const String& s) const;
    String GetParamForFlag(const String& flag);
    Vector<String> GetParamsForFlag(const String& flag);

    static int32 GetCommandsCount();

    static String GetCommand(uint32 commandPosition);
    static String GetCommandParam(const String& command);

    DAVA_DEPRECATED(static String GetCommandParamAdditional(const String& command, const int32 paramIndex)); //TODO: remove this method after fix of DF-1584

    static bool CommandIsFound(const String& command);

private:
    static int32 GetCommandPosition(const DAVA::String& command);

    struct Flag
    {
        explicit Flag(const String& f)
            : name(f)
        {
        }
        String name;
        Vector<String> params;
    };

    Vector<Flag> flags;
    bool isVerbose;
    bool isExtendedOutput;
    bool useTeamcityOutput;
};
}

#endif // __DAVAENGINE_COMMANDLINEPARSER_H__