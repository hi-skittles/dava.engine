#include "Classes/Export/Mitsuba/MitsubaExporter.h"
#include "Classes/Export/Mitsuba/Private/MitsubaExporterTools.h"

#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <TArc/Utils/ModuleCollection.h>
#include <TArc/WindowSubSystem/ActionUtils.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/WindowSubSystem/UI.h>

#include <Base/RefPtr.h>
#include <Base/String.h>
#include <FileSystem/FileSystem.h>
#include <Functional/Function.h>
#include <Render/Highlevel/Landscape.h>
#include <Render/Highlevel/LandscapeThumbnails.h>
#include <Render/Highlevel/Light.h>
#include <Render/Highlevel/RenderBatch.h>
#include <Render/Highlevel/RenderObject.h>
#include <Render/Image/Image.h>
#include <Render/Image/ImageSystem.h>
#include <Render/Material/NMaterial.h>
#include <Render/Texture.h>
#include <Render/TextureDescriptor.h>
#include <Scene3D/Components/ComponentHelpers.h>
#include <Scene3D/Components/LightComponent.h>
#include <Scene3D/Components/TransformComponent.h>

namespace MitsubaExporterDetail
{
struct TextureExport
{
    DAVA::Texture* texture = nullptr;
    DAVA::FilePath originalFilePath;
    DAVA::String destinationFilePath;
    DAVA::float32 uScale = 1.0f;
    DAVA::float32 vScale = 1.0f;
};

struct MaterialExport
{
    DAVA::NMaterial* material = nullptr;
    DAVA::Map<DAVA::String, DAVA::String> textures;
    DAVA::String bumpmapTextureId;
    DAVA::String alphaTestTextureId;
    DAVA::String materialType;
    bool hasBumpmap = false;
    bool alphaTestMaterial = false;
};

struct RenderBatchExport
{
    DAVA::RenderBatch* rb = nullptr;
    DAVA::Matrix4 transform;
    DAVA::String material;
};

struct LightExport
{
    DAVA::LightComponent* light = nullptr;
};

class Exporter
{
public:
    static const DAVA::uint32 MeshExporterVersion = 1;

    DAVA::String exportFolder;
    DAVA::String meshesFolder;
    DAVA::String texturesFolder;
    DAVA::UnorderedMap<DAVA::String, MitsubaExporterDetail::TextureExport> texturesToExport;
    DAVA::UnorderedMap<DAVA::String, MitsubaExporterDetail::MaterialExport> materialsToExport;
    DAVA::Vector<std::pair<DAVA::String, MitsubaExporterDetail::RenderBatchExport>> renderBatchesToExport;
    DAVA::UnorderedMap<DAVA::String, MitsubaExporterDetail::LightExport> lightsToExport;
    DAVA::Landscape* landscapeToExport = nullptr;

    void CollectExportObjects(const DAVA::Entity* entity);
    bool Export(DAVA::Scene*, const DAVA::FilePath&);

    void ExportTexture(const DAVA::String& name, const MitsubaExporterDetail::TextureExport& tex);
    void ExportMaterial(const DAVA::String& name, MitsubaExporterDetail::MaterialExport mat);
    void ExportLandscape(DAVA::Landscape* landscape);

    void ExportCamera(const DAVA::Camera* cam, const DAVA::Size2i& viewport);
    void ExportBatch(const DAVA::String& name, const MitsubaExporterDetail::RenderBatchExport& rb);
    void ExportLight(DAVA::LightComponent* light);

    bool RenderObjectsIsValidForExport(DAVA::RenderObject* object);
    bool RenderBatchIsValidForExport(DAVA::RenderBatch* batch, DAVA::int32 lodIndex, DAVA::int32 swIndex);
    bool MaterialIsValidForExport(DAVA::NMaterial* material);

    void CollectMaterialTextures(MitsubaExporterDetail::MaterialExport& material);
    bool AddTextureToExport(DAVA::Texture*, DAVA::String& textureId);
};

static const DAVA::String FileExtension = ".xml";
void LandscapeThumbnailCallback(DAVA::String fileName, DAVA::Landscape* landscape, DAVA::Texture* landscapeTexture);
}

