#include "Classes/UI/Find/Filters/AnchorsAndSizePoliciesConflictFilter.h"

AnchorsSizePoliciesConflictFilter::AnchorsSizePoliciesConflictFilter()
    : horizontalSizePolicyHolder("horizontalPolicy")
    , verticalSizePolicyHolder("verticalPolicy")
    , anchorsEnabledHolder("enabled")
    , leftAnchorEnabledHolder("leftAnchorEnabled")
    , hCenterAnchorEnabledHolder("hCenterAnchorEnabled")
    , rightAnchorEnabledHolder("rightAnchorEnabled")
    , topAnchorEnabledHolder("topAnchorEnabled")
    , vCenterAnchorEnabledHolder("vCenterAnchorEnabled")
    , bottomAnchorEnabledHolder("bottomAnchorEnabled")
{
}

FindFilter::ePackageStatus AnchorsSizePoliciesConflictFilter::AcceptPackage(const PackageInformation* package) const
{
    return PACKAGE_CAN_ACCEPT_CONTROLS;
}

bool AnchorsSizePoliciesConflictFilter::AcceptControl(const ControlInformation* control) const
{
    using namespace DAVA;

    UISizePolicyComponent::eSizePolicy hSizePolicy = horizontalSizePolicyHolder.Get(control, UISizePolicyComponent::IGNORE_SIZE);
    UISizePolicyComponent::eSizePolicy vSizePolicy = verticalSizePolicyHolder.Get(control, UISizePolicyComponent::IGNORE_SIZE);

    bool anchorsEnabled = anchorsEnabledHolder.Get(control, true);
    bool leftAnchorEnabled = leftAnchorEnabledHolder.Get(control, false);
    bool hCenterAnchorEnabled = hCenterAnchorEnabledHolder.Get(control, false);
    bool rightAnchorEnabled = rightAnchorEnabledHolder.Get(control, false);
    bool topAnchorEnabled = topAnchorEnabledHolder.Get(control, false);
    bool vCenterAnchorEnabled = vCenterAnchorEnabledHolder.Get(control, false);
    bool bottomAnchorEnabled = bottomAnchorEnabledHolder.Get(control, false);

    if (anchorsEnabled == false)
    {
        return false;
    }

    if (hSizePolicy != UISizePolicyComponent::IGNORE_SIZE &&
        ((leftAnchorEnabled && hCenterAnchorEnabled)
         || (leftAnchorEnabled && rightAnchorEnabled)
         || (hCenterAnchorEnabled && rightAnchorEnabled)))
    {
        return true;
    }

    if (vSizePolicy != UISizePolicyComponent::IGNORE_SIZE &&
        ((topAnchorEnabled && vCenterAnchorEnabled)
         || (topAnchorEnabled && bottomAnchorEnabled)
         || (vCenterAnchorEnabled && bottomAnchorEnabled)))
    {
        return true;
    }

    return false;
}
