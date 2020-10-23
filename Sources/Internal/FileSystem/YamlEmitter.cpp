#include "YamlEmitter.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/YamlNode.h"
#include "Logger/Logger.h"

#define YAML_DECLARE_STATIC
#include "yaml/yaml.h"

namespace DAVA
{
static const int32 INDENTATION_INCREMENT = 4;
static const int32 UNESCAPED_UNICODE_CHARACTERS_ALLOWED = 1;
static const int32 PREFERRED_LINE_WIDTH = -1; //-1 means unlimited.

static yaml_scalar_style_t GetYamlScalarStyle(YamlNode::eStringRepresentation representation)
{
    yaml_scalar_style_t style;
    switch (representation)
    {
    case YamlNode::SR_DOUBLE_QUOTED_REPRESENTATION:
        style = YAML_DOUBLE_QUOTED_SCALAR_STYLE;
        break;
    case YamlNode::SR_PLAIN_REPRESENTATION:
        style = YAML_PLAIN_SCALAR_STYLE;
        break;
    default:
        style = YAML_ANY_SCALAR_STYLE;
        break;
    }
    return style;
}

static yaml_sequence_style_t GetYamlSequenceStyle(YamlNode::eArrayRepresentation representation)
{
    yaml_sequence_style_t style;
    switch (representation)
    {
    case YamlNode::AR_BLOCK_REPRESENTATION:
        style = YAML_BLOCK_SEQUENCE_STYLE;
        break;
    case YamlNode::AR_FLOW_REPRESENTATION:
        style = YAML_FLOW_SEQUENCE_STYLE;
        break;
    default:
        style = YAML_ANY_SEQUENCE_STYLE;
        break;
    }
    return style;
}

static yaml_mapping_style_t GetYamlMappingStyle(YamlNode::eMapRepresentation representation)
{
    yaml_mapping_style_t style;
    switch (representation)
    {
    case YamlNode::MR_BLOCK_REPRESENTATION:
        style = YAML_BLOCK_MAPPING_STYLE;
        break;
    case YamlNode::MR_FLOW_REPRESENTATION:
        style = YAML_FLOW_MAPPING_STYLE;
        break;
    default:
        style = YAML_ANY_MAPPING_STYLE;
        break;
    }
    return style;
}

int write_handler(void* ext, unsigned char* buffer, size_t size) //yaml_write_handler_t
{
    File* yamlFile = static_cast<File*>(ext);

    uint32 bytesWritten = yamlFile->Write(buffer, static_cast<uint32>(size));

    return (size == bytesWritten) ? 1 : 0;
}

DAVA::YamlEmitter::~YamlEmitter()
{
}

DAVA::YamlEmitter::YamlEmitter()
{
}

bool YamlEmitter::SaveToYamlFile(const FilePath& outFileName, const YamlNode* node, uint32 attr)
{
    ScopedPtr<File> outFile(File::Create(outFileName, attr));
    if (!outFile)
    {
        Logger::Error("[YamlEmitter::Emit] Can't create file: %s for output %s", outFileName.GetStringValue().c_str(), strerror(errno));
        return false;
    }

    return SaveToYamlFile(node, outFile);
}

bool YamlEmitter::SaveToYamlFile(const YamlNode* node, File* outfile)
{
    ScopedPtr<YamlEmitter> emitter(new YamlEmitter());
    return emitter->Emit(node, outfile);
}

bool YamlEmitter::Emit(const YamlNode* node, File* outFile)
{
    yaml_emitter_t emitter;

    const int initializeResult = yaml_emitter_initialize(&emitter);
    DVASSERT(initializeResult);
    yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);
    yaml_emitter_set_break(&emitter, YAML_ANY_BREAK);
    yaml_emitter_set_unicode(&emitter, UNESCAPED_UNICODE_CHARACTERS_ALLOWED);
    yaml_emitter_set_width(&emitter, PREFERRED_LINE_WIDTH);
    yaml_emitter_set_indent(&emitter, INDENTATION_INCREMENT);
    yaml_emitter_set_output(&emitter, &write_handler, outFile);

    do
    {
        if (!EmitStreamStart(&emitter))
            break;

        if (!EmitDocumentStart(&emitter))
            break;

        if (!EmitYamlNode(&emitter, node))
            break;

        if (!EmitDocumentEnd(&emitter))
            break;

        if (!EmitStreamEnd(&emitter))
            break;
    } while (0);

    switch (emitter.error)
    {
    case YAML_NO_ERROR:
        break;

    case YAML_MEMORY_ERROR:
        Logger::Error("[YamlEmitter::Emit] Memory error: Not enough memory for emitting");
        break;

    case YAML_WRITER_ERROR:
        Logger::Error("[YamlEmitter::Emit] Writer error: %s", emitter.problem);
        break;

    case YAML_EMITTER_ERROR:
        Logger::Error("[YamlEmitter::Emit] Emitter error: %s", emitter.problem);
        break;

    case YAML_READER_ERROR:
    case YAML_SCANNER_ERROR:
    case YAML_PARSER_ERROR:
    case YAML_COMPOSER_ERROR:
    default:
        /* Couldn't happen. */
        Logger::Error("[YamlEmitter::Emit] Internal error\n");
        break;
    }

