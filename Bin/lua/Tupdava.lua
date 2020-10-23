--[[ ==========================================================================
--
-- TODO:
-- usage description
--
-- ============================================================================ ]]



--[[ ==========================================================================
--
-- TupUtils
--
-- ============================================================================ ]]

--tup.initdava("debugger.lua")
--debugger = require()

if debugger == nil then
    debugger = function() end
end

function UtilDumpTable(table, indent)
    indent = indent or 0 
    for key, val in pairs(table) do
        formatting = string.rep("  ", indent) .. key .. ": "
        if type(val) == "table" then
            print(formatting)
            UtilDumpTable(val, indent + 1)
        elseif type(val) == "boolean" then
            print(formatting .. tostring(val))
        else
            print(formatting .. tostring(val))
        end
    end
end 

function UtilConvertToPlatformPath(platform, p)
    -- convert only unix pathes into win pathes
    if platform ~= "win32" then
        return p
    end
    
    if type(p) == "string" then
        -- if p is string just convert it 
        return p:gsub("/", "\\")        
    elseif type(p) == "table" then
        -- if p is table - convert evry
        -- string in that table
        for k,v in pairs(p) do
            if type(v) == "string" then
                p[k] = v:gsub("/", "\\")
            end
        end        
    end
end

function UtilIterateTable(table, count)
    local totalCount = #table
    local i = 0
    return function()
        if i < totalCount then
            local part = { }

            for j = 1, math.min(totalCount - i, count) do
                part[j] = table[i + j]
            end

            i = i + count
            return (i / count), part 
        end
    end
end

--[[ ==========================================================================
--
-- TupPack
--
-- ============================================================================ ]]

TupPack = {}
TupPack.__index = TupPack

function TupPack.New(params)
    local self = setmetatable({}, TupPack)
    self.rules = { } 

    TupPack.IsOkPackParams(params)
    gpu = gpu or ""   
        
    -- create default TupPack table
    self.name = params.name
    self.is_gpu = params.is_gpu or false
    self.is_base = params.is_base or false
    self.is_lowercase = params.is_lowercase or false
    self.depends = params.depends or { }
    self.compression = params.compression or "lz4hc"
    self.files = { }
    
    -- parse pack rules
    self:SetRules(params.rules or { })
    
    return self
end

function TupPack.IsOkPackParams(params)
    -- check params is a table
    if type(params) ~= "table" then
        error "Pack should be defined as table { name = name, rules = { rules table}, depends = { dependencies }}"
    end

    -- check that params have name entity
    if type(params.name) ~= "string" then
        error "Pack name should be a specified as string"
    end
    
    if params.is_gpu ~= nil and type(params.is_gpu) ~= "boolean" then
        error "Pack is_gpu flag should be specified as boolean true or false"
    end

    if params.is_lowercase ~= nil and type(params.is_lowercase) ~= "boolean" then
        error "Pack is_lowercase flag be specified as boolean true or false"
    end

    if params.is_base ~= nil and type(params.is_base) ~= "boolean" then
        error "Pack is_base flag should be specified as boolean true or false"
    end
    
    return true
end

function TupPack.SetRules(self, rules)
    -- check that rules are defined in table
    if type(rules) ~= "table" then
        error "Pack rules should be a table"
    end

    -- check that rules are well formated
    for k, v in pairs(rules) do
        -- key type is number
        if type(k) == "number" then
            if type(v) == "table" then
                if #v ~= 2 or type(v[1]) ~= "string" or type(v[2]) ~= "string" then
                    print("pack = " .. self.name .. ", rule #" .. k)
                    error "Pack rule # table should be defined as { 'dir pattern', 'file pattern' }"
                end
            elseif type(v) ~= "function" and type(v) ~= "string" then
                print("pack = " .. self.name .. ", rule #" .. k)
                error "Pack rule can be either string, table or function"
            end
        -- key type is string
        elseif type(k) == "string" then
            if k ~= "depends" then
                print("pack = " .. self.name .. ", k = " .. tostring(k))
                error "Pack dependencies should be declared with 'depends' key."
            end
        -- unknow key type
        else
            print(v)
            error "Unknown pack rule"
        end
    end
    
    -- assign rules
    self.rules = rules
