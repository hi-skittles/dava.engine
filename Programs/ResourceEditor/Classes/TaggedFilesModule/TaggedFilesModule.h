#pragma once

#include <TArc/Core/ClientModule.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Utils/QtConnections.h>

#include <Base/Any.h>
#include <Reflection/Reflection.h>

class TaggedFilesModule : public DAVA::ClientModule
{
protected:
    void PostInit() override;

private:
    void OnConvertTaggedTextures();

    DAVA::QtConnections connections;
    DAVA_VIRTUAL_REFLECTION(TaggedFilesModule, DAVA::ClientModule);
};