void MitsubaExporter::PostInit()
{
    using namespace DAVA;

    QtAction* exportAction = new QtAction(GetAccessor(), "Export to Mitsuba...");
    FieldDescriptor fieldDescriptor(DAVA::ReflectedTypeDB::Get<SceneData>(), DAVA::FastName(SceneData::scenePropertyName));
    exportAction->SetStateUpdationFunction(QtAction::Enabled, fieldDescriptor, [](const DAVA::Any& value) -> DAVA::Any
                                           {
                                               return value.CanCast<SceneData::TSceneType>();
                                           });

    GetUI()->AddAction(DAVA::mainWindowKey, CreateMenuPoint("DebugFunctions"), exportAction);
    connections.AddConnection(exportAction, &QAction::triggered, DAVA::MakeFunction(this, &MitsubaExporter::Export));
}

void MitsubaExporter::Export()
{
    using namespace DAVA;

    FileDialogParams parameters;
    parameters.title = "Export to";
    parameters.filters = "XML Files (*.xml)";
    QString exportFolder = GetUI()->GetSaveFileName(DAVA::mainWindowKey, parameters);
    if (!exportFolder.isEmpty())
    {
        DAVA::FilePath exportFile(exportFolder.toUtf8().toStdString());
        if (exportFile.GetExtension() != MitsubaExporterDetail::FileExtension)
        {
            exportFile.ReplaceExtension(MitsubaExporterDetail::FileExtension);
        }

        MitsubaExporterDetail::Exporter exporter;
        SceneData* sceneData = GetAccessor()->GetActiveContext()->GetData<SceneData>();
        exporter.Export(sceneData->GetScene().Get(), exportFile);
    }
}

bool MitsubaExporterDetail::Exporter::Export(DAVA::Scene* scene, const DAVA::FilePath& toFile)
{
    exportFolder = toFile.GetDirectory().GetAbsolutePathname();

    meshesFolder = toFile.GetBasename() + "_meshes";
    DAVA::FileSystem::Instance()->CreateDirectory(toFile.GetDirectory() + meshesFolder, true);

    texturesFolder = toFile.GetBasename() + "_textures";
    DAVA::FileSystem::Instance()->CreateDirectory(toFile.GetDirectory() + texturesFolder, true);

    CollectExportObjects(scene);

    if (renderBatchesToExport.empty())
        return false;

    std::ofstream fOut(toFile.GetAbsolutePathname());
    mitsuba::currentOutput = &fOut;
    {
        fOut << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
        mitsuba::scope sceneTag("scene", DAVA::String("version"), DAVA::String("0.5.0"));
        {
            mitsuba::scope integrator("integrator", mitsuba::kType, DAVA::String("volpath"));
        }
        DAVA::Size2i vpSize(scene->GetMainPassConfig().viewport.width, scene->GetMainPassConfig().viewport.height);
        ExportCamera(scene->GetCurrentCamera(), vpSize);

        for (const auto& tex : texturesToExport)
        {
            ExportTexture(tex.first, tex.second);
        }

        for (const auto& lit : lightsToExport)
        {
            ExportLight(lit.second.light);
        }

        for (const auto& mat : materialsToExport)
        {
            ExportMaterial(mat.first, mat.second);
        }

        for (const auto& rb : renderBatchesToExport)
        {
            ExportBatch(rb.first, rb.second);
        }

        if (landscapeToExport != nullptr)
        {
            ExportLandscape(landscapeToExport);
        }
    }
    fOut.flush();
    fOut.close();
    mitsuba::currentOutput = nullptr;

    // hardcoded for now
    return true;
}

