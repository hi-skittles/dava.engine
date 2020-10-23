-- Base interface of UI controller
-- All interface functions are OPTIONAL

local speed = 1

function init(controlRef, componentRef)
    DV.Debug("init")
    parametersChanged(controlRef, componentRef);
end

function release(controlRef, componentRef)
    DV.Debug("release")   
    controlRef.angle = 0
end

function parametersChanged(controlRef, componentRef)
    if componentRef.parameters ~= "" then
        speed = tonumber(componentRef.parameters)
        DV.Debug("changed "..componentRef.parameters)
        if speed == nil then
            speed = 1
        end
    else
        speed = 1
        DV.Debug("changed *")  
    end
end

function process(controlRef, componentRef, frameDelta)
   controlRef.angle = controlRef.angle + frameDelta * 50 * speed
--    DV.Debug("process")   
end

function processEvent(controlRef, componentRef, event, ...)
    DV.Debug("processEvent: "..event)   
    controlRef.angle = 0
    return false
end
