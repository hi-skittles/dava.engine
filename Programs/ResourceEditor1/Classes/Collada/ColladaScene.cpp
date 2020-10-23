#include "stdafx.h"
#include "ColladaScene.h"
#include "DAVAEngine.h"

namespace DAVA
{
ColladaScene::ColladaScene(FCDSceneNode* _rootNode)
{
    rootFCDNode = _rootNode;
    rootNode = new ColladaSceneNode(this, rootFCDNode);
    exportSceneLevel = 0;
    currentTime = 0.0f;
}

ColladaScene::~ColladaScene()
{
    SafeDelete(rootNode);
}

void ColladaScene::ExportAnimations(ColladaAnimation* colladaAnimation, FCDSceneNode* currentNode, float32 anStart, float32 anEnd)
{
    ColladaSceneNode* node = rootNode->FindNode(currentNode->GetDaeId());
    if (node)
    {
        //printf("- founded equal node: %s", currentNode->GetDaeId().c_str());
        SceneNodeAnimation* anim = ColladaSceneNode::ExportNodeAnimation(currentNode, anStart, anEnd, 30.0f);
        if (anim)
        {
            colladaAnimation->animations[node] = anim;
            ColladaAnimation::ExportAnimationData(node, &colladaAnimation->animationsData[node]);
        }
    }

    for (int i = 0; i < (int)currentNode->GetChildrenCount(); i++)
    {
        FCDSceneNode* childNode = currentNode->GetChild(i);
        ExportAnimations(colladaAnimation, childNode, anStart, anEnd);
    }
}

void ColladaScene::ExportScene(FCDSceneNode* fcdNode /* = 0 */, ColladaSceneNode* node /* = 0 */)
{
    exportSceneLevel++;

    if (fcdNode == nullptr)
        fcdNode = rootFCDNode;

    if (node == nullptr)
        node = rootNode;

    node->UpdateTransforms(0.0f);

    for (size_t i = 0; i < fcdNode->GetInstanceCount(); ++i)
    {
        FCDEntityInstance* instance = fcdNode->GetInstance(i);
        FCDEntity* entity = instance->GetEntity();
        if (entity == nullptr)
            continue;

        fm::string name = entity->GetDaeId();

        for (int space = 0; space < exportSceneLevel; ++space)
            printf(" ");
        printf(" - instance: %s\n", name.c_str());

        ColladaMesh* mesh = FindMeshWithName(name);
        if (mesh)
        {
            ColladaMeshInstance* meshInstance = CreateMeshInstance(mesh, dynamic_cast<FCDGeometryInstance*>(instance));
            node->AddMeshInstance(meshInstance);
        }

        ColladaSkinnedMesh* sMesh = FindSkinnedMeshWithName(name);
        if (sMesh)
        {
            if (sMesh->mesh)
            {
                ColladaMeshInstance* meshInstance = CreateMeshInstance(sMesh->mesh, dynamic_cast<FCDGeometryInstance*>(instance), sMesh);
                node->AddMeshInstance(meshInstance);
            }
        }

        ColladaLight* light = FindLightWithName(name);
        if (light)
        {
            node->AddLight(light);
        }

        ColladaCamera* aCam = FindCameraWithName(name);
        if (aCam)
        {
            node->AddCamera(aCam);
        }
    }

    for (int i = 0; i < (int)fcdNode->GetChildrenCount(); i++)
    {
        FCDSceneNode* fcdChildNode = fcdNode->GetChild(i);
        ColladaSceneNode* childNode = new ColladaSceneNode(this, fcdChildNode);
        ExportScene(fcdChildNode, childNode);
        node->AddNode(childNode);
    }
    exportSceneLevel--;
}

void ColladaScene::Render()
{
    SetupDefaultLights();

    ColladaLightState state;

    rootNode->PreProcessLights(state);
    if ((state.globalAmbientalLight[0] > 0.0f) || (state.globalAmbientalLight[1] > 0.0f) || (state.globalAmbientalLight[2] > 0.0f))
    {
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, state.globalAmbientalLight);

        // draw a square at the center that represents the ambient light
        //if (show_lights)
        {
            glDisable(GL_LIGHTING);
            glColor3f(0.984375, 0.078125, 0.64453125);
            //glutWireCube(1.0f);
            glEnable(GL_LIGHTING);
            glColor3f(1.0f, 1.0f, 1.0f);
        }
    }

    //	printf("Light Count: %d\n", state.lightIndex);

    currentTime += SystemTimer::GetFrameDelta();
    if (currentTime >= animationEndTime)
        currentTime = 0;

    rootNode->UpdateTransforms(currentTime);
    for (int ameshIndex = 0; ameshIndex < (int)colladaSkinnedMeshes.size(); ++ameshIndex)
    {
        ColladaSkinnedMesh* animMesh = colladaSkinnedMeshes[ameshIndex];
        animMesh->UpdateSkinnedMesh(currentTime);
    }

