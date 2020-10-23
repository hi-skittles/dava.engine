#pragma once

#include "Classes/UI/Find/Filters/FindFilter.h"
#include "Classes/UI/Find/Filters/FieldHolder.h"
#include "Classes/UI/Find/PackageInformation/ControlInformation.h"

#include <UI/Layouts/UISizePolicyComponent.h>
#include <UI/Layouts/UIAnchorComponent.h>

class AnchorsSizePoliciesConflictFilter : public FindFilter
{
public:
    AnchorsSizePoliciesConflictFilter();

private:
    FindFilter::ePackageStatus AcceptPackage(const PackageInformation* package) const override;
    bool AcceptControl(const ControlInformation* control) const override;

    FieldHolder<DAVA::UISizePolicyComponent, DAVA::UISizePolicyComponent::eSizePolicy> horizontalSizePolicyHolder;
    FieldHolder<DAVA::UISizePolicyComponent, DAVA::UISizePolicyComponent::eSizePolicy> verticalSizePolicyHolder;

    FieldHolder<DAVA::UIAnchorComponent, bool> anchorsEnabledHolder;
    FieldHolder<DAVA::UIAnchorComponent, bool> leftAnchorEnabledHolder;
    FieldHolder<DAVA::UIAnchorComponent, bool> hCenterAnchorEnabledHolder;
    FieldHolder<DAVA::UIAnchorComponent, bool> rightAnchorEnabledHolder;
    FieldHolder<DAVA::UIAnchorComponent, bool> topAnchorEnabledHolder;
    FieldHolder<DAVA::UIAnchorComponent, bool> vCenterAnchorEnabledHolder;
    FieldHolder<DAVA::UIAnchorComponent, bool> bottomAnchorEnabledHolder;
};