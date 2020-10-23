#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLayer.h"
#include "Utils/StringFormat.h"
#include "FileSystem/FileSystem.h"
#include "FileSystem/YamlNode.h"
#include "FileSystem/YamlEmitter.h"
#include "Logger/Logger.h"
#include "Reflection/ReflectionRegistrator.h"

namespace DAVA
{
bool ParticleEmitter::FORCE_DEEP_CLONE = false;
const float32 ParticleEmitter::PARTICLE_EMITTER_DEFAULT_LIFE_TIME = 100.0f;

PartilceEmitterLoadProxy::PartilceEmitterLoadProxy()
{
    AddFlag(RenderObject::ALWAYS_CLIPPING_VISIBLE);
    bbox = AABBox3(Vector3(0, 0, 0), Vector3(0, 0, 0));
}

void PartilceEmitterLoadProxy::Load(KeyedArchive* archive, SerializationContext* serializationContext)
{
    if (NULL != archive)
        emitterFilename = archive->GetString("pe.configpath");
}

ParticleEmitter::ParticleEmitter()
{
    Cleanup(false);
}

ParticleEmitter::~ParticleEmitter()
{
    CleanupLayers();
    ReleaseFromCache(configPath);
}

void ParticleEmitter::Cleanup(bool needCleanupLayers)
{
    lifeTime = PARTICLE_EMITTER_DEFAULT_LIFE_TIME;
    emitterType = EMITTER_POINT;
    emissionVector.Set(NULL);
    emissionVector = RefPtr<PropertyLineValue<Vector3>>(new PropertyLineValue<Vector3>(Vector3(1.0f, 0.0f, 0.0f)));

    emissionVelocityVector.Set(nullptr);

    emissionAngle = nullptr;
    emissionAngleVariation = nullptr;
    emissionRange = nullptr;
    emissionRange = RefPtr<PropertyLineValue<float32>>(new PropertyLineValue<float32>(0.0f));
    size = nullptr;
    colorOverLife = nullptr;
    radius = nullptr;
    innerRadius = nullptr;
    name = FastName("Particle Emitter");

    if (needCleanupLayers)
    {
        CleanupLayers();
    }
}

void ParticleEmitter::CleanupLayers()
{
    for (auto& layer : layers)
    {
        SafeRelease(layer);
    }
    layers.clear();
}

ParticleEmitter* ParticleEmitter::Clone()
{
    if (!requireDeepClone) //emitter referencing is allowed instead of deep cloning
        return SafeRetain(this);

    ParticleEmitter* clonedEmitter = new ParticleEmitter();
    clonedEmitter->configPath = this->configPath;

    clonedEmitter->name = name;
    clonedEmitter->lifeTime = lifeTime;

    if (this->emissionVector)
    {
        clonedEmitter->emissionVector = this->emissionVector->Clone();
        clonedEmitter->emissionVector->Release();
    }
    if (this->emissionVelocityVector)
    {
        clonedEmitter->emissionVelocityVector = this->emissionVelocityVector->Clone();
        clonedEmitter->emissionVelocityVector->Release();
    }
    if (this->emissionAngle)
    {
        clonedEmitter->emissionAngle = this->emissionAngle->Clone();
        clonedEmitter->emissionAngle->Release();
    }
    if (this->emissionAngleVariation)
    {
        clonedEmitter->emissionAngleVariation = this->emissionAngleVariation->Clone();
        clonedEmitter->emissionAngleVariation->Release();
    }
    if (this->emissionRange)
    {
        clonedEmitter->emissionRange = this->emissionRange->Clone();
        clonedEmitter->emissionRange->Release();
    }
    if (this->radius)
    {
        clonedEmitter->radius = this->radius->Clone();
        clonedEmitter->radius->Release();
    }
    if (this->innerRadius)
    {
        clonedEmitter->innerRadius = this->innerRadius->Clone();
        clonedEmitter->innerRadius->Release();
    }
    if (this->colorOverLife)
    {
        clonedEmitter->colorOverLife = this->colorOverLife->Clone();
        clonedEmitter->colorOverLife->Release();
    }
    if (this->size)
    {
        clonedEmitter->size = this->size->Clone();
        clonedEmitter->size->Release();
    }

    clonedEmitter->emitterType = this->emitterType;
    clonedEmitter->shortEffect = shortEffect;
    clonedEmitter->generateOnSurface = generateOnSurface;
    clonedEmitter->shockwaveMode = shockwaveMode;

    clonedEmitter->layers.resize(layers.size());
    for (size_t i = 0, sz = layers.size(); i < sz; ++i)
    {
        clonedEmitter->layers[i] = layers[i]->Clone();
    }

    return clonedEmitter;
}

void ParticleEmitter::AddLayer(ParticleLayer* layer)
{
    if (layer == nullptr)
        return;

    // Don't allow the same layer to be added twice.
    Vector<ParticleLayer*>::iterator layerIter = std::find(layers.begin(), layers.end(), layer);
    if (layerIter != layers.end())
    {
        DVASSERT(false);
        return;
    }

    layers.push_back(SafeRetain(layer));
}

ParticleLayer* ParticleEmitter::GetNextLayer(ParticleLayer* layer)
{
    if (!layer || layers.size() < 2)
    {
        return NULL;
    }

    int32 layersToCheck = static_cast<int32>(layers.size() - 1);
    for (int32 i = 0; i < layersToCheck; i++)
    {
        ParticleLayer* curLayer = layers[i];
        if (curLayer == layer)
        {
            return layers[i + 1];
        }
    }

    return NULL;
}

void ParticleEmitter::InsertLayer(ParticleLayer* layer, ParticleLayer* beforeLayer)
{
    AddLayer(layer);
    if (beforeLayer)
    {
        MoveLayer(layer, beforeLayer);
    }
}

void ParticleEmitter::InsertLayer(ParticleLayer* layer, int32 indexToInsert)
{
    DVASSERT(0 <= indexToInsert && indexToInsert <= static_cast<int32>(layers.size()));
    layers.insert(layers.begin() + indexToInsert, SafeRetain(layer));
}

int32 ParticleEmitter::RemoveLayer(ParticleLayer* layer)
{
    int32 removedLayerIndex = -1;
    if (!layer)
    {
        return removedLayerIndex;
    }

    Vector<ParticleLayer*>::iterator layerIter = std::find(layers.begin(), layers.end(), layer);
    if (layerIter != this->layers.end())
    {
        removedLayerIndex = static_cast<int32>(std::distance(layers.begin(), layerIter));
        layers.erase(layerIter);
        SafeRelease(layer);
    }
    return removedLayerIndex;
}

void ParticleEmitter::RemoveLayer(int32 index)
{
    DVASSERT(0 <= index && index < static_cast<int32>(layers.size()));
    RemoveLayer(layers[index]);
}

void ParticleEmitter::MoveLayer(ParticleLayer* layer, ParticleLayer* beforeLayer)
{
    Vector<ParticleLayer*>::iterator layerIter = std::find(layers.begin(), layers.end(), layer);
    Vector<ParticleLayer*>::iterator beforeLayerIter = std::find(layers.begin(), layers.end(), beforeLayer);

    if (layerIter == layers.end() || beforeLayerIter == layers.end() ||
        layerIter == beforeLayerIter)
    {
        return;
    }

    layers.erase(layerIter);

    // Look for the position again - an iterator might be changed.
    beforeLayerIter = std::find(layers.begin(), layers.end(), beforeLayer);
    layers.insert(beforeLayerIter, layer);
}

bool ParticleEmitter::ContainsLayer(ParticleLayer* layer)
{
    return std::find(layers.begin(), layers.end(), layer) != layers.end();
}

ParticleEmitter::EmitterCacheMap ParticleEmitter::emitterCache;

void ParticleEmitter::ReleaseFromCache(const FilePath& name)
{
    EmitterCacheMap::iterator it = emitterCache.find(FILEPATH_MAP_KEY(name));
    if (it != emitterCache.end())
    {
        emitterCache.erase(it);
    }
}

ParticleEmitter* ParticleEmitter::LoadEmitter(const FilePath& filename)
{
    ParticleEmitter* res;
    if (FORCE_DEEP_CLONE) //resource and ui editor set this flag not to cache emitters
    {
        res = new ParticleEmitter();
        res->LoadFromYaml(filename);
        return res;
    }

    EmitterCacheMap::iterator it = emitterCache.find(FILEPATH_MAP_KEY(filename));
    if (it != emitterCache.end())
    {
        //just return reference
        res = SafeRetain(it->second);
    }
    else
    {
        res = new ParticleEmitter();
        if (res->LoadFromYaml(filename))
        {
            List<ModifiablePropertyLineBase*> modifiables;
            res->GetModifableLines(modifiables);
            if (modifiables.empty()) //if emitter have no modifiable lines - cache it
            {
                res->requireDeepClone = false; //allow referencing instead of cloning
                emitterCache[FILEPATH_MAP_KEY(filename)] = res;
            }
        }
    }

    return res;
}

bool ParticleEmitter::LoadFromYaml(const FilePath& filename, bool preserveInheritPosition)
{
    Cleanup(true);

    RefPtr<YamlParser> parser(YamlParser::Create(filename));
    if (!parser)
    {
        Logger::Error("ParticleEmitter::LoadFromYaml failed (%s)", filename.GetStringValue().c_str());
        return false;
    }

    YamlNode* rootNode = parser->GetRootNode();
    if (rootNode == nullptr)
    {
        Logger::Error("ParticleEmitter::LoadFromYaml: (%s) has no rootNode", filename.GetStringValue().c_str());
        return false;
    }

    configPath = filename;
    const YamlNode* emitterNode = rootNode->Get("emitter");
    if (emitterNode)
    {
        const YamlNode* lifeTimeNode = emitterNode->Get("life");
        if (lifeTimeNode)
        {
            lifeTime = lifeTimeNode->AsFloat();
        }
        else
        {
            lifeTime = PARTICLE_EMITTER_DEFAULT_LIFE_TIME;
        }

        const YamlNode* nameNode = emitterNode->Get("name");
        if (nameNode)
            name = FastName(nameNode->AsString().c_str());
        if (emitterNode->Get("emissionAngle"))
            emissionAngle = PropertyLineYamlReader::CreatePropertyLine<float32>(emitterNode->Get("emissionAngle"));
        if (emitterNode->Get("emissionAngleVariation"))
            emissionAngleVariation = PropertyLineYamlReader::CreatePropertyLine<float32>(emitterNode->Get("emissionAngleVariation"));

        if (emitterNode->Get("emissionVector"))
            emissionVector = PropertyLineYamlReader::CreatePropertyLine<Vector3>(emitterNode->Get("emissionVector"));

        if (emitterNode->Get("emissionVelocityVector"))
            emissionVelocityVector = PropertyLineYamlReader::CreatePropertyLine<Vector3>(emitterNode->Get("emissionVelocityVector"));

        const YamlNode* emissionVectorInvertedNode = emitterNode->Get("emissionVectorInverted");
        if (!emissionVectorInvertedNode)
        {
            // Yuri Coder, 2013/04/12. This means that the emission vector in the YAML file is not inverted yet.
            // Because of [DF-1003] fix for such files we have to invert coordinates for this vector.
            InvertEmissionVectorCoordinates();
        }

        if (emitterNode->Get("emissionRange"))
            emissionRange = PropertyLineYamlReader::CreatePropertyLine<float32>(emitterNode->Get("emissionRange"));

        if (emitterNode->Get("colorOverLife"))
            colorOverLife = PropertyLineYamlReader::CreatePropertyLine<Color>(emitterNode->Get("colorOverLife"));
        if (emitterNode->Get("radius"))
            radius = PropertyLineYamlReader::CreatePropertyLine<float32>(emitterNode->Get("radius"));
        if (emitterNode->Get("innerRadius"))
            innerRadius = PropertyLineYamlReader::CreatePropertyLine<float32>(emitterNode->Get("innerRadius"));

        const YamlNode* shortEffectNode = emitterNode->Get("shortEffect");
        if (shortEffectNode)
            shortEffect = shortEffectNode->AsBool();

        const YamlNode* generateOnSurfaceNode = emitterNode->Get("generateOnSurface");
        if (generateOnSurfaceNode)
            generateOnSurface = generateOnSurfaceNode->AsBool();

        const YamlNode* shockwaveModeNode = emitterNode->Get("shockwaveMode");
        shockwaveMode = SHOCKWAVE_DISABLED;
        if (shockwaveModeNode)
        {
            if (shockwaveModeNode->AsString() == "shockNormal")
                shockwaveMode = SHOCKWAVE_NORMAL;
            else if (shockwaveModeNode->AsString() == "shockHorizontal")
                shockwaveMode = SHOCKWAVE_HORIZONTAL;
        }

        const YamlNode* typeNode = emitterNode->Get("type");
        if (typeNode)
        {
            if (typeNode->AsString() == "point")
                emitterType = EMITTER_POINT;
            else if (typeNode->AsString() == "line")
            {
                emitterType = EMITTER_RECT;
            }
            else if (typeNode->AsString() == "rect")
                emitterType = EMITTER_RECT;
            else if (typeNode->AsString() == "oncircle")
                emitterType = EMITTER_ONCIRCLE_VOLUME;
            else if (typeNode->AsString() == "oncircle_edges")
                emitterType = EMITTER_ONCIRCLE_EDGES;
            else if (typeNode->AsString() == "shockwave") // Deprecated.
            {
                emitterType = EMITTER_ONCIRCLE_EDGES;
                shockwaveMode = SHOCKWAVE_NORMAL;
            }
            else if (typeNode->AsString() == "sphere")
                emitterType = EMITTER_SPHERE;
            else
                emitterType = EMITTER_POINT;
        }
        else
            emitterType = EMITTER_POINT;

        size = PropertyLineYamlReader::CreatePropertyLine<Vector3>(emitterNode->Get("size"));

        if (size == nullptr)
        {
            Vector3 _size(0, 0, 0);
            const YamlNode* widthNode = emitterNode->Get("width");
            if (widthNode)
                _size.x = widthNode->AsFloat();

            const YamlNode* heightNode = emitterNode->Get("height");
            if (heightNode)
                _size.y = heightNode->AsFloat();

            const YamlNode* depthNode = emitterNode->Get("depth");
            if (depthNode)
                _size.y = depthNode->AsFloat();

            size.Set(new PropertyLineValue<Vector3>(_size));
        }
    }

    int cnt = rootNode->GetCount();
    for (int k = 0; k < cnt; ++k)
    {
        const YamlNode* node = rootNode->Get(k);
        const YamlNode* typeNode = node->Get("type");
        if (typeNode && typeNode->AsString() == "layer")
        {
            LoadParticleLayerFromYaml(node, preserveInheritPosition);
        }
    }

    // Yuri Coder, 2013/01/15. The "name" node for Layer was just added and may not exist for
    // old yaml files. Generate the default name for nodes with empty names.
    UpdateEmptyLayerNames();
    return true;
}

void ParticleEmitter::SaveToYaml(const FilePath& filename)
{
    configPath = filename;

    RefPtr<YamlNode> rootYamlNode = YamlNode::CreateMapNode(false);
    YamlNode* emitterYamlNode = new YamlNode(YamlNode::TYPE_MAP);
    rootYamlNode->AddNodeToMap("emitter", emitterYamlNode);

    emitterYamlNode->Set("name", (name.c_str() ? String(name.c_str()) : ""));
    emitterYamlNode->Set("type", GetEmitterTypeName());
    emitterYamlNode->Set("shortEffect", shortEffect);
    emitterYamlNode->Set("generateOnSurface", generateOnSurface);
    emitterYamlNode->Set("shockwaveMode", GetEmitterShockwaveModeName());

    // Write the property lines.
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(emitterYamlNode, "emissionAngle", this->emissionAngle);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(emitterYamlNode, "emissionAngleVariation", this->emissionAngleVariation);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(emitterYamlNode, "emissionRange", this->emissionRange);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector3>(emitterYamlNode, "emissionVector", this->emissionVector);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector3>(emitterYamlNode, "emissionVelocityVector", this->emissionVelocityVector);

    // Yuri Coder, 2013/04/12. After the coordinates inversion for the emission vector we need to introduce the
    // new "emissionVectorInverted" flag to mark we don't need to invert coordinates after re-loading the YAML.
    PropertyLineYamlWriter::WritePropertyValueToYamlNode<bool>(emitterYamlNode, "emissionVectorInverted", true);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(emitterYamlNode, "radius", this->radius);
    PropertyLineYamlWriter::WritePropertyLineToYamlNode<float32>(emitterYamlNode, "innerRadius", this->innerRadius);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Color>(emitterYamlNode, "colorOverLife", this->colorOverLife);

    PropertyLineYamlWriter::WritePropertyLineToYamlNode<Vector3>(emitterYamlNode, "size", this->size);

    // Now write all the Layers. Note - layers are child of root node, not the emitter one.
    int32 layersCount = static_cast<int32>(layers.size());
    for (int32 i = 0; i < layersCount; i++)
    {
        this->layers[i]->SaveToYamlNode(configPath, rootYamlNode.Get(), i);
    }

    YamlEmitter::SaveToYamlFile(filename, rootYamlNode.Get());
}

void ParticleEmitter::GetModifableLines(List<ModifiablePropertyLineBase*>& modifiables)
{
    PropertyLineHelper::AddIfModifiable(emissionVector.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(emissionVelocityVector.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(emissionRange.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(radius.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(innerRadius.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(size.Get(), modifiables);
    PropertyLineHelper::AddIfModifiable(colorOverLife.Get(), modifiables);
    int32 layersCount = static_cast<int32>(layers.size());
    for (int32 i = 0; i < layersCount; i++)
    {
        layers[i]->GetModifableLines(modifiables);
    }
}

String ParticleEmitter::GetEmitterTypeName()
{
    switch (this->emitterType)
    {
    case EMITTER_POINT:
    {
        return "point";
    }

    case EMITTER_RECT:
    {
        return "rect";
    }

    case EMITTER_ONCIRCLE_VOLUME:
    {
        return "oncircle";
    }

    case EMITTER_ONCIRCLE_EDGES:
    {
        return "oncircle_edges";
    }

    case EMITTER_SPHERE:
    {
        return "sphere";
    }

    default:
    {
        return "unknown";
    }
    }
}

String ParticleEmitter::GetEmitterShockwaveModeName()
{
    switch (shockwaveMode)
    {
    case SHOCKWAVE_DISABLED:
        return "shockDisabeld";
    case SHOCKWAVE_NORMAL:
        return "shockNormal";
    case SHOCKWAVE_HORIZONTAL:
        return "shockHorizontal";
    default:
        return "shockUnknown";
    }
}

void ParticleEmitter::UpdateEmptyLayerNames()
{
    int32 layersCount = static_cast<int32>(layers.size());
    for (int32 i = 0; i < layersCount; i++)
    {
        UpdateLayerNameIfEmpty(layers[i], i);
    }
}

void ParticleEmitter::UpdateLayerNameIfEmpty(ParticleLayer* layer, int32 index)
{
    if (layer && layer->layerName.empty())
    {
        layer->layerName = Format("Layer %i", index);
    }
}

void ParticleEmitter::LoadParticleLayerFromYaml(const YamlNode* yamlNode, bool preserveInheritPosition)
{
    ParticleLayer* layer = new ParticleLayer();
    layer->LoadFromYaml(configPath, yamlNode, preserveInheritPosition);
    AddLayer(layer);
    SafeRelease(layer);
}

void ParticleEmitter::InvertEmissionVectorCoordinates()
{
    if (!this->emissionVector)
    {
        return;
    }

    PropertyLine<Vector3>* pvk = emissionVector.Get();
    for (auto& key : pvk->keys)
    {
        key.value *= -1;
    }
}

DAVA_VIRTUAL_REFLECTION_IMPL(ParticleEmitter)
{
    DAVA::ReflectionRegistrator<ParticleEmitter>::Begin()
    .End();
}
}
