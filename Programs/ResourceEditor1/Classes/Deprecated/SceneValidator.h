#pragma once

#include "DAVAEngine.h"

class SceneValidator
{
public:
    /*
     \brief Function to validate Scene errors
     \param[in] scene scene for validation
     \param[out] errorsLog set for validation errors
	 */
    void ValidateScene(DAVA::Scene* scene, const DAVA::FilePath& scenePath);

    /*
     \brief Function to find Scales in models transformations
     \param[in] scene scene for validation
     \param[out] errorsLog set for validation errors
	 */
    void ValidateScales(DAVA::Scene* scene);

    /*
     \brief Function to validate Texture errors
     \param[in] texture texture for validation
     \param[out] errorsLog set for validation errors
	 */

    void ValidateTexture(DAVA::Texture* texture, const DAVA::String& validatedObjectName);

    /*
     \brief Function to validate LandscapeNode errors
     \param[in] landscape landscape for validation
     \param[out] errorsLog set for validation errors
	 */
    void ValidateLandscape(DAVA::Landscape* landscape);

    /*
     \brief Function to validate Entity errors
     \param[in] sceneNode sceneNode for validation
     \param[out] errorsLog set for validation errors
	 */
    void ValidateSceneNode(DAVA::Entity* sceneNode);

    /*
     \brief Function to validate Materials errors
     \param[in] scene that has materials for validation
     \param[out] errorsLog set for validation errors
	 */
    void ValidateMaterials(DAVA::Scene* scene);

    /*
     \brief Function sets 3d folder path for checking texture pathnames
     \param[in] pathname path to DataSource/3d folder
     \return old path for checking
	 */

    void ValidateNodeCustomProperties(DAVA::Entity* sceneNode);

    DAVA::FilePath SetPathForChecking(const DAVA::FilePath& pathname);

    void EnumerateNodes(DAVA::Scene* scene);

    static bool IsTextureChanged(const DAVA::TextureDescriptor* descriptor, DAVA::eGPUFamily forGPU);
    static bool IsTextureChanged(const DAVA::FilePath& texturePathname, DAVA::eGPUFamily forGPU);

    bool ValidateTexturePathname(const DAVA::FilePath& pathForValidation);
    bool ValidateHeightmapPathname(const DAVA::FilePath& pathForValidation);

    bool IsPathCorrectForProject(const DAVA::FilePath& pathname);

    DAVA_DEPRECATED(static void FindSwitchesWithDifferentLODs(DAVA::Entity* entity, DAVA::Set<DAVA::FastName>& names));
    DAVA_DEPRECATED(static bool IsEntityHasDifferentLODsCount(DAVA::Entity* entity));
    DAVA_DEPRECATED(static bool IsObjectHasDifferentLODsCount(DAVA::RenderObject* renderObject));

    static void ExtractEmptyRenderObjects(DAVA::Entity* entity);

protected:
    void ValidateRenderComponent(DAVA::Entity* ownerNode);
    void ValidateRenderBatch(DAVA::Entity* ownerNode, DAVA::RenderBatch* renderBatch);

    void ValidateParticleEffectComponent(DAVA::Entity* ownerNode) const;
    void ValidateParticleEmitter(DAVA::ParticleEmitterInstance* emitter, DAVA::Entity* owner) const;

    void ValidateLandscapeTexture(DAVA::Landscape* landscape, const DAVA::FastName& texLevel);
    void ValidateCustomColorsTexture(DAVA::Entity* landscapeEntity);

    void FixIdentityTransform(DAVA::Entity* ownerNode, const DAVA::String& errorMessage);

    bool ValidateColor(DAVA::Color& color);

    DAVA::int32 EnumerateSceneNodes(DAVA::Entity* node);

    void ValidateScalesInternal(DAVA::Entity* sceneNode);

    bool ValidatePathname(const DAVA::FilePath& pathForValidation, const DAVA::String& validatedObjectName);

    bool NodeRemovingDisabled(DAVA::Entity* node);

    bool WasTextureChanged(DAVA::Texture* texture, DAVA::eGPUFamily forGPU);

    bool IsTextureDescriptorPath(const DAVA::FilePath& path);

    bool IsFBOTexture(DAVA::Texture* texture);

    DAVA::VariantType* GetCustomPropertyFromParentsTree(DAVA::Entity* ownerNode, const DAVA::String& key);

    DAVA::Set<DAVA::Entity*> emptyNodesForDeletion;

    DAVA::FilePath pathForChecking;
    DAVA::String sceneName;
};
