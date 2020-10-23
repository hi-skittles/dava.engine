## Animation Clip File
## 'S0' means null-terminated string aligned by zeros

    FileHeader
	{
        signature   U4,
        version     U4,
        crc32       U4,
        dataSize    U4,
	}
    
    AnimationClip
    {
        duration        F4,

        node_count      U4,    
        node
        {
            UID         S0,
            name        S0,
            data        Track
        } [node_count]
        
        mark_count      U4,
        mark
        {
            markID      S0,
            timestamp   F4,
        } [mark_count]
        
    }
    
## Track Data

    Track
    {
        signature       U4,

        channel_count   U4,
        channels
        {
            target      U1,
            pad         U1[3],
            data        Channel
        } [channel_count]
    }
    
## Channel Data

    Channel
    {
        signature           U4
        dimension           U1,
        interpolation       U1,
        compression         U2,

        key_count           U4,
        keys[key_count]
        {
            time            F4,
            data            F4[dim]
            intrpl_meta     F4  *optional. for bezier interpolation*
        }
    }
