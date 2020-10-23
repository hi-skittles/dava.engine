#pragma once

#include <Math/Vector.h>

class PackageNode;
class ControlNode;

namespace ControlPlacementUtils
{
void SetAbsoulutePosToControlNode(PackageNode* package, ControlNode* node, ControlNode* dstNode, const DAVA::Vector2& pos);
}
