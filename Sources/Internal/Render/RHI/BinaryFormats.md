
## VertexLayout
    
    VertexLayout
    {
        element_count   [UI4],
        element
        {
            usage       [UI1],
            usage_index [UI1],
            data_type   [UI1],
            data_count  [UI1],
        } element_count
    }
    


## ShaderSource
    
    ShaderSource
    {
        progType            [UI4],
        code                [S0],
    
        vertexLayout        [VertexLayout],
    
        propCount           [UI4]
        property
        {
            uid             [S0],
            tag             [S0],
            type            [UI4],
            storage         [UI4],
            isBigArray      [UI4],
            arraySize       [UI4],
            bufIndex        [UI4],
            bufReg          [UI4],
            bufRegCount     [UI4],
            defaultValue    [FP4]
        } propCount,
    
        bufferCount         [UI4],
        buffer
        {
            storage         [UI4],
            tag             [S0],
            regCount        [UI4]
        } bufferCount,
    
        samplerCount        [UI4],
        sampler
        {
            type            [UI4],
            uid             [S0]
        } samplerCount,
    
        blendState
        {
            colorFunc       [UI1],
            colorSrc        [UI1],
            colorDst        [UI1],
            alphaFunc       [UI1],
            alphaSrc        [UI1],
            alphaDst        [UI1],
            writeMask       [UI1],
            blendEnabled    [UI1],
            alphaToCoverage [UI1],
            pad             [UI3]
        }
    }
    


## CacheFile
    
    CacheFile
    {
        formatVersion       [UI4],
    
        sourceCount         [UI4],
        source
        {
            uid             [S0],
            srcHash         [UI4],
            source          [ShaderSource]
        } sourceCount
    }
    