void MitsubaExporterDetail::Exporter::ExportCamera(const DAVA::Camera* cam, const DAVA::Size2i& viewport)
{
    DAVA::String origin = DAVA::Format("%f, %f, %f", cam->GetPosition().x, cam->GetPosition().y, cam->GetPosition().z);
    DAVA::String target = DAVA::Format("%f, %f, %f", cam->GetTarget().x, cam->GetTarget().y, cam->GetTarget().z);
    DAVA::String up = DAVA::Format("%f, %f, %f", cam->GetUp().x, cam->GetUp().y, cam->GetUp().z);

    mitsuba::scope sensor("sensor", mitsuba::kType, DAVA::String("perspective"));
    mitsuba::tag(mitsuba::kFloat, mitsuba::kName, DAVA::String("fov"), mitsuba::kValue, cam->GetFOV());
    {
        mitsuba::scope sampler("sampler", mitsuba::kType, DAVA::String("independent"));
        mitsuba::tag(mitsuba::kInteger, mitsuba::kName, DAVA::String("sampleCount"), mitsuba::kValue, 64);
    }
    {
        mitsuba::scope transform("transform", mitsuba::kName, DAVA::String("toWorld"));
        mitsuba::tag("lookat", DAVA::String("origin"), origin, DAVA::String("target"), target, DAVA::String("up"), up);
    }
    {
        mitsuba::scope film("film", mitsuba::kType, DAVA::String("hdrfilm"));
        mitsuba::tag(mitsuba::kInteger, mitsuba::kName, DAVA::String("width"), mitsuba::kValue, viewport.dx);
        mitsuba::tag(mitsuba::kInteger, mitsuba::kName, DAVA::String("height"), mitsuba::kValue, viewport.dy);
        {
            mitsuba::scope rfilter("rfilter", mitsuba::kType, DAVA::String("gaussian"));
            mitsuba::tag(mitsuba::kFloat, mitsuba::kName, DAVA::String("stddev"), mitsuba::kValue, 0.25);
        }
    }
}

void MitsubaExporterDetail::Exporter::ExportBatch(const DAVA::String& name, const MitsubaExporterDetail::RenderBatchExport& rb)
{
    DAVA::String uniqueName = DAVA::Format("%s/%u_%s.obj", meshesFolder.c_str(), MeshExporterVersion, name.c_str(), reinterpret_cast<DAVA::uint64>(rb.rb));

    DAVA::String outFile = exportFolder + uniqueName;
    if (!DAVA::FileSystem::Instance()->Exists(outFile))
    {
        DAVA::PolygonGroup* poly = rb.rb->GetPolygonGroup();
        if (poly == nullptr)
        {
            DAVA::Logger::Info("Object %s does not contain polygon group.", name.c_str());
            return;
        }
        DAVA::int32 vertexCount = poly->GetVertexCount();
        DAVA::int32 vertexFormat = poly->GetFormat();

        if ((vertexFormat & DAVA::EVF_NORMAL) == 0)
        {
            DAVA::Logger::Error("%s data does not contain normals", name.c_str());
            return;
        }

        bool hasPivot = (vertexFormat & DAVA::EVF_PIVOT4) == DAVA::EVF_PIVOT4;
        if (hasPivot)
        {
            DAVA::Logger::Error("%s data contains pivot point, which are not supported", name.c_str());
        }

        bool hasTexCoords = (vertexFormat & DAVA::EVF_TEXCOORD0) == DAVA::EVF_TEXCOORD0;

        std::ofstream fOut(outFile.c_str(), std::ios::out);
        for (DAVA::int32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            DAVA::Vector3 v;
            poly->GetCoord(vertexIndex, v);
            fOut << DAVA::Format("v %.7f %.7f %.7f\n", v.x, v.y, v.z);

            DAVA::Vector3 n;
            poly->GetNormal(vertexIndex, n);
            fOut << DAVA::Format("vn %.7f %.7f %.7f\n", n.x, n.y, n.z);

            if (hasTexCoords)
            {
                DAVA::Vector2 t;
                poly->GetTexcoord(0, vertexIndex, t);
                fOut << DAVA::Format("vt %.7f %.7f\n", t.x, 1.0f - t.y);
            }
        }

        fOut << std::endl
             << DAVA::Format("g group_%llu_%llu", poly->GetNodeID(), reinterpret_cast<DAVA::uint64>(poly)) << std::endl;
        for (DAVA::int32 ti = 0, te = poly->GetPrimitiveCount(); ti < te; ++ti)
        {
            DAVA::int32 tri[3] = {};
            poly->GetIndex(3 * ti + 0, tri[0]);
            poly->GetIndex(3 * ti + 1, tri[1]);
            poly->GetIndex(3 * ti + 2, tri[2]);
            if (hasTexCoords)
            {
                fOut << "f " <<
                tri[0] + 1 << "/" << tri[0] + 1 << "/" << tri[0] + 1 << mitsuba::kSpace <<
                tri[1] + 1 << "/" << tri[1] + 1 << "/" << tri[1] + 1 << mitsuba::kSpace <<
                tri[2] + 1 << "/" << tri[2] + 1 << "/" << tri[2] + 1 << mitsuba::kSpace << std::endl;
            }
            else
            {
                fOut << "f " <<
                tri[0] + 1 << "//" << tri[0] + 1 << mitsuba::kSpace <<
                tri[1] + 1 << "//" << tri[1] + 1 << mitsuba::kSpace <<
                tri[2] + 1 << "//" << tri[2] + 1 << mitsuba::kSpace << std::endl;
            }
        }
        fOut.flush();
        fOut.close();
    }

    // write to Mitsuba xml
    mitsuba::scope shape("shape", mitsuba::kType, DAVA::String("obj"));
    mitsuba::tag("ref", mitsuba::kId, rb.material);
    mitsuba::tag(mitsuba::kString, mitsuba::kName, DAVA::String("filename"), mitsuba::kValue, uniqueName);
    {
        const float* w = rb.transform.data;
        mitsuba::scope transform("transform", mitsuba::kName, DAVA::String("toWorld"));
        mitsuba::tag("matrix", mitsuba::kValue, DAVA::Format(
                                                "%.7f %.7f %.7f %.7f "
                                                "%.7f %.7f %.7f %.7f "
                                                "%.7f %.7f %.7f %.7f "
                                                "%.7f %.7f %.7f %.7f ",
                                                w[0], w[4], w[8], w[12],
                                                w[1], w[5], w[9], w[13],
                                                w[2], w[6], w[10], w[14],
                                                w[3], w[7], w[11], w[15])
                     );
    }
}

