#pragma once

#include "Base/BaseTypes.h"
#include "Command/ICommand.h"
#include "Reflection/ReflectedType.h"
#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
class Command : public ICommand
{
public:
    /**
     \brief Creates instance of a command base class.
     \param[in] text command text description to be displayed in widgets / network packets / log texts.
     */
    Command(const String& description = String());

    /**
    \brief Returns command text description.
    \returns String command text description.
    */
    const String& GetDescription() const;

    /**
    \brief Some commands passed to stack can make Redo and Undo, but do not change any files so do not change save state.
    As an example it can be selection command or command which toggle view state in the editor;
    \returns true if command change save state aka modify any files or serializable objects.
    */
    virtual bool IsClean() const;

    template <typename T>
    T* Cast();

    template <typename T>
    const T* Cast() const;

private:
    //this function is not a part of public API and can be called only by CommandBatch
    virtual bool MergeWith(const Command* command);
    friend class CommandBatch;

    const String description;

    DAVA_VIRTUAL_REFLECTION(Command, ICommand);
};

inline const String& Command::GetDescription() const
{
    return description;
}

inline bool Command::IsClean() const
{
    return false;
}

inline bool Command::MergeWith(const Command* command)
{
    return false;
}

template <typename T>
T* Command::Cast()
{
    T* result = nullptr;
    void** outPrt = reinterpret_cast<void**>(&result);
    const ReflectedType* commandType = ReflectedTypeDB::GetByPointer(this);
    const ReflectedType* requireType = ReflectedTypeDB::Get<T>();
    DVASSERT(commandType != nullptr);
    DVASSERT(requireType != nullptr);
    if (TypeInheritance::Cast(commandType->GetType()->Pointer(), requireType->GetType()->Pointer(), this, outPrt) == false)
    {
        return nullptr;
    }

    return result;
}

template <typename T>
const T* Command::Cast() const
{
    T* result = nullptr;
    void** outPrt = reinterpret_cast<void**>(&result);
    const ReflectedType* commandType = ReflectedTypeDB::GetByPointer(this);
    const ReflectedType* requireType = ReflectedTypeDB::Get<T>();
    DVASSERT(commandType != nullptr);
    DVASSERT(requireType != nullptr);
    if (TypeInheritance::Cast(commandType->GetType()->Pointer(), requireType->GetType()->Pointer(), const_cast<Command*>(this), outPrt) == false)
    {
        return nullptr;
    }

    return const_cast<const T*>(result);
}
} //namespace DAVA
