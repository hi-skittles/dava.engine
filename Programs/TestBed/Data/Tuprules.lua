tup.davainclude("../../../Bin/lua/Tupdava.lua")

tupState = TupState.New({
    outputDir = "../.Assets",
    superpackDir = "../.AssetsSuperpack",
    intermediateDir = "../.tmp",
    superpack = true
})

tup.davainclude("Tuprules.inc.lua")

tupState:AddPacks {
    {
        name = "allpacks",
        depends = { "pack1", "pack2", "packgpu" }
    },
    {
        name = "pack0",
        is_base = true,
        rules = {
            {"Shaders", ".*"}
        }
    },
    {
        name = "pack1",
        is_base = true, -- is_base - only for local pack, not in superpack
        compression = "none",
        rules = {
            { "^$",  ".*" }
        },
    },
    lowercase_rule,
    {
        name = "packgpu",
        is_gpu = true,
        rules = {
            { "", ".*{gpu}.*"}
        }
    },
    {
        name = "pack2",
        rules = {
            { "", ".*" }
        },
    }
}