void MitsubaExporterDetail::Exporter::ExportLight(DAVA::LightComponent* light)
{
    if (light->IsDynamic())
    {
        DAVA::Logger::Warning("Dynamic lights not supported yet");
        return;
    }

    DAVA::Light::eType lightType = static_cast<DAVA::Light::eType>(light->GetLightType());
    if (lightType == DAVA::Light::TYPE_POINT)
    {
        mitsuba::scope emitter("emitter", mitsuba::kType, DAVA::String("point"));
        DAVA::Vector3 pos = light->GetPosition();
        mitsuba::tag("spectrum", mitsuba::kName, DAVA::String("intensity"), mitsuba::kValue, 10.0f * light->GetIntensity());
        mitsuba::tag("point", mitsuba::kName, DAVA::String("position"), DAVA::String("x"), pos.x, DAVA::String("y"), pos.y, DAVA::String("z"), pos.z);
    }
    else if (lightType == DAVA::Light::TYPE_DIRECTIONAL)
    {
        mitsuba::scope emitter("emitter", mitsuba::kType, DAVA::String("sunsky"));
        DAVA::Vector3 dir = -light->GetDirection();
        mitsuba::tag(mitsuba::kFloat, mitsuba::kName, DAVA::String("sunScale"), mitsuba::kValue, 2.7f);
        mitsuba::tag(mitsuba::kFloat, mitsuba::kName, DAVA::String("skyScale"), mitsuba::kValue, 2.7f);
        mitsuba::tag("vector", mitsuba::kName, DAVA::String("sunDirection"), DAVA::String("x"), dir.x, DAVA::String("y"), dir.y, DAVA::String("z"), dir.z);
        mitsuba::scope transform("transform", mitsuba::kName, DAVA::String("toWorld"));
        mitsuba::tag("rotate", DAVA::String("x"), 1, DAVA::String("angle"), 90);
    }
    else if (lightType == DAVA::Light::TYPE_SKY)
    {
        DAVA::Logger::Warning("Sky emitter skipped");
    }
    else
    {
        DAVA::Logger::Warning("Unsupported light type: %d (IsDynamic = %d)", static_cast<DAVA::int32>(lightType),
                              static_cast<DAVA::int32>(light->IsDynamic()));
    }
}

