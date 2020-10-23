#ifndef __QUICKED_YAML_PACKAGE_SERIALIZER_H__
#define __QUICKED_YAML_PACKAGE_SERIALIZER_H__

#include "PackageSerializer.h"

namespace DAVA
{
class YamlNode;
}

class YamlPackageSerializer : public PackageSerializer
{
public:
    YamlPackageSerializer();
    virtual ~YamlPackageSerializer();

    virtual void PutValue(const DAVA::String& name, const DAVA::String& value, bool quotes) override;
    virtual void PutValue(const DAVA::String& name, const DAVA::Vector<DAVA::String>& value) override;
    virtual void PutValue(const DAVA::String& value, bool quotes) override;

    virtual void BeginMap() override;
    virtual void BeginMap(const DAVA::String& name, bool quotes = false) override;
    virtual void EndMap() override;

    virtual void BeginArray(const DAVA::String& name, bool flow = false) override;
    virtual void BeginArray() override;
    virtual void BeginFlowArray() override;
    virtual void EndArray() override;

    DAVA::YamlNode* GetYamlNode() const;
    bool WriteToFile(const DAVA::FilePath& path);
    DAVA::String WriteToString() const;

private:
    DAVA::Vector<DAVA::RefPtr<DAVA::YamlNode>> nodesStack;
};


#endif // __QUICKED_YAML_PACKAGE_SERIALIZER_H__
