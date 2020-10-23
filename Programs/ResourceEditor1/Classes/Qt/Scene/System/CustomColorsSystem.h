#pragma once

#include "Command/Command.h"
#include "LandscapeEditorSystem.h"
#include "LandscapeEditorDrawSystem.h"

class CustomColorsSystem : public LandscapeEditorSystem
{
public:
    CustomColorsSystem(DAVA::Scene* scene);
    ~CustomColorsSystem() override;

    LandscapeEditorDrawSystem::eErrorType EnableLandscapeEditing();
    bool DisableLandscapeEdititing(bool saveNeeded = true);

    void PrepareForRemove() override
    {
    }
    void Process(DAVA::float32 timeElapsed) override;
    bool Input(DAVA::UIEvent* event) override;
    void InputCancelled(DAVA::UIEvent* event) override;

    void SetBrushSize(DAVA::int32 brushSize, bool updateDrawSystem = true);
    DAVA::int32 GetBrushSize();
    void SetColor(DAVA::int32 colorIndex);
    DAVA::int32 GetColor();

    void SaveTexture(); // with current of default generated path
    void SaveTexture(const DAVA::FilePath& filePath);
    bool LoadTexture(const DAVA::FilePath& filePath, bool createUndo);
    DAVA::FilePath GetCurrentSaveFileName();

    bool ChangesPresent();

private:
    bool CouldApplyImage(DAVA::Image* image, const DAVA::String& imageName) const;

    void UpdateToolImage(bool force = false);
    void UpdateBrushTool();
    void CreateToolImage(const DAVA::FilePath& filePath);

    void AddRectToAccumulator(const DAVA::Rect& rect);
    void ResetAccumulatorRect();
    DAVA::Rect GetUpdatedRect();

    void StoreOriginalState();

    void StoreSaveFileName(const DAVA::FilePath& filePath);

    DAVA::FilePath GetScenePath();
    DAVA::String GetRelativePathToScenePath(const DAVA::FilePath& absolutePath);
    DAVA::FilePath GetAbsolutePathFromScenePath(const DAVA::String& relativePath);
    DAVA::String GetRelativePathToProjectPath(const DAVA::FilePath& absolutePath);
    DAVA::FilePath GetAbsolutePathFromProjectPath(const DAVA::String& relativePath);

    void FinishEditing(bool applyModification);

    std::unique_ptr<DAVA::Command> CreateSaveFileNameCommand(const DAVA::String& filePath);

private:
    DAVA::Texture* toolImageTexture = nullptr;
    DAVA::Texture* loadedTexture = nullptr;
    DAVA::Image* originalImage = nullptr;
    DAVA::Color drawColor = DAVA::Color::Transparent;
    DAVA::int32 colorIndex = 0;
    DAVA::int32 curToolSize = 120;
    DAVA::Rect updatedRectAccumulator;
    bool editingIsEnabled = false;
};
