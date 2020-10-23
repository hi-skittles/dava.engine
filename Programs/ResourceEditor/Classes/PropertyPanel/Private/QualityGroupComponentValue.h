#pragma once

#include <TArc/Controls/PropertyPanel/BaseComponentValue.h>
#include <TArc/DataProcessing/DataWrappersProcessor.h>
#include <TArc/Controls/ControlProxy.h>

#include <Reflection/Reflection.h>
#include <Base/Any.h>
#include <Base/BaseTypes.h>
#include <Base/FastName.h>

class QWidget;
class QualityGroupComponentValue : public DAVA::BaseComponentValue
{
public:
    QualityGroupComponentValue();

    bool RepaintOnUpdateRequire() const override;

protected:
    DAVA::Any GetMultipleValue() const override;
    bool IsValidValueToSet(const DAVA::Any& newValue, const DAVA::Any& currentValue) const override;
    DAVA::ControlProxy* CreateEditorWidget(QWidget* parent, const DAVA::Reflection& model, DAVA::DataWrappersProcessor* wrappersProcessor) override;
    bool IsSpannedControl() const override;

private:
    DAVA::Any IsFilterByType() const;
    void SetFilterByType(const DAVA::Any& filtrate);

    size_t GetModelTypeIndex() const;
    void SetModelTypeIndex(size_t index);
    bool IsModelTypeReadOnly() const;

    size_t GetGroupIndex() const;
    void SetGroupIndex(size_t index);
    bool IsGroupReadOnly() const;

    size_t GetQualityIndex() const;
    void SetQualityIndex(size_t index);
    bool IsQualityReadOnly() const;

    const DAVA::Vector<DAVA::FastName>& GetFilters() const;
    const DAVA::Vector<DAVA::FastName>& GetGroups() const;
    const DAVA::Vector<DAVA::FastName>& GetQualities() const;

    DAVA::Vector<DAVA::FastName> modelTypes;
    DAVA::Vector<DAVA::FastName> modelTypesWithDifferent;
    DAVA::Vector<DAVA::FastName> groups;
    DAVA::Vector<DAVA::FastName> groupsWithDifferent;
    DAVA::UnorderedMap<DAVA::FastName, std::pair<DAVA::Vector<DAVA::FastName>, DAVA::Vector<DAVA::FastName>>> qualities;

    mutable bool isDifferentFilterByType = false;
    mutable bool isDifferentModelTypes = false;
    mutable bool isDifferentGroups = false;
    mutable bool isDifferentQualities = false;

    DAVA_VIRTUAL_REFLECTION(QualityGroupComponentValue, DAVA::BaseComponentValue);
};