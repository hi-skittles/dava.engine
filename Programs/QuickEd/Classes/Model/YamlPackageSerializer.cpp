#include "YamlPackageSerializer.h"

#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlEmitter.h"
#include "FileSystem/DynamicMemoryFile.h"

using namespace DAVA;

YamlPackageSerializer::YamlPackageSerializer()
{
    nodesStack.push_back(YamlNode::CreateMapNode(false, YamlNode::MR_BLOCK_REPRESENTATION, YamlNode::SR_PLAIN_REPRESENTATION));
}

YamlPackageSerializer::~YamlPackageSerializer()
{
    DVASSERT(nodesStack.size() == 1);

    nodesStack.clear();
}

void YamlPackageSerializer::PutValue(const DAVA::String& name, const DAVA::String& value, bool quotes)
{
    YamlNode* parent = nodesStack.back().Get();
    RefPtr<YamlNode> node = YamlNode::CreateStringNode();
    node->Set(value);
    if (!quotes)
    {
        node->SetStringRepresentation(YamlNode::SR_PLAIN_REPRESENTATION);
    }
    parent->Add(name, node);
}

void YamlPackageSerializer::PutValue(const DAVA::String& name, const DAVA::Vector<DAVA::String>& value)
{
    RefPtr<YamlNode> node = YamlNode::CreateArrayNode(YamlNode::AR_FLOW_REPRESENTATION);
    for (const auto& str : value)
        node->Add(str);

    nodesStack.back()->Add(name, node);
}

void YamlPackageSerializer::PutValue(const DAVA::String& value, bool quotes)
{
    YamlNode* parent = nodesStack.back().Get();
    RefPtr<YamlNode> node = YamlNode::CreateStringNode();
    node->Set(value);
    if (!quotes)
    {
        node->SetStringRepresentation(YamlNode::SR_PLAIN_REPRESENTATION);
    }
    parent->Add(node);
}

void YamlPackageSerializer::BeginMap()
{
    RefPtr<YamlNode> node = YamlNode::CreateMapNode(false, YamlNode::MR_BLOCK_REPRESENTATION, YamlNode::SR_PLAIN_REPRESENTATION);
    nodesStack.back()->Add(node);
    nodesStack.push_back(node);
}

void YamlPackageSerializer::BeginMap(const DAVA::String& name, bool quotes)
{
    RefPtr<YamlNode> node = YamlNode::CreateMapNode(false, YamlNode::MR_BLOCK_REPRESENTATION,
                                                    quotes ? YamlNode::SR_DOUBLE_QUOTED_REPRESENTATION : YamlNode::SR_PLAIN_REPRESENTATION);
    nodesStack.back()->Add(name, node);
    nodesStack.push_back(node);
}

void YamlPackageSerializer::EndMap()
{
    nodesStack.pop_back();
}

void YamlPackageSerializer::BeginArray(const DAVA::String& name, bool flow)
{
    RefPtr<YamlNode> node = YamlNode::CreateArrayNode(flow ? YamlNode::AR_FLOW_REPRESENTATION : YamlNode::AR_BLOCK_REPRESENTATION);
    nodesStack.back()->Add(name, node);
    nodesStack.push_back(node);
}

void YamlPackageSerializer::BeginArray()
{
    RefPtr<YamlNode> node = YamlNode::CreateArrayNode(YamlNode::AR_BLOCK_REPRESENTATION);
    nodesStack.back()->Add(node);
    nodesStack.push_back(node);
}

void YamlPackageSerializer::BeginFlowArray()
{
    RefPtr<YamlNode> node = YamlNode::CreateArrayNode(YamlNode::AR_FLOW_REPRESENTATION);
    nodesStack.back()->Add(node);
    nodesStack.push_back(node);
}

void YamlPackageSerializer::EndArray()
{
    nodesStack.pop_back();
}

YamlNode* YamlPackageSerializer::GetYamlNode() const
{
    DVASSERT(nodesStack.size() == 1);
    return nodesStack.back().Get();
}

bool YamlPackageSerializer::WriteToFile(const FilePath& path)
{
    return YamlEmitter::SaveToYamlFile(path, GetYamlNode());
}

String YamlPackageSerializer::WriteToString() const
{
    DynamicMemoryFile* file = DynamicMemoryFile::Create(File::WRITE);
    YamlEmitter::SaveToYamlFile(GetYamlNode(), file);
    String str(reinterpret_cast<const char*>(file->GetData()), file->GetSize());
    SafeRelease(file);
    return str;
}
