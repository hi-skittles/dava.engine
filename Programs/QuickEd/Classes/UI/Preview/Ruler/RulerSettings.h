#pragma once

// Settings for the ruler widget.
struct RulerSettings
{
    int startPos = 0; // Start ruler position.
    int smallTicksDelta = 10; // Distance between "small" ticks.
    int bigTicksDelta = 50; // Distance between "big" ticks.

    float zoomLevel = 1.0f; // Zoom level for the current control.
};