    Matrix4 base;
    base.Identity();
    rootNode->Render(base);

    RenderAxes();
}

ColladaMesh* ColladaScene::FindMeshWithName(const fm::string& name)
{
    for (int m = 0; m < (int)colladaMeshes.size(); ++m)
    {
        if (colladaMeshes[m]->mesh->GetDaeId() == name)
        {
            return colladaMeshes[m];
        }
    }

    return nullptr;
}

ColladaMaterial* ColladaScene::FindMaterialWithName(const fm::string& name)
{
    for (int m = 0; m < (int)colladaMaterials.size(); ++m)
    {
        if (colladaMaterials[m]->material->GetDaeId() == name)
        {
            return colladaMaterials[m];
        }
    }

    return nullptr;
}

ColladaTexture* ColladaScene::FindTextureWithName(const fm::string& name)
{
    for (int m = 0; m < (int)colladaTextures.size(); ++m)
    {
        if (colladaTextures[m]->image->GetDaeId() == name)
        {
            return colladaTextures[m];
        }
    }

    return nullptr;
}

ColladaLight* ColladaScene::FindLightWithName(const fm::string& name)
{
    for (int m = 0; m < (int)colladaLights.size(); ++m)
    {
        if (colladaLights[m]->light->GetDaeId() == name)
        {
            return colladaLights[m];
        }
    }

    return nullptr;
}

ColladaSkinnedMesh* ColladaScene::FindSkinnedMeshWithName(const fm::string& name)
{
    for (int m = 0; m < (int)colladaSkinnedMeshes.size(); ++m)
    {
        if (colladaSkinnedMeshes[m]->controller->GetDaeId() == name)
        {
            return colladaSkinnedMeshes[m];
        }
    }

    return nullptr;
}

int ColladaScene::FindMaterialIndex(ColladaMaterial* material)
{
    for (int m = 0; m < (int)colladaMaterials.size(); ++m)
    {
        if (material == colladaMaterials[m])
        {
            return m;
        }
    }

    return -1;
}

int ColladaScene::FindMeshIndex(ColladaMesh* mesh)
{
    for (int m = 0; m < (int)colladaMeshes.size(); ++m)
    {
        if (mesh == colladaMeshes[m])
        {
            return m;
        }
    }

    return -1;
}
bool ColladaScene::FindPolyGroupIndex(ColladaPolygonGroup* group, int& meshIndex, int& polygroupIndex)
{
    for (int m = 0; m < (int)colladaMeshes.size(); ++m)
    {
        ColladaMesh* mesh = colladaMeshes[m];
        for (int p = 0; p < (int)mesh->polygons.size(); ++p)
        {
            if (mesh->polygons[p] == group)
            {
                meshIndex = m;
                polygroupIndex = p;
                return true;
            }
            //printf("- polygroup: 0x%08x\n", mesh->polygons[p]);
        }
    }

    for (int m = 0; m < (int)colladaSkinnedMeshes.size(); ++m)
    {
        ColladaSkinnedMesh* mesh = colladaSkinnedMeshes[m];
        for (int p = 0; p < (int)mesh->mesh->polygons.size(); ++p)
        {
            if (mesh->mesh->polygons[p] == group)
            {
                meshIndex = m;
                polygroupIndex = p;
                return true;
            }
            //printf("- polygroup: 0x%08x\n", mesh->polygons[p]);
        }
    }
    return false;
}

ColladaMeshInstance* ColladaScene::CreateMeshInstance(ColladaMesh* mesh, FCDGeometryInstance* geometryInstance, ColladaSkinnedMesh* skinned)
{
    ColladaMeshInstance* meshInstance = new ColladaMeshInstance(skinned);
    meshInstance->geometryInstance = geometryInstance;

    // now the most difficult, material binding
    for (int i = 0; i < mesh->GetPolygonGroupCount(); i++)
    {
        fstring polygonMaterialSemantic = mesh->GetPolygonGroup(i)->GetMaterialSemantic();

        ColladaMaterial* material = 0;

        // look for this material_semantic in geometry_instance
        for (int k = 0; k < (int)geometryInstance->GetMaterialInstanceCount(); k++)
        {
            // look for this material in my material lib, so I store a pointer
            FCDMaterialInstance* materialInstance = geometryInstance->GetMaterialInstance(k);

            fstring materialSemantic = materialInstance->GetSemantic();

            if (materialSemantic == polygonMaterialSemantic)
            {
                auto colladaMaterial = materialInstance->GetMaterial();
                if (colladaMaterial != nullptr)
                {
                    const fm::string& materialId = colladaMaterial->GetDaeId();
                    material = FindMaterialWithName(materialId);
                }
                else
                {
                    printf("\n[ERROR] no material for %ls in polygon group %ls inside mesh %s\n",
                           materialSemantic.c_str(), polygonMaterialSemantic.c_str(), mesh->name.c_str());
                }
            }
        }

        printf(" - mesh instance: %s ", mesh->mesh->GetDaeId().c_str());
        if (material != nullptr)
        {
            printf(" material: %s", material->material->GetDaeId().c_str());
            if (material->hasDiffuseTexture)
            {
                wprintf(L" diffuse texture: %ls\n", (wchar_t*)(material->diffuseTexture->image->GetFilename().c_str()));
            }
            printf("\n");
        }
        ColladaPolygonGroup* polyGroup = mesh->GetPolygonGroup(i);
        printf("- mesh instance added polygroup: %p %d\n", polyGroup, i);

        ColladaPolygonGroupInstance* polygonGroupInstance = new ColladaPolygonGroupInstance(polyGroup, material);
        meshInstance->AddPolygonGroupInstance(polygonGroupInstance);
    }

    return meshInstance;
}

