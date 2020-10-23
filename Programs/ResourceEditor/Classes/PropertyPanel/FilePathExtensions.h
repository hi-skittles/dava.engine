#pragma once

#include <TArc/Core/ContextAccessor.h>
#include <Reflection/ReflectedMeta.h>

void InitFilePathExtensions(DAVA::ContextAccessor* accessor);

DAVA::M::Validator CreateHeightMapValidator();
DAVA::M::Validator CreateTextureValidator(bool bindToScenePath = true);
DAVA::M::Validator CreateImageValidator();
DAVA::M::Validator CreateSceneValidator();
DAVA::M::Validator CreateExistsFile();

class REFileMeta : public DAVA::Metas::File
{
public:
    REFileMeta(const DAVA::String& filters, const DAVA::String& dlgTitle);

    DAVA::String GetDefaultPath() const override;
    DAVA::String GetRootDirectory() const override;
};

class HeightMapFileMeta : public REFileMeta
{
public:
    HeightMapFileMeta(const DAVA::String& filters);
};

class TextureFileMeta : public REFileMeta
{
public:
    TextureFileMeta(const DAVA::String& filters);
};

class ImageFileMeta : public REFileMeta
{
public:
    ImageFileMeta(const DAVA::String& filters);
};

class SceneFileMeta : public REFileMeta
{
public:
    SceneFileMeta(const DAVA::String& filters);
};

template <typename T>
using GenericFileMeta = DAVA::Meta<T, DAVA::Metas::File>;

GenericFileMeta<HeightMapFileMeta> CreateHeightMapFileMeta();
GenericFileMeta<TextureFileMeta> CreateTextureFileMeta();
GenericFileMeta<ImageFileMeta> CreateImageFileMeta();
GenericFileMeta<SceneFileMeta> CreateSceneFileMeta();
