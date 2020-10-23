#include "Scene/System/HoodSystem/NormalHood.h"
#include "Scene/System/ModifSystem.h"
#include "Scene/System/TextDrawSystem.h"

// framework
#include "Render/RenderHelper.h"

NormalHood::NormalHood()
    : HoodObject(2.0f)
{
    axisX = CreateLine(DAVA::Vector3(0, 0, 0), DAVA::Vector3(baseSize, 0, 0));
    axisX->axis = ST_AXIS_X;

    axisY = CreateLine(DAVA::Vector3(0, 0, 0), DAVA::Vector3(0, baseSize, 0));
    axisY->axis = ST_AXIS_Y;

    axisZ = CreateLine(DAVA::Vector3(0, 0, 0), DAVA::Vector3(0, 0, baseSize));
    axisZ->axis = ST_AXIS_Z;
}

NormalHood::~NormalHood()
{
}

void NormalHood::Draw(ST_Axis selectedAxis, ST_Axis mouseOverAxis, DAVA::RenderHelper* drawer, TextDrawSystem* textDrawSystem)
{
    // x
    drawer->DrawLine(axisX->curFrom, axisX->curTo, colorX, DAVA::RenderHelper::DRAW_WIRE_NO_DEPTH);

    // y
    drawer->DrawLine(axisY->curFrom, axisY->curTo, colorY, DAVA::RenderHelper::DRAW_WIRE_NO_DEPTH);

    // z
    drawer->DrawLine(axisZ->curFrom, axisZ->curTo, colorZ, DAVA::RenderHelper::DRAW_WIRE_NO_DEPTH);

    DrawAxisText(textDrawSystem, axisX, axisY, axisZ);
}
