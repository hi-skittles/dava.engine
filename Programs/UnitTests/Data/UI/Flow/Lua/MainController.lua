local dataRef = nil
local first = true

function init(context)
    dataRef = context.data
    dataRef.model = {
        a = 1,
        b = "demo",
        c = true
    }
    dataRef.testStep = dataRef.testStep + 1
end

function loadResources(context, view)
    dataRef.testStep = dataRef.testStep + 1
end

function activate(context, view)
    dataRef.testStep = dataRef.testStep + 1
end

function process(delta)
    if first then
        dataRef.testStep = dataRef.testStep + 1
        first = false
    end
end

function processEvent(event)
    if event == "INC_A" then
        dataRef.model.a = dataRef.model.a + 1
        dataRef.testStep = dataRef.testStep + 1
        return true
    end
    return false
end

function deactivate(context, view)
    dataRef.testStep = dataRef.testStep + 1
end

function unloadResources(context, view)
    dataRef.testStep = dataRef.testStep + 1
end

function release(context)
    dataRef.testStep = dataRef.testStep + 1
    context.data.model = nil
end