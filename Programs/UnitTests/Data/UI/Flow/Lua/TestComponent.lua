-- Base interface of UI controller
-- All interface functions are OPTIONAL

local step = 0

-- Called after create controller
function init(controlRef, componentRef)
    DV.Debug("init")    
    controlRef.GetComponentByName("UITextComponent", 0).text = "TextFromLua:init"
end

-- Called before destroy controller
function release(controlRef, componentRef)
    DV.Debug("release")    
    controlRef.GetComponentByName("UITextComponent", 0).text = "TextFromLua:release"
end

-- Called when parameters changed controller
function parametersChanged(controlRef, componentRef)
    DV.Debug("parametersChanged")    
    controlRef.GetComponentByName("UITextComponent", 0).text = "TextFromLua:parametersChanged:" .. componentRef.parameters
end

-- Called each frame
function process(controlRef, componentRef, frameDelta)
    DV.Debug("process")    
    if step == 1 then
        controlRef.GetComponentByName("UITextComponent", 0).text = "TextFromLua:process:" .. step
    end
    step = step + 1
end

-- Called on each dropped event
-- Should return true if event should been dispatched otherwise false
function processEvent(controlRef, componentRef, event, ...)
    DV.Debug("processEvent")    
    controlRef.GetComponentByName("UITextComponent", 0).text = "TextFromLua:processEvent:" .. event
    return true
end