void ColladaScene::SetupDefaultLights()
{
    // enable lighting
    glEnable(GL_LIGHTING);

    // deactivate all lights
    for (int i = 0; i < 8; i++) glDisable(GL_LIGHT0 + i);

    // ambiental light to nothing
    GLfloat ambientalLight[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientalLight);

    GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    GLfloat light_diffuse[4];
    light_diffuse[0] = 1.0f;
    light_diffuse[1] = 1.0f;
    light_diffuse[2] = 1.0f;
    light_diffuse[3] = 1.0f;

    GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f };

    // activate this light
    glEnable(GL_LIGHT0);

    //always position 0,0,0 because light  is moved with transformations
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // colors
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_diffuse); // now like diffuse color
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    //specific values for this light
    glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 1);
    glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0);
    glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0);

    //other values
    glLightf(GL_LIGHT0, GL_SPOT_CUTOFF, 180.0f);
    glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, 0.0f);
    GLfloat spotdirection[] = { 0.0f, 0.0f, -1.0f, 0.0f }; // irrelevant for this light (I guess)
    glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, spotdirection);
}

void ColladaScene::RenderAxes()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f); // x is red
    glVertex3f(0.0f, 0.0f, 0.0f); // origin of the line
    glVertex3f(50.0f, 0.0f, 0.0f); // ending point of the line

    glColor3f(0.0f, 1.0f, 0.0f); // y green
    glVertex3f(0.0f, 0.0f, 0.0f); // origin of the line
    glVertex3f(0.0f, 50.0f, 0.0f); // ending point of the line

    glColor3f(0.0f, 0.0f, 1.0f); // z blue
    glVertex3f(0.0f, 0.0f, 0.0f); // origin of the line
    glVertex3f(0.0f, 0.0f, 50.0f); // ending point of the line
    glEnd();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);
};

void ColladaScene::RenderGrid()
{
    float size = 10;

    glPushMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(0.5f, 0.5f, 0.5f);
    // grid is 1x1, change its size
    glScalef(size, 0.0f, size);
    // center this grid
    glTranslatef(-0.5f, 0.0f, -0.5f);

    float squares_per_side = 30;
    // new grid version, easier than
    float pass = 1.0f / (float)squares_per_side;

    for (int i = 0; i <= squares_per_side; i++)
    {
        glBegin(GL_LINES);
        glVertex3f(pass * i, 0.0f, 0.0f);
        glVertex3f(pass * i, 0.0f, 1.0f);
        glEnd();

        glBegin(GL_LINES);
        glVertex3f(0.0f, 0.0f, pass * i);
        glVertex3f(1.0f, 0.0f, pass * i);
        glEnd();
    }
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

ColladaCamera* ColladaScene::FindCameraWithName(const fm::string& name)
{
    for (int m = 0; m < (int)colladaCameras.size(); ++m)
        if (colladaCameras[m]->camera->GetDaeId() == name)
        {
            return colladaCameras[m];
        }
    return 0;
}

int ColladaScene::FindCameraIndex(ColladaCamera* cam)
{
    for (int m = 0; m < (int)colladaCameras.size(); ++m)
        if (cam == colladaCameras[m])
        {
            return m;
        }

    return -1;
}

void ColladaScene::SetExclusiveAnimation(int32 index)
{
    if (index >= (int)colladaAnimations.size())
        return;
    rootNode->SetAnimation(0, true);
    colladaAnimations[index]->Assign();
    animationEndTime = colladaAnimations[index]->duration;
    currentTime = 0.0f;
    printf("start animation: %s duration: %f\n", colladaAnimations[index]->name.c_str(), colladaAnimations[index]->duration);
}
};
