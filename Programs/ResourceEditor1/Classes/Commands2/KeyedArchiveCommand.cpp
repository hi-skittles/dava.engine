#include "Commands2/KeyedArchiveCommand.h"
#include "Commands2/RECommandIDs.h"

#include "FileSystem/KeyedArchive.h"

KeyedArchiveAddValueCommand::KeyedArchiveAddValueCommand(DAVA::KeyedArchive* _archive, const DAVA::String& _key, const DAVA::VariantType& _val)
    : RECommand(CMDID_KEYEDARCHIVE_ADD_KEY, "Add key to archive")
    , archive(_archive)
    , key(_key)
    , val(_val)
{
}

KeyedArchiveAddValueCommand::~KeyedArchiveAddValueCommand()
{
}

void KeyedArchiveAddValueCommand::Undo()
{
    if (NULL != archive)
    {
        archive->DeleteKey(key);
    }
}

void KeyedArchiveAddValueCommand::Redo()
{
    if (NULL != archive)
    {
        archive->SetVariant(key, val);
    }
}

KeyeadArchiveRemValueCommand::KeyeadArchiveRemValueCommand(DAVA::KeyedArchive* _archive, const DAVA::String& _key)
    : RECommand(CMDID_KEYEDARCHIVE_REM_KEY, "Rem key from archive")
    , archive(_archive)
    , key(_key)
{
    if (NULL != archive)
    {
        DAVA::VariantType* vPtr = archive->GetVariant(key);

        if (NULL != vPtr)
        {
            val = *vPtr;
        }
    }
}

KeyeadArchiveRemValueCommand::~KeyeadArchiveRemValueCommand()
{
}

void KeyeadArchiveRemValueCommand::Undo()
{
    if (NULL != archive)
    {
        archive->SetVariant(key, val);
    }
}

void KeyeadArchiveRemValueCommand::Redo()
{
    if (NULL != archive)
    {
        archive->DeleteKey(key);
    }
}

KeyeadArchiveSetValueCommand::KeyeadArchiveSetValueCommand(DAVA::KeyedArchive* _archive, const DAVA::String& _key, const DAVA::VariantType& _val)
    : RECommand(CMDID_KEYEDARCHIVE_SET_KEY, "Set archive value")
    , archive(_archive)
    , key(_key)
    , newVal(_val)
{
    if (NULL != archive)
    {
        oldVal = *archive->GetVariant(key);
    }
}

KeyeadArchiveSetValueCommand::~KeyeadArchiveSetValueCommand()
{
}

void KeyeadArchiveSetValueCommand::Undo()
{
    if (NULL != archive && archive->IsKeyExists(key))
    {
        archive->SetVariant(key, oldVal);
    }
}

void KeyeadArchiveSetValueCommand::Redo()
{
    if (NULL != archive && archive->IsKeyExists(key))
    {
        archive->SetVariant(key, newVal);
    }
}