end

function TupPack.Match(self, dir, file, gpuName, gpuVar, gpuPattern)
    local ret = false
    local full_path = file

    if dir ~= "." then
        full_path = dir .. "/" .. file
    else
        dir = ""
    end

    if self.is_lowercase then
        full_path = full_path:lower()
    end

    -- check for correct input arguments: gpuName    
    if gpuName == nil or type(gpuName) ~= "string" then
        error "No GPU name specified"
    end
    
    -- check for correct input arguments: gpuVar and gpuPattern    
    if self.is_gpu then
        if gpuVar == nil then
            error "No gpuVar specified"
        end

        if gpuPattern == nil then
            error "No gpuPattern specified"
        end
        
        -- escape all % symbols in pattern 
        gpuPattern = gpuPattern:gsub("%%", "%%%%")
    end
    
    -- each pack has multiple rules
    for ri, rule in pairs(self.rules) do

        -- dependency specification
        if ri == "depends" then

            -- nothing to do here

        -- when pack rule is a simple string
        -- we should just compare it to match
        elseif type(rule) == "string" then
        
            if self.is_gpu then
                rule = rule:gsub(gpuVar, gpuPattern)
            end

            if self.is_lowercase then
                rule = rule:lower()
            end            

            if full_path:match(rule) then
                ret = true
                break
            end

        -- when pack rule is a table
        -- we should check independent matching
        -- for directory and file
        elseif type(rule) == "table" then

            local dir_rule = rule[1]
            local file_rule = rule[2]
            
            if self.is_gpu then
                dir_rule = dir_rule:gsub(gpuVar, gpuPattern)
                file_rule = file_rule:gsub(gpuVar, gpuPattern)
            end

            if self.is_lowercase then
                dir_rule = dir_rule:lower()
                file_rule = file_rule:lower()
            end

            if nil ~= dir:match(dir_rule) and nil ~= file:match(file_rule) then
                ret = true
                --print("!" .. full_path .. " is mathing [" .. dir_rule .. ", " .. file_rule .. "]")
                break
            else
                --print(full_path .. " is not mathing [" .. dir_rule .. ", " .. file_rule .. "]")
            end

        -- when pack rule is a function
        -- we should call it check for return value
        elseif type(rule) == "function" then

            if rule(dir, file, gpuPattern) == true then
                ret = true
                break
            end

        -- this should never happend
        else
            error "Unknown rule type"
        end
    end

    -- if matched populate files table
    if ret then
        self.files[gpuName] = self.files[gpuName] or { }
        self.files[gpuName][#self.files[gpuName] + 1] = file
    end
    
    return ret    
end

--[[ ==========================================================================
--
-- TupPack
--
-- ============================================================================ ]]

TupState = {}
TupState.__index = TupState

function TupState.New(userConf)    
    local userConf = userConf or { }
    local self = setmetatable({ }, TupState)
    
    -- check for gpus correctness
    if userConf.gpus ~= nil then
        if type(userConf.gpus) ~= "table" then
            error "gpus are defined incorrent. Should be { gpu1 = \"patten1\"}, ...  gpuN = \"pattenN\"} }"
        end
    end
    
    -- can be defined by user
    local conf = { }
    conf.outputDir = tup.getcwd() .. "/" .. (userConf.outputDir or "Assets")
    conf.superpackDir = tup.getcwd() .. "/" .. (userConf.superpackDir or "AssetsSuperpack")
    conf.intermediateDir = tup.getcwd() .. "/" .. (userConf.intermediateDir or ".tmp")

    conf.superpack = userConf.superpack or false

    conf.outputDbExt = userConf.outputDbExt or ".db"
    conf.packlistExt = userConf.packlistExt or ".list"
    conf.mergedlistExt = userConf.mergedlistExt or ".mergedlist"

    conf.commonGpu = userConf.commonGpu or "common" 
    conf.gpuVar = userConf.gpuVar or "{gpu}" 
    conf.gpus = userConf.gpus or { 
        pvr_ios = "PowerVR_iOS%.",
        pvr_android = "PowerVR_Android%.", 
        tegra = "tegra%.",
        mali = "mali%.",
        adreno = "adreno%.",
        dx11 = "dx11%."
    }
    conf.baseGpu = userConf.baseGpu or "mali"

    conf.unusedPackName = userConf.unusedPackName or "__unused__"
    conf.delimiter = userConf.delimiter or "#"
    conf.cmdMaxFilesCount = userConf.cmdMaxFilesCount or 255

    -- check for gpu names intersection
    if conf.gpus[conf.commonGpu] ~= nil then
        print("GPU name: " .. conf.commonGpu)
        error "Common gpu name is same as real gpu"
    end

    -- setting up configuration
    self.conf = conf
            
    -- can't be defined by user
    self.platform = tup.getconfig("TUP_PLATFORM");
    self.projectDir = tup.getcwd()
    self.currentDir = tup.getrelativedir(self.projectDir)
    self.currentDirString = self.currentDir:gsub("/", "_") 
    
    self.outputDir = self.conf.outputDir
    self.packlistDir = self.conf.intermediateDir .. "/lists"
    self.mergeDir = self.conf.intermediateDir .. "/merged"
    self.sqlDir = self.conf.intermediateDir .. "/sql"
    self.dbDir = self.conf.intermediateDir .. "/db"
    self.packsDir = self.conf.intermediateDir .. "/packs"
       
    --debugger()

    local dbginfo = debug.getinfo(1, 'S')
    --local fwPath = self:FindFWLuaPath(dbginfo.source:sub(2), self.projectDir)
    local fwPath = self:FindFWLuaPath(davapath, self.projectDir)
    self.frameworkPath = fwPath
           
    -- setting up commands
    self.cmd = { }
    self.cmd.cp = "cp"
    self.cmd.cat = "cat"
    self.cmd.fwdep = fwPath .. "../Toolset/dep"    
    self.cmd.fwzip = fwPath .. "../7za"    
    self.cmd.fwsql = fwPath .. "../sqlite3"
    self.cmd.fwResourceArchive = fwPath .. "../x64/ResourceArchiver"
    
    if self.platform == "win32" then
        self.cmd.cp = "copy"
        self.cmd.cat = "type"
        self.cmd.fwzip = fwPath .. "../7z.exe"
        self.cmd.fwsql = self.cmd.fwsql .. ".exe"
        self.cmd.fwdep = self.cmd.fwdep .. ".exe" 
        self.cmd.fwResourceArchive = self.cmd.fwResourceArchive .. ".exe"
    
        UtilConvertToPlatformPath(self.platform, self.cmd)
    end
    
    self.packs = { }
    self.packNames = { }    
    
    if userConf.packs ~= nil then
        self:AddPacks(userConf.packs)
    end
    
    return self
end

function TupState.GetPackGroup(self, packName, packGPU)
    return packName .. packGPU
end

function TupState.GetSqlGroup(self, packGPU)
    return "sql_" .. packGPU
end

function TupState.GetOutputDir(self, isBasePack, defaultDir)
    if self.conf.superpack == true then
        if isBasePack == true then
            return self.outputDir
        else
            return defaultDir
        end
    else
        return self.outputDir
    end
end

function TupState.FindFWLuaPath(self, sourcePath, projectDir)
    -- crop string from the end to the first
    -- separator presence
    local cropDir = function(path, sep)
        sep = sep or "/"
        return path:match("(.*" .. sep .. ")")
    end
    
    -- get dir path from file path
    local fwPath = cropDir(sourcePath, "/") or cropDir(sourcePath, "\\")

    -- if got dir isn't absolute we should concat
    -- it with project dir
    if fwPath:sub(1, 1) ~= "/" and fwPath:match(":\\") == nil then
        fwPath = projectDir .. "/" .. fwPath
    end
    
    return fwPath
end

function TupState.AddPack(self, t)
    if TupPack.IsOkPackParams(t) then
        -- create pack
        local pack = TupPack.New(t)

        -- check if pack with such name 
        -- already exists        
        if self.packNames[pack.name] ~= nil then
            print("PackName = " .. pack.name)
            error "Such pack name already defined"
        end
        
        -- remember created pack and its name
        self.packs[#self.packs + 1] = pack
        self.packNames[pack.name] = true
    end
end

function TupState.AddPacks(self, t)

    -- packs should be defined as table
    if type(t) ~= "table" then
        error "Packs should be specified as array"
    end
    
    -- go throught packs and create
    -- corresponding TupPack tables
    for k,v in pairs(t) do
        
        if type(k) ~= "number" then
            error "Packs should be defined in indexed array" 
        end
        
        self:AddPack(v)
    end
end

function TupState.BuildLists(self)
    
    local tupFiles = tup.glob("*")

    for k, file in pairs(tupFiles) do 

        local matchedPacks = { }

        -- find all packs that match current file
        for pi, pack in pairs(self.packs) do

            local isMatched = false 

            -- if pack is gpu dependent
            -- try to match file for each gpu
            if pack.is_gpu then
                for gpu, gpuPattern in pairs(self.conf.gpus) do
                    local m = pack:Match(self.currentDir, file, gpu, self.conf.gpuVar, gpuPattern)
                    isMatched = isMatched or m
                end
            else
            -- try to match pack for common gpu
                local m = pack:Match(self.currentDir, file, self.conf.commonGpu)
                isMatched = isMatched or m
            end

            if isMatched then
                matchedPacks[pack.name] = true
                break
            end
        end
        
        local firstMatch = next(matchedPacks)
        local secondMatch = next(matchedPacks, firstMatch)

        -- check that file match only one pack
        -- if not - that should be thread as error
        if secondMatch ~= nil then
            print("Packs: " .. firstMatch .. " and " .. secondMatch)
            error "File is matching more than one pack"
        end
    end
    
    for pi, pack in pairs(self.packs) do
        for gpu, files in pairs(pack.files) do
            if pack.is_base == true then
                -- copy only files with mached GPU or common files
                if gpu == self.conf.baseGpu or gpu == self.conf.commonGpu then
                    for index, file in pairs(files) do
                        local baseOutput = self.conf.outputDir .. "/Data/" .. self.currentDir .. "/" .. file
                        baseOutput = UtilConvertToPlatformPath(self.platform, baseOutput) 
                        -- ganerate rool to copy files from base pack into specified base-directory
                        -- using framework dep tool to create destination directory
                        tup.rule(file, self.cmd.fwdep .. " merge %\"f -o %\"o", {baseOutput}) 
                    end
                end
            else
                local packGroup = self:GetPackGroup(pack.name, gpu)
                local packGroupPath = self.projectDir .. "/<" .. packGroup .. ">"

                for i, part in UtilIterateTable(files, self.conf.cmdMaxFilesCount) do
                    local partCmd = self.cmd.fwdep .. " echo " .. " %\"f -o %o"
                    if self.currentDir ~= "." then 
                        partCmd = partCmd .. " -ap \"" ..  self.currentDir .. "/\""
                    end
                    local partCmdText = "^ Gen " .. gpu .. " list " .. i .. " for " .. pack.name .. "^ "
                    local partOutput = self.packlistDir .. "/" .. gpu .. "/" .. pack.name 
                        .. self.conf.delimiter .. self.currentDirString 
                        .. "-" .. i .. self.conf.packlistExt
                        
                    partOutput = UtilConvertToPlatformPath(self.platform, partOutput) 
                    tup.rule(part, partCmdText .. partCmd, { partOutput, packGroupPath })
                end
            end
        end
    end    
end

function TupState.BuildPacks(self)
    local superPackGroup = "superpack"
    local superPackGroupPath = self.projectDir .. "/<" .. superPackGroup .. ">"
    local superPackFiles = { }

    for pai, pack in pairs(self.packs) do
        -- not base pack
        -- by default gpu list contain only common folder
        local gpus = { }
        gpus[self.conf.commonGpu] = self.conf.commonGpu
        
        -- if pack is gpu-dependent we should use 
        -- real gpu folders from config
        if pack.is_gpu then
            gpus = self.conf.gpus
        end
    
        -- now execute commands for each gpu in list
        for gpu, gpuv in pairs(gpus) do
            local packGroup = self:GetPackGroup(pack.name, gpu)
            local packGroupPath = self.projectDir .. "/<" .. packGroup .. ">"
            local sqlGroup = self:GetSqlGroup(gpu)
            local sqlGroupPath = self.sqlDir .. "/<" .. sqlGroup .. ">"

            -- generate emply lists for each pack
            -- this will allow cat/type command not fail
            -- if no lists were generated for pack
            local emptyPackCmd = self.cmd.fwdep .. " echo -o %o"
            local emptyPackCmdText = "^ Get empty list for " .. pack.name .. gpu .. "^ "
            local emptyPackOutput = self.packlistDir .. "/" .. gpu .. "/" .. pack.name .. self.conf.delimiter .. "_empty" .. self.conf.packlistExt 
            
            tup.rule(emptyPackCmdText .. emptyPackCmd, { emptyPackOutput })
 
            -- merge lists
            local mergePackMask = self.packlistDir .. "/" .. gpu .. "/" .. pack.name .. self.conf.delimiter .. "*" .. self.conf.packlistExt 
            local mergePackCmd = self.cmd.fwdep .. " merge " .. mergePackMask .. " -o %o" --[[" -- %<" .. packGroup .. ">"]]
            local mergePackCmdText = "^ Gen merged list for " .. packGroup .. "^ "
            local mergePackOutput = self.mergeDir .. "/" .. gpu .. "/" .. pack.name .. self.conf.mergedlistExt
            
            tup.rule({ mergePackMask, packGroupPath, emptyPackOutput }, mergePackCmdText .. mergePackCmd, { mergePackOutput })

            if pack.is_base ~= true then
                -- archivate
                local archiveCmd = self.cmd.fwResourceArchive .. " pack -compression " .. pack.compression .. " -listfile %f %o"
                local archiveCmdText = "^ Archive " .. pack.name .. gpu .. "^ "
                local archiveOutput = self:GetOutputDir(pack.is_base, self.packsDir) .. "/" .. gpu .. "/" .. pack.name .. ".dvpk"
                tup.rule(mergePackOutput, archiveCmdText .. archiveCmd, archiveOutput)
                superPackFiles[#superPackFiles + 1] = UtilConvertToPlatformPath(self.platform, archiveOutput)
                -- generate pack sql
                local packDepends = table.concat(pack.depends, " ")
                local isGpu = tostring(gpu ~= "common")
                local sqlCmd = self.cmd.fwdep .. " sql -l " .. mergePackOutput .. " -g " .. isGpu .. " " .. archiveOutput .. " ".. pack.name .. " " .. packDepends .. " -o %o"
                local sqlCmdText = "^ SQL for " .. pack.name .. gpu .. "^ "
                local sqlOutput = self.sqlDir .. "/" .. gpu .. "/" .. pack.name .. ".sql"
                tup.rule({ mergePackOutput, archiveOutput }, sqlCmdText ..sqlCmd, { sqlOutput, sqlGroupPath }) 
            end           
        end -- end for gpus
    end -- end for packs
    
    -- create sqlite database for each gpu
    for gpu, gpuv in pairs(self.conf.gpus) do
        local sqlGroup = self:GetSqlGroup(gpu)
        local sqlGroupPath = self.sqlDir .. "/<" .. sqlGroup .. ">"
        local sqlCommonGroup = self:GetSqlGroup(self.conf.commonGpu)
        local sqlCommonGroupPath = self.sqlDir .. "/<" .. sqlCommonGroup .. ">"
    
        -- generate emply sql for each gpu
        local emptySqlCmd = self.cmd.fwdep .. " echo -o %o"
        local emptySqlCmdText = "^ Get empty sql for " .. gpu .. "^ "
        local emptySqlOutput = self.sqlDir .. "/" .. gpu .. "/" .. "empty.sql"
        tup.rule(emptySqlCmdText .. emptySqlCmd, { emptySqlOutput })

        -- merge final sql
        local mergeSqlMaskCommon = self.sqlDir .. "/" .. self.conf.commonGpu .. "/*.sql"
        local mergeSqlMaskGpu = self.sqlDir .. "/" .. gpu .. "/*.sql"  
        local mergeSqlCmd = self.cmd.fwdep .. " merge " .. mergeSqlMaskGpu .. " " .. mergeSqlMaskCommon .. " -o %o"
        local mergeSqlCmdText = "^ Gen merged sql " .. gpu .. "^ "
        local mergeSqlOutput = self.mergeDir .. "/" .. gpu .. "/final.sql" 

        mergeSqlCmd = UtilConvertToPlatformPath(self.platform, mergeSqlCmd)
            
        tup.rule({ mergeSqlMaskGpu, mergeSqlMaskCommon, sqlGroupPath , sqlCommonGroupPath },
            mergeSqlCmdText .. mergeSqlCmd, mergeSqlOutput)
            
        -- generate packs database
        local dbName = "db_" .. gpu .. self.conf.outputDbExt
        local dbOutput = self.dbDir .. "/" .. dbName
        local dbCmd = self.cmd.fwsql .. ' -cmd ".read ' .. mergeSqlOutput .. '" -cmd ".save ' .. dbOutput .. '" "" ""'
        local dbCmdText = "^ Gen final packs DB for " .. gpu .. "^ "
        tup.rule(mergeSqlOutput, dbCmdText .. dbCmd, dbOutput)

        -- zip generated db
        local dbZipOutput = self:GetOutputDir(true, self.packsDir) .. "/" .. dbName .. ".zip"
        local dbZipCmd = self.cmd.fwzip .. " a -bd -bso0 -tzip %o %f"
        local dbZipCmdText = "^ Zip final packs DB for " .. gpu .. "^ "
        tup.rule(dbOutput, dbZipCmdText .. dbZipCmd, dbZipOutput)

        -- zip generated and output into packs dir
        -- will be used when generating superpack
        if self.conf.superpack == true then
            local dbZipOutput2 = self.packsDir .. "/" ..dbName .. ".zip"
            local dbZipCmd2 = self.cmd.fwzip .. " a -bd -bso0 -tzip %o %f"
            local dbZipCmdText2 = "^ Zip final packs DB for superpack^ "
            tup.rule(dbOutput, dbZipCmdText2 .. dbZipCmd2, dbZipOutput2)

            superPackFiles[#superPackFiles + 1] = UtilConvertToPlatformPath(self.platform, dbZipOutput2)
        end
    end

    if self.conf.superpack == true then
        local packsDirPrefix = UtilConvertToPlatformPath(self.platform, self.packsDir .. "/") 

        -- create superpack lists
        for i, part in UtilIterateTable(superPackFiles, self.conf.cmdMaxFilesCount) do
            local superPartOutput = self.packlistDir .. "/super-" .. i .. self.conf.packlistExt
            local superPartCmd = self.cmd.fwdep .. " echo %f -rp " .. packsDirPrefix .." -o %o"
            local superPartCmdText = "^ Gen superpack list-" .. i .. "^ "
            tup.rule(part, superPartCmdText .. superPartCmd, { superPartOutput, superPackGroupPath })
        end

        -- merge superpack lists
        local mergedSuperMask = self.packlistDir .. "/super-*" .. self.conf.packlistExt
        local mergedSuperCmd = self.cmd.fwdep .. " merge " .. mergedSuperMask .. " > %o"
        local mergedSuperCmdText = "^ Gen merged superlist^ "
        local mergedSuperOutput = self.mergeDir .. "/super" ..  self.conf.mergedlistExt

        mergedSuperCmd = UtilConvertToPlatformPath(self.platform, mergedSuperCmd)

        tup.rule({ mergedSuperMask, superPackGroupPath }, mergedSuperCmdText .. mergedSuperCmd, mergedSuperOutput)

        -- create super pack
        local superpackOutput = self.conf.superpackDir .. "/superpack.dvpk"
        local superpackCmdText = "^ Archive superpack^ "
        local superpackCmd = self.cmd.fwResourceArchive .. " pack -compression none -basedir " .. self.packsDir .. "/ -listfile %f %o"
        tup.rule(mergedSuperOutput, superpackCmdText .. superpackCmd, superpackOutput)
    end
end