void MitsubaExporterDetail::Exporter::ExportLandscape(DAVA::Landscape* landscape)
{
    DAVA::Vector<DAVA::Landscape::LandscapeVertex> vertices;
    DAVA::Vector<DAVA::int32> indices;
    DAVA::Vector<DAVA::Vector3> normals;

    if (!landscape->GetLevel0Geometry(vertices, indices))
        return;

    DVASSERT(indices.size() % 3 == 0);

    DAVA::String landscapeTextureName = DAVA::Format("%s%s/landscape.png", exportFolder.c_str(), texturesFolder.c_str(), reinterpret_cast<DAVA::uint64>(landscape));
    auto callback = DAVA::Bind(&MitsubaExporterDetail::LandscapeThumbnailCallback, landscapeTextureName, std::placeholders::_1, std::placeholders::_2);
    DAVA::LandscapeThumbnails::Create(landscape, callback);

    {
        // compute landscape normals
        normals.resize(vertices.size());
        for (DAVA::size_type i = 0, e = indices.size() / 3; i < e; ++i)
        {
            DAVA::int32 i0 = indices.at(3 * i + 0);
            DAVA::int32 i1 = indices.at(3 * i + 1);
            DAVA::int32 i2 = indices.at(3 * i + 2);
            const DAVA::Vector3& p1 = vertices[i0].position;
            const DAVA::Vector3& p2 = vertices[i1].position;
            const DAVA::Vector3& p3 = vertices[i2].position;
            DAVA::Vector3 normal = (p2 - p1).CrossProduct(p3 - p1);
            DVASSERT(normal.Length() > std::numeric_limits<float>::epsilon(), "Landscape contains degenerate triangles");
            normals[i0] += normal;
            normals[i1] += normal;
            normals[i2] += normal;
        }
        for (auto& n : normals)
        {
            n.Normalize();
        }
    }

    DAVA::String uniqueName = DAVA::Format("%s/landscape.obj", meshesFolder.c_str(), reinterpret_cast<DAVA::uint64>(landscape));
    DAVA::String outFile = exportFolder + uniqueName;
    std::ofstream fOut(outFile.c_str(), std::ios::out);
    for (const auto& v : vertices)
    {
        fOut << DAVA::Format("v %.7f %.7f %.7f\n", v.position.x, v.position.y, v.position.z);
        fOut << DAVA::Format("vt %.7f %.7f\n", v.texCoord.x, v.texCoord.y);
    }
    for (const auto& v : normals)
    {
        fOut << DAVA::Format("vn %.7f %.7f %.7f\n", v.x, v.y, v.z);
    }
    fOut << "\ng landscape\n";
    for (DAVA::uint64 i = 0, e = indices.size() / 3; i < e; ++i)
    {
        DAVA::int32 i0 = 1 + indices.at(3 * i + 0);
        DAVA::int32 i1 = 1 + indices.at(3 * i + 1);
        DAVA::int32 i2 = 1 + indices.at(3 * i + 2);
        fOut << DAVA::Format("f %d/%d/%d %d/%d/%d %d/%d/%d\n", i0, i0, i0, i1, i1, i1, i2, i2, i2);
    }
    fOut.flush();
    fOut.close();

    mitsuba::scope shape("shape", mitsuba::kType, DAVA::String("obj"));
    mitsuba::tag(mitsuba::kString, mitsuba::kName, DAVA::String("filename"), mitsuba::kValue, uniqueName);
    {
        mitsuba::scope bsdf("bsdf", mitsuba::kType, DAVA::String("diffuse"));
        mitsuba::scope texture("texture", mitsuba::kType, DAVA::String("bitmap"), mitsuba::kName, DAVA::String("reflectance"));
        mitsuba::tag(mitsuba::kString, mitsuba::kName, DAVA::String("filename"), mitsuba::kValue, landscapeTextureName);
    }
}

void MitsubaExporterDetail::Exporter::CollectExportObjects(const DAVA::Entity* entity)
{
    DAVA::RenderObject* renderObject = DAVA::GetRenderObject(entity);
    if (RenderObjectsIsValidForExport(renderObject))
    {
        if (renderObject->GetType() == DAVA::RenderObject::TYPE_LANDSCAPE)
        {
            landscapeToExport = static_cast<DAVA::Landscape*>(renderObject);
        }
        else
        {
            DAVA::TransformComponent* tc = entity->GetComponent<DAVA::TransformComponent>();
            for (DAVA::uint32 i = 0, e = renderObject->GetRenderBatchCount(); i < e; ++i)
            {
                DAVA::int32 lodIndex = -1;
                DAVA::int32 swIndex = -1;
                DAVA::RenderBatch* batch = renderObject->GetRenderBatch(i, lodIndex, swIndex);
                if (RenderBatchIsValidForExport(batch, lodIndex, swIndex))
                {
                    DAVA::NMaterial* material = batch->GetMaterial();
                    DAVA::String materialID = DAVA::Format("mat_%p", material);

                    materialsToExport[materialID].material = material;
                    CollectMaterialTextures(materialsToExport[materialID]);

                    DAVA::String batchID = DAVA::Format("%s_batch_%u", entity->GetName().c_str(), i);
                    renderBatchesToExport.emplace_back(batchID, MitsubaExporterDetail::RenderBatchExport());
                    renderBatchesToExport.back().second.rb = batch;
                    renderBatchesToExport.back().second.material = materialID;
                    renderBatchesToExport.back().second.transform = tc->GetWorldMatrix();
                }
            }
        }
    }

    DAVA::LightComponent* lightComponent = DAVA::GetLightComponent(entity);
    if (lightComponent != nullptr)
    {
        DAVA::String lightID = DAVA::Format("light_%p", lightComponent);
        lightsToExport[lightID].light = lightComponent;
    }

    for (DAVA::int32 i = 0, e = entity->GetChildrenCount(); i < e; ++i)
    {
        CollectExportObjects(entity->GetChild(i));
    }
}

