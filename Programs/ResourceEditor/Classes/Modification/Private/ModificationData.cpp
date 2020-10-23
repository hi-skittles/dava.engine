#include "Classes/Modification/Private/ModificationData.h"

#include <Math/Transform.h>
#include <Math/TransformUtils.h>

const char* ModificationData::transformTypeField = "type";
const char* ModificationData::transformPivotField = "pivot";
const char* ModificationData::transformInLocalField = "transformInLocal";
const char* ModificationData::pivotModeField = "pivotMode";
const char* ModificationData::transformableSelectionField = "transformableSelection";
const char* ModificationData::scaleModeOffField = "scaleModeOff";
const char* ModificationData::manualModificationEnabledField = "isManualModificationEnabled";
const char* ModificationData::manualModificationArrowsEnabledField = "isManualModificationArrowsEnabled";
const char* ModificationData::modifyingSingleItemField = "isModifyingSingleItem";
const char* ModificationData::xLabelField = "xLabel";
const char* ModificationData::xValueField = "xValue";
const char* ModificationData::yValueField = "yValue";
const char* ModificationData::zValueField = "zValue";

bool ModificationData::GetScaleModeOff() const
{
    return GetTransformType() != DAVA::Selectable::TransformType::Scale;
}

bool ModificationData::GetManualModificationEnabled() const
{
    return (GetTransformableSelection().IsEmpty() == false) && (GetTransformType() != DAVA::Selectable::TransformType::Disabled);
}

bool ModificationData::GetManualModificationArrowsEnabled() const
{
    return !(GetTransformableSelection().GetSize() > 1 && GetPivotMode() == DAVA::EntityModificationSystem::PivotAbsolute);
}

bool ModificationData::GetModifyingSingleItem() const
{
    return (GetTransformableSelection().GetSize() <= 1) && (GetManualModificationEnabled() == true);
}

DAVA::String ModificationData::GetXLabelText() const
{
    static DAVA::String labelScale = "Scale:";
    static DAVA::String labelX = "X:";
    return (GetTransformType() == DAVA::Selectable::TransformType::Scale) ? labelScale : labelX;
}

DAVA::Any ModificationData::GetValueX()
{
    RecalculateValueFields();
    return xValue;
}

DAVA::Any ModificationData::GetValueY()
{
    RecalculateValueFields();
    return yValue;
}

DAVA::Any ModificationData::GetValueZ()
{
    RecalculateValueFields();
    return zValue;
}

void ModificationData::SetValueX(const DAVA::Any& value)
{
    if (value.CanCast<DAVA::float32>() == true)
    {
        xValue = value.Cast<DAVA::float32>();
        ApplyEnteredValues(DAVA::ST_AXIS_X);
    }
}

void ModificationData::SetValueY(const DAVA::Any& value)
{
    if (value.CanCast<DAVA::float32>() == true)
    {
        yValue = value.Cast<DAVA::float32>();
        ApplyEnteredValues(DAVA::ST_AXIS_Y);
    }
}

void ModificationData::SetValueZ(const DAVA::Any& value)
{
    if (value.CanCast<DAVA::float32>() == true)
    {
        zValue = value.Cast<DAVA::float32>();
        ApplyEnteredValues(DAVA::ST_AXIS_Z);
    }
}

void ModificationData::ApplyEnteredValues(DAVA::ST_Axis axis)
{
    DAVA::float32 x = xValue.Get<DAVA::float32>(0.f);
    DAVA::float32 y = yValue.Get<DAVA::float32>(0.f);
    DAVA::float32 z = zValue.Get<DAVA::float32>(0.f);
    DAVA::Vector3 values(x, y, z);
    DAVA::SelectableGroup selection = GetTransformableSelection();
    selection.RemoveObjectsWithDependantTransform();
    modificationSystem->ApplyValues(axis, selection, values);
}

void ModificationData::RecalculateValueFields()
{
    using namespace DAVA;

    if (GetManualModificationEnabled() == true)
    {
        if (GetPivotMode() == EntityModificationSystem::PivotRelative)
        {
            xValue = 0.f;
            yValue = 0.f;
            zValue = 0.f;
            return;
        }
        else
        {
            const SelectableGroup& selection = GetTransformableSelection();
            if (selection.GetSize() == 1)
            {
                const Selectable& selectedObject = selection.GetFirst();
                const DAVA::Transform& localTransform = selectedObject.GetLocalTransform();

                switch (GetTransformType())
                {
                case Selectable::TransformType::Translation:
                {
                    const DAVA::Vector3& translation = localTransform.GetTranslation();
                    xValue = translation.x;
                    yValue = translation.y;
                    zValue = translation.z;
                    break;
                }
                case Selectable::TransformType::Rotation:
                {
                    const DAVA::Vector3& rotate = localTransform.GetRotation().GetEuler();
                    float32 newXValue = DAVA::RadToDeg(rotate.x);
                    float32 newYValue = DAVA::RadToDeg(rotate.y);
                    float32 newZValue = DAVA::RadToDeg(rotate.z);

                    auto setValue = [](float32 newValue, Any& value)
                    {
                        if (value.CanGet<float32>())
                        {
                            static const DAVA::float32 eps = 0.001f;
                            float32 origValue = value.Get<float32>();
                            if (fabs(origValue - newValue) < eps)
                            {
                                return;
                            }
                            if ((180.f - fabs(origValue) < eps) && (180.f - fabs(newValue) < eps))
                            {
                                return;
                            }
                        }
                        value = newValue;
                    };

                    setValue(newXValue, xValue);
                    setValue(newYValue, yValue);
                    setValue(newZValue, zValue);
                    break;
                }
                case Selectable::TransformType::Scale:
                {
                    const DAVA::Vector3& scale = localTransform.GetScale();
                    xValue = scale.x;
                    yValue = scale.y;
                    zValue = scale.z;
                    break;
                }
                default:
                {
                    xValue = 0.f;
                    yValue = 0.f;
                    zValue = 0.f;
                    break;
                }
                }
                return;
            }
        }
    }

    // for all remaining cases
    xValue = String("");
    yValue = String("");
    zValue = String("");
}
