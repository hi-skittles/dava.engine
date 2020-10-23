#pragma once

#include <TArc/DataProcessing/DataNode.h>

#include <Reflection/Reflection.h>
#include <FileSystem/FilePath.h>
#include <Math/Vector.h>
#include <Math/Color.h>
#include <Base/FastName.h>
#include <Base/UnordererMap.h>

class SlotTemplatesData : public DAVA::TArc::DataNode
{
public:
    struct Template
    {
        DAVA::FastName name;
        DAVA::Vector3 boundingBoxSize;
        DAVA::Vector3 pivot;
    };

    const Template* GetTemplate(DAVA::FastName name) const;
    DAVA::Vector<Template> GetTemplates() const;

private:
    friend class SlotSupportModule;
    void Clear();
    void ParseConfig(const DAVA::FilePath& configPath);

private:
    DAVA::UnorderedMap<DAVA::FastName, Template> templates;

    DAVA_VIRTUAL_REFLECTION(SlotTemplatesData, DAVA::TArc::DataNode);
};