bool MitsubaExporterDetail::Exporter::RenderObjectsIsValidForExport(DAVA::RenderObject* object)
{
    static const DAVA::Set<DAVA::RenderObject::eType> validRenderObjectTypes =
    {
      DAVA::RenderObject::TYPE_RENDEROBJECT,
      DAVA::RenderObject::TYPE_MESH,
      DAVA::RenderObject::TYPE_LANDSCAPE,
      DAVA::RenderObject::TYPE_SPEED_TREE
    };

    return (object != nullptr) && (validRenderObjectTypes.count(object->GetType()) > 0);
}

bool MitsubaExporterDetail::Exporter::RenderBatchIsValidForExport(DAVA::RenderBatch* batch, DAVA::int32 lodIndex, DAVA::int32 swIndex)
{
    if ((batch == nullptr) || (lodIndex > 0) || (swIndex > 0))
        return false;

    return MaterialIsValidForExport(batch->GetMaterial());
}

bool MitsubaExporterDetail::Exporter::MaterialIsValidForExport(DAVA::NMaterial* material)
{
    if (material == nullptr)
        return false;

    if (material->GetEffectiveFXName().find("Sky") != DAVA::String::npos)
        return false;

    if (material->GetEffectiveFXName().find("Leaf") != DAVA::String::npos)
        return false;

    if (material->GetEffectiveFXName().find("Shadow") != DAVA::String::npos)
        return false;

    return true;
}

void MitsubaExporterDetail::Exporter::CollectMaterialTextures(MitsubaExporterDetail::MaterialExport& material)
{
    DAVA::FastName fxName = material.material->GetEffectiveFXName();
    material.alphaTestMaterial = fxName.find("Alphatest") != DAVA::String::npos;

    material.materialType = mitsuba::kDiffuse;

    if (fxName.find("Water") != DAVA::String::npos)
        material.materialType = mitsuba::kDielectric;

    if (fxName.find("Normalized") != DAVA::String::npos)
        material.materialType = mitsuba::kRoughConductor;

    DAVA::String textureId;
    DAVA::Texture* diffuseTexture = material.material->GetEffectiveTexture(DAVA::NMaterialTextureName::TEXTURE_ALBEDO);
    if (AddTextureToExport(diffuseTexture, textureId))
    {
        material.alphaTestTextureId = textureId;
        if (material.materialType == mitsuba::kRoughConductor)
        {
            material.textures["specularReflectance"] = textureId;
        }
        else
        {
            material.textures["reflectance"] = textureId;
        }
    }
    else
    {
        material.alphaTestMaterial = false;
    }

    DAVA::Texture* normalMap = material.material->GetEffectiveTexture(DAVA::NMaterialTextureName::TEXTURE_NORMAL);
    if (AddTextureToExport(normalMap, textureId))
    {
        material.bumpmapTextureId = textureId;
        material.hasBumpmap = true;
    }
}

bool MitsubaExporterDetail::Exporter::AddTextureToExport(DAVA::Texture* texture, DAVA::String& textureId)
{
    if (texture == nullptr)
        return false;

    DAVA::FilePath sourceFile = texture->GetDescriptor()->GetSourceTexturePathname();
    if (DAVA::FileSystem::Instance()->Exists(sourceFile) == false)
        return false;

    DAVA::String baseName = sourceFile.GetFilename();
    textureId = DAVA::Format("%s/%s.png", texturesFolder.c_str(), baseName.c_str());
    texturesToExport[textureId].texture = texture;
    texturesToExport[textureId].originalFilePath = sourceFile;
    texturesToExport[textureId].destinationFilePath = exportFolder + textureId;

    return true;
}

