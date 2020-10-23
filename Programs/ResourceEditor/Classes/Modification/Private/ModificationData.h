#pragma once

#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/Scene/SceneTypes.h>
#include <REPlatform/Scene/Systems/ModifSystem.h>

#include <TArc/DataProcessing/TArcDataNode.h>

struct ModificationData : public DAVA::TArcDataNode
{
    DAVA::EntityModificationSystem* modificationSystem = nullptr;

    DAVA::Any xValue = 0.f;
    DAVA::Any yValue = 0.f;
    DAVA::Any zValue = 0.f;

    // modes
    static const char* transformTypeField;
    static const char* transformPivotField;
    static const char* transformInLocalField;
    static const char* pivotModeField;

    // calculated values
    static const char* transformableSelectionField;

    static const char* scaleModeOffField;
    static const char* manualModificationEnabledField;
    static const char* manualModificationArrowsEnabledField;
    static const char* modifyingSingleItemField;

    static const char* xLabelField;
    static const char* xValueField;
    static const char* yValueField;
    static const char* zValueField;

    DAVA::Selectable::TransformType GetTransformType() const
    {
        return modificationSystem->GetTransformType();
    }

    void SetTransformType(DAVA::Selectable::TransformType type)
    {
        modificationSystem->SetTransformType(type);
    }

    DAVA::Selectable::TransformPivot GetTransformPivot() const
    {
        return modificationSystem->GetPivotPoint();
    }

    void SetTransformPivot(DAVA::Selectable::TransformPivot pivot)
    {
        modificationSystem->SetPivotPoint(pivot);
    }

    bool GetTransformInLocalCoordinates() const
    {
        return modificationSystem->GetModifyInLocalCoordinates();
    }

    void SetTransformInLocalCoordinates(bool value)
    {
        return modificationSystem->SetModifyInLocalCoordinates(value);
    }

    DAVA::EntityModificationSystem::PivotMode GetPivotMode() const
    {
        return modificationSystem->GetPivotMode();
    }

    void SetPivotMode(DAVA::EntityModificationSystem::PivotMode value)
    {
        return modificationSystem->SetPivotMode(value);
    }

    const DAVA::SelectableGroup& GetTransformableSelection() const
    {
        return modificationSystem->GetTransformableSelection();
    }

    bool GetScaleModeOff() const;
    bool GetManualModificationEnabled() const;
    bool GetManualModificationArrowsEnabled() const;
    bool GetModifyingSingleItem() const;

    DAVA::String GetXLabelText() const;
    DAVA::Any GetValueX();
    DAVA::Any GetValueY();
    DAVA::Any GetValueZ();
    void SetValueX(const DAVA::Any& value);
    void SetValueY(const DAVA::Any& value);
    void SetValueZ(const DAVA::Any& value);

private:
    void ApplyEnteredValues(DAVA::ST_Axis axis);
    void RecalculateValueFields();

    DAVA_VIRTUAL_REFLECTION_IN_PLACE(ModificationData, DAVA::TArcDataNode)
    {
        DAVA::ReflectionRegistrator<ModificationData>::Begin()
        .Field(transformTypeField, &ModificationData::GetTransformType, &ModificationData::SetTransformType)
        .Field(transformPivotField, &ModificationData::GetTransformPivot, &ModificationData::SetTransformPivot)
        .Field(transformInLocalField, &ModificationData::GetTransformInLocalCoordinates, &ModificationData::SetTransformInLocalCoordinates)
        .Field(pivotModeField, &ModificationData::GetPivotMode, &ModificationData::SetPivotMode)
        .Field(transformableSelectionField, &ModificationData::GetTransformableSelection, nullptr)
        .Field(scaleModeOffField, &ModificationData::GetScaleModeOff, nullptr)
        .Field(manualModificationEnabledField, &ModificationData::GetManualModificationEnabled, nullptr)
        .Field(manualModificationArrowsEnabledField, &ModificationData::GetManualModificationArrowsEnabled, nullptr)
        .Field(modifyingSingleItemField, &ModificationData::GetModifyingSingleItem, nullptr)
        .Field(xLabelField, &ModificationData::GetXLabelText, nullptr)
        .Field(xValueField, &ModificationData::GetValueX, &ModificationData::SetValueX)
        .Field(yValueField, &ModificationData::GetValueY, &ModificationData::SetValueY)
        .Field(zValueField, &ModificationData::GetValueZ, &ModificationData::SetValueZ)
        .End();
    }
};
