#include "Classes/Utils/ControlPlacementUtils.h"

#include "Classes/Model/PackageHierarchy/ControlNode.h"
#include "Classes/Model/PackageHierarchy/PackageNode.h"
#include "Classes/Model/ControlProperties/RootProperty.h"

#include <Base/BaseTypes.h>
#include <Debug/DVAssert.h>
#include <UI/UIControl.h>

namespace ControlPlacementUtils
{
using namespace DAVA;

void SetAbsoulutePosToControlNode(PackageNode* package, ControlNode* node, ControlNode* dstNode, const DAVA::Vector2& pos)
{
    DVASSERT(nullptr != node);
    DVASSERT(nullptr != node->GetControl());
    DVASSERT(nullptr != dstNode);
    DVASSERT(nullptr != dstNode->GetControl());
    UIControl* parent = dstNode->GetControl();
    Vector2 sizeOffset = parent->GetSize() * parent->GetPivot();
    float32 angle = parent->GetAngle();
    UIGeometricData gd = parent->GetGeometricData();
    const Vector2& nodeSize = node->GetControl()->GetSize();
    sizeOffset -= nodeSize / 2;
    sizeOffset *= gd.scale;
    Vector2 controlPos = gd.position - DAVA::Rotate(sizeOffset, angle); //top left corner of dest control
    Vector2 relativePos = pos - controlPos; //new abs pos

    //now calculate new relative pos

    Vector2 scale = gd.scale;
    if (scale.x == 0.0f || scale.y == 0.0f)
    {
        relativePos.SetZero();
    }
    else
    {
        relativePos /= scale;
    }
    relativePos = DAVA::Rotate(relativePos, -angle);
    RootProperty* rootProperty = node->GetRootProperty();
    AbstractProperty* positionProperty = rootProperty->FindPropertyByName("position");
    DVASSERT(nullptr != positionProperty);
    Vector2 clampedRelativePos(std::floor(relativePos.x), std::floor(relativePos.y));
    package->SetControlProperty(node, positionProperty, clampedRelativePos);
}
}