void MitsubaExporterDetail::Exporter::ExportTexture(const DAVA::String& name, const MitsubaExporterDetail::TextureExport& tex)
{
    if (!DAVA::FileSystem::Instance()->Exists(tex.destinationFilePath))
    {
        DAVA::ScopedPtr<DAVA::Image> image(DAVA::ImageSystem::LoadSingleMip(tex.originalFilePath));
        DAVA::ImageSystem::Save(tex.destinationFilePath, image);
    }

    mitsuba::scope texture("texture", mitsuba::kType, DAVA::String("bitmap"), mitsuba::kId, name);
    mitsuba::tag(mitsuba::kString, mitsuba::kName, DAVA::String("filename"), mitsuba::kValue, tex.destinationFilePath);
    mitsuba::tag(mitsuba::kFloat, mitsuba::kName, DAVA::String("uscale"), mitsuba::kValue, tex.uScale);
    mitsuba::tag(mitsuba::kFloat, mitsuba::kName, DAVA::String("vscale"), mitsuba::kValue, tex.vScale);
}

void MitsubaExporterDetail::Exporter::ExportMaterial(const DAVA::String& name, MitsubaExporterDetail::MaterialExport mtl)
{
    if (mtl.alphaTestMaterial)
    {
        mitsuba::scope mask("bsdf", mitsuba::kType, DAVA::String("mask"), mitsuba::kId, name);
        {
            mitsuba::scope texture("texture", mitsuba::kName, DAVA::String("opacity"), mitsuba::kType, DAVA::String("bitmap"));
            mitsuba::tag(mitsuba::kString, mitsuba::kName, DAVA::String("filename"), mitsuba::kValue, mtl.alphaTestTextureId);
            mitsuba::tag(mitsuba::kString, mitsuba::kName, DAVA::String("channel"), mitsuba::kValue, DAVA::String("a"));
        }
        mitsuba::scope twosided("bsdf", mitsuba::kType, DAVA::String("twosided"), mitsuba::kId, "twosided_" + name);
        {
            MitsubaExporterDetail::MaterialExport subBSDF = mtl;
            subBSDF.alphaTestMaterial = false;
            ExportMaterial("sub_" + name, subBSDF);
        }
    }
    else if (mtl.hasBumpmap)
    {
        mitsuba::scope bump("bsdf", mitsuba::kType, mitsuba::kBumpmap, mitsuba::kId, name);
        {
            mitsuba::scope scale("texture", mitsuba::kType, DAVA::String("scale"));
            mitsuba::tag("ref", mitsuba::kName, mitsuba::kBumpmap, mitsuba::kId, mtl.bumpmapTextureId);
            mitsuba::tag(mitsuba::kFloat, mitsuba::kName, DAVA::String("scale"), mitsuba::kValue, 0.033333f);
        }
        MitsubaExporterDetail::MaterialExport subBSDF = mtl;
        subBSDF.hasBumpmap = false;
        ExportMaterial("sub_" + name, subBSDF);
    }
    else
    {
        mitsuba::scope bsdf("bsdf", mitsuba::kType, mtl.materialType, mitsuba::kId, name);
        if (mtl.materialType == mitsuba::kDielectric)
        {
            mitsuba::tag(mitsuba::kString, mitsuba::kName, DAVA::String("intIOR"), mitsuba::kValue, DAVA::String("air"));
            mitsuba::tag(mitsuba::kString, mitsuba::kName, DAVA::String("extIOR"), mitsuba::kValue, DAVA::String("water"));
        }
        else
        {
            if (mtl.materialType == mitsuba::kRoughConductor)
            {
                mitsuba::tag(mitsuba::kString, mitsuba::kName, DAVA::String("material"), mitsuba::kValue, DAVA::String("Ag"));
            }
            for (const auto& ref : mtl.textures)
            {
                mitsuba::tag("ref", mitsuba::kName, ref.first, mitsuba::kId, ref.second);
            }
        }
    }
}

void MitsubaExporterDetail::LandscapeThumbnailCallback(DAVA::String fileName, DAVA::Landscape* landscape, DAVA::Texture* landscapeTexture)
{
    DAVA::ScopedPtr<DAVA::Image> image(landscapeTexture->CreateImageFromMemory());
    DAVA::ImageSystem::Save(fileName, image);
}

DECL_TARC_MODULE(MitsubaExporter);