    const int flushResult = yaml_emitter_flush(&emitter);
    DVASSERT(flushResult);
    yaml_emitter_delete(&emitter);

    return true;
}

bool YamlEmitter::EmitYamlNode(yaml_emitter_t* emitter, const YamlNode* node)
{
    switch (node->GetType())
    {
    case YamlNode::TYPE_STRING:
    {
        if (!EmitScalar(emitter, node->AsString(), GetYamlScalarStyle(node->GetStringRepresentation())))
            return false;
    }
    break;
    case YamlNode::TYPE_ARRAY:
    {
        if (!EmitSequenceStart(emitter, GetYamlSequenceStyle(node->GetArrayRepresentation())))
            return false;

        int32 count = node->GetCount();
        for (int32 i = 0; i < count; ++i)
        {
            if (!EmitYamlNode(emitter, node->Get(i)))
                return false;
        }

        if (!EmitSequenceEnd(emitter))
            return false;
    }
    break;
    case YamlNode::TYPE_MAP:
    {
        if (!EmitMappingStart(emitter, GetYamlMappingStyle(node->GetMapRepresentation())))
            return false;

        bool res = node->GetMapOrderRepresentation() ? EmitOrderedMap(emitter, node) : EmitUnorderedMap(emitter, node);
        if (!res)
            return false;

        if (!EmitMappingEnd(emitter))
            return false;
    }
    break;
    }

    return true;
}

bool YamlEmitter::EmitStreamStart(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    if (!yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING) ||
        !yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool YamlEmitter::EmitStreamEnd(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    if (!yaml_stream_end_event_initialize(&event) ||
        !yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool YamlEmitter::EmitDocumentStart(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    if (!yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1) ||
        !yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool YamlEmitter::EmitDocumentEnd(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    if (!yaml_document_end_event_initialize(&event, 1) ||
        !yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool YamlEmitter::EmitSequenceStart(yaml_emitter_t* emitter, int32 sequenceStyle)
{
    yaml_event_t event;
    if (!yaml_sequence_start_event_initialize(&event, NULL, NULL, 0, static_cast<yaml_sequence_style_t>(sequenceStyle)) ||
        !yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool YamlEmitter::EmitSequenceEnd(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    if (!yaml_sequence_end_event_initialize(&event) ||
        !yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool YamlEmitter::EmitMappingStart(yaml_emitter_t* emitter, int32 mappingStyle)
{
    yaml_event_t event;
    if (!yaml_mapping_start_event_initialize(&event, NULL, NULL, 0, static_cast<yaml_mapping_style_t>(mappingStyle)) ||
        !yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool YamlEmitter::EmitMappingEnd(yaml_emitter_t* emitter)
{
    yaml_event_t event;
    if (!yaml_mapping_end_event_initialize(&event) ||
        !yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool YamlEmitter::EmitScalar(yaml_emitter_t* emitter, const String& value, int32 scalarStyle)
{
    yaml_event_t event;

    const yaml_char_t* tag = reinterpret_cast<const yaml_char_t*>(YAML_DEFAULT_SCALAR_TAG);
    const yaml_char_t* data = reinterpret_cast<const yaml_char_t*>(value.c_str());
    if (!yaml_scalar_event_initialize(&event, NULL, const_cast<yaml_char_t*>(tag), const_cast<yaml_char_t*>(data), -1, 1, 1, static_cast<yaml_scalar_style_t>(scalarStyle)) ||
        !yaml_emitter_emit(emitter, &event))
        return false;

    return true;
}

bool YamlEmitter::EmitUnorderedMap(yaml_emitter_t* emitter, const YamlNode* mapNode)
{
    int32 count = mapNode->GetCount();
    for (int32 i = 0; i < count; ++i)
    {
        if (!EmitScalar(emitter, mapNode->GetItemKeyName(i), GetYamlScalarStyle(mapNode->GetMapKeyRepresentation())))
            return false;
        if (!EmitYamlNode(emitter, mapNode->Get(i)))
            return false;
    }
    return true;
}

bool YamlEmitter::EmitOrderedMap(yaml_emitter_t* emitter, const YamlNode* mapNode)
{
    const auto& map = mapNode->AsMap();
    auto iter = map.begin(), end = map.end();
    for (; iter != end; ++iter)
    {
        if (!EmitScalar(emitter, iter->first, GetYamlScalarStyle(mapNode->GetMapKeyRepresentation())))
            return false;
        if (!EmitYamlNode(emitter, iter->second.Get()))
            return false;
    }
    return true;
}
}
