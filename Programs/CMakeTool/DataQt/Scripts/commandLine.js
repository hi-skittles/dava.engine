function createOutput(configuration, fileSystemHelper, sourcePath, buildPath, cmakePath, davaPath, customOptions) {
    var outputText = ""; //pass by reference

    if(configuration["currentPlatform"] === undefined) {
        throw qsTr("current platform must be specified!");
    }
    var index = configuration["currentPlatform"];
    var platformObject = configuration["platforms"][index];
    outputText += platformObject.value;
    if(!buildPath || buildPath.length === 0) {
        throw qsTr("build path required");
    }

    outputText += " -B" + buildPath;

    var substrings = [];
    var defaults = platformObject.defaults;
    var localOptionsObject = configuration["currentOptions"];
    if(defaults && Array.isArray(defaults)) {
        if(defaults.length > localOptionsObject.count) {
            throw qsTr("Internal error: local options size less than default substrings size!");
        }
        for(var i = 0, length = defaults.length; i < length; ++i) {
            substrings[i] = defaults[i];
        }

        for(var i = 0, length = localOptionsObject.length; i < length; ++i) {
            var localOption = localOptionsObject[i];
            substrings[localOption.index] = localOption.value;
        }
    }
    for(var i = 0, length = substrings.length; i < length; ++i) {
        outputText = outputText.arg(substrings[i])
    }
    
    var globalOptions = configuration.checkedGlobalOptions;
    if(globalOptions && Array.isArray(globalOptions)) {
        for(var i = 0, length = globalOptions.length; i < length; ++i) {
            outputText += " " + globalOptions[i].value;
        }
    }
    
    if(outputText.indexOf("$CMAKE_PATH") !== -1) {
        if(!fileSystemHelper.IsFileExists(cmakePath)) {
            throw qsTr("cmake path required");
        } else {
            outputText = outputText.replace(/\$CMAKE_PATH/g, cmakePath);
        }
    }

    if(outputText.indexOf("$SOURCE_PATH") !== -1) {
        if(!fileSystemHelper.IsDirExists(sourcePath)) {
            throw qsTr("source path required")
        } else {
            outputText = outputText.replace(/\$SOURCE_PATH/g, sourcePath)
        }
    }
    if(outputText.indexOf("$DAVA_FRAMEWORK_PATH") !== -1) {
        if(!fileSystemHelper.IsDirExists(davaPath)) {
            throw qsTr("DAVA folder path required");
        } else {
            outputText = outputText.replace(/\$DAVA_FRAMEWORK_PATH/g, davaPath)
        }
    }
    
    if(typeof customOptions === "string") {
        outputText += " " + customOptions;
    }
    return outputText;
}
