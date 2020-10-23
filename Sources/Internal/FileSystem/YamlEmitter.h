#ifndef __DAVAENGINE_YAML_EMITTER_H__
#define __DAVAENGINE_YAML_EMITTER_H__
#include "Base/BaseObject.h"
#include "Base/BaseTypes.h"
#include "FileSystem/File.h"

using yaml_emitter_t = struct yaml_emitter_s;

namespace DAVA
{
class YamlNode;
/** 
     \ingroup yaml
     \brief this class is yaml saver and it used if you want to save data to yaml file
     */
class YamlEmitter : public BaseObject
{
    virtual ~YamlEmitter();
    YamlEmitter();

public:
    /**
     \brief Store content of node to file
     \returns true if success.
     */
    static bool SaveToYamlFile(const FilePath& outFileName, const YamlNode* node, uint32 attr = File::CREATE | File::WRITE);
    static bool SaveToYamlFile(const YamlNode* node, File* outfile);

protected:
    bool Emit(const YamlNode* node, File* outFile);

private:
    bool EmitStreamStart(yaml_emitter_t* emitter);
    bool EmitStreamEnd(yaml_emitter_t* emitter);
    bool EmitDocumentStart(yaml_emitter_t* emitter);
    bool EmitDocumentEnd(yaml_emitter_t* emitter);
    bool EmitSequenceStart(yaml_emitter_t* emitter, int32 sequenceStyle /*yaml_sequence_style_t*/);
    bool EmitSequenceEnd(yaml_emitter_t* emitter);
    bool EmitMappingStart(yaml_emitter_t* emitter, int32 mappingStyle /*yaml_mapping_style_t*/);
    bool EmitMappingEnd(yaml_emitter_t* emitter);
    bool EmitScalar(yaml_emitter_t* emitter, const String& value, int32 scalarStyle /*yaml_scalar_style_t*/);
    bool EmitYamlNode(yaml_emitter_t* emitter, const YamlNode* node);
    bool EmitUnorderedMap(yaml_emitter_t* emitter, const YamlNode* mapNode);
    bool EmitOrderedMap(yaml_emitter_t* emitter, const YamlNode* mapNode);
};
};
#endif // __DAVAENGINE_YAML_EMITTER_H__