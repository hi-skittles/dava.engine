#pragma once

#include <REPlatform/DataNodes/Selectable.h>

#include <TArc/Qt/QtIcon.h>
#include <TArc/Qt/QtString.h>

#include <Base/Any.h>
#include <Base/BaseTypes.h>

#include <QStringList>
#include <QVariant>

namespace DAVA
{
class Scene;
class ContextAccessor;
class SceneEditor2;
class ReflectedMimeData;
} // namespace DAVA

class BaseSceneTreeTraits
{
public:
    BaseSceneTreeTraits(DAVA::ContextAccessor* accessor_);
    virtual ~BaseSceneTreeTraits() = default;

    virtual QIcon GetIcon(const DAVA::Selectable& object) const = 0;
    virtual QString GetName(const DAVA::Selectable& object) const = 0;
    virtual QString GetTooltip(const DAVA::Selectable& object) const;

    virtual DAVA::int32 GetChildrenCount(const DAVA::Selectable& object) const = 0;
    virtual void BuildUnfetchedList(const DAVA::Selectable& object, const DAVA::Function<bool(const DAVA::Selectable&)>& isFetchedFn,
                                    DAVA::Vector<DAVA::int32>& unfetchedIndexes) const = 0;
    virtual void FetchMore(const DAVA::Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                           const DAVA::Function<void(DAVA::int32, const DAVA::Selectable&)>& fetchCallback) const = 0;

    virtual Qt::ItemFlags GetItemFlags(const DAVA::Selectable& object, Qt::ItemFlags defaultFlags) const;
    virtual QVariant GetValue(const DAVA::Selectable& object, int itemRole) const;
    virtual bool SetValue(const DAVA::Selectable& object, const QVariant& value, int itemRole, DAVA::SceneEditor2* scene) const;

    virtual bool CanBeDropped(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
                              const DAVA::Selectable& parent, DAVA::int32 positionInParent) const = 0;
    virtual bool Drop(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
                      const DAVA::Selectable& parent, DAVA::int32 positionInParent, DAVA::SceneEditor2* scene) const = 0;

protected:
    DAVA::ContextAccessor* accessor = nullptr;
};

class EntityTraits final : public BaseSceneTreeTraits
{
public:
    EntityTraits(DAVA::ContextAccessor* accessor);
    QIcon GetIcon(const DAVA::Selectable& object) const override;
    QString GetName(const DAVA::Selectable& object) const override;
    DAVA::int32 GetChildrenCount(const DAVA::Selectable& object) const override;

    void BuildUnfetchedList(const DAVA::Selectable& object, const DAVA::Function<bool(const DAVA::Selectable&)>& isFetchedFn,
                            DAVA::Vector<DAVA::int32>& unfetchedIndexes) const override;
    void FetchMore(const DAVA::Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                   const DAVA::Function<void(DAVA::int32, const DAVA::Selectable&)>& fetchCallback) const override;

    Qt::ItemFlags GetItemFlags(const DAVA::Selectable& object, Qt::ItemFlags defaultFlags) const override;
    QVariant GetValue(const DAVA::Selectable& object, int itemRole) const override;

    bool CanBeDropped(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
                      const DAVA::Selectable& parent, DAVA::int32 positionInParent) const override;
    bool Drop(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
              const DAVA::Selectable& parent, DAVA::int32 positionInParent, DAVA::SceneEditor2* scene) const override;

private:
    bool CanBeEntityReparent(DAVA::Entity* entity, DAVA::Entity* parentCandidate) const;
    bool IsEditorLocalEntity(DAVA::Entity* entity) const;
    bool IsDragNDropAllow(DAVA::Entity* entity, bool isRoot = true) const;
};

class ParticleEmitterInstanceTraits final : public BaseSceneTreeTraits
{
public:
    ParticleEmitterInstanceTraits(DAVA::ContextAccessor* accessor);
    QIcon GetIcon(const DAVA::Selectable& object) const override;
    QString GetName(const DAVA::Selectable& object) const override;
    DAVA::int32 GetChildrenCount(const DAVA::Selectable& object) const override;

    void BuildUnfetchedList(const DAVA::Selectable& object, const DAVA::Function<bool(const DAVA::Selectable&)>& isFetchedFn,
                            DAVA::Vector<DAVA::int32>& unfetchedIndexes) const override;
    void FetchMore(const DAVA::Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                   const DAVA::Function<void(DAVA::int32, const DAVA::Selectable&)>& fetchCallback) const override;

    Qt::ItemFlags GetItemFlags(const DAVA::Selectable& object, Qt::ItemFlags defaultFlags) const override;
    QVariant GetValue(const DAVA::Selectable& object, int itemRole) const override;

    bool CanBeDropped(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
                      const DAVA::Selectable& parent, DAVA::int32 positionInParent) const override;
    bool Drop(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
              const DAVA::Selectable& parent, DAVA::int32 positionInParent, DAVA::SceneEditor2* scene) const override;
};

class ParticleLayerTraits final : public BaseSceneTreeTraits
{
public:
    ParticleLayerTraits(DAVA::ContextAccessor* accessor);
    QIcon GetIcon(const DAVA::Selectable& object) const override;
    QString GetName(const DAVA::Selectable& object) const override;
    DAVA::int32 GetChildrenCount(const DAVA::Selectable& object) const override;

    void BuildUnfetchedList(const DAVA::Selectable& object, const DAVA::Function<bool(const DAVA::Selectable&)>& isFetchedFn,
                            DAVA::Vector<DAVA::int32>& unfetchedIndexes) const override;
    void FetchMore(const DAVA::Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                   const DAVA::Function<void(DAVA::int32, const DAVA::Selectable&)>& fetchCallback) const override;

    Qt::ItemFlags GetItemFlags(const DAVA::Selectable& object, Qt::ItemFlags defaultFlags) const override;
    QVariant GetValue(const DAVA::Selectable& object, int itemRole) const override;
    bool SetValue(const DAVA::Selectable& object, const QVariant& value, int itemRole, DAVA::SceneEditor2* scene) const override;

    bool CanBeDropped(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
                      const DAVA::Selectable& parent, DAVA::int32 positionInParent) const override;
    bool Drop(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
              const DAVA::Selectable& parent, DAVA::int32 positionInParent, DAVA::SceneEditor2* scene) const override;
};

class ParticleSimplifiedForceTraits final : public BaseSceneTreeTraits
{
public:
    ParticleSimplifiedForceTraits(DAVA::ContextAccessor* accessor);
    QIcon GetIcon(const DAVA::Selectable& object) const override;
    QString GetName(const DAVA::Selectable& object) const override;
    DAVA::int32 GetChildrenCount(const DAVA::Selectable& object) const override;

    void BuildUnfetchedList(const DAVA::Selectable& object, const DAVA::Function<bool(const DAVA::Selectable&)>& isFetchedFn,
                            DAVA::Vector<DAVA::int32>& unfetchedIndexes) const override;
    void FetchMore(const DAVA::Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                   const DAVA::Function<void(DAVA::int32, const DAVA::Selectable&)>& fetchCallback) const override;

    Qt::ItemFlags GetItemFlags(const DAVA::Selectable& object, Qt::ItemFlags defaultFlags) const override;
    bool CanBeDropped(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
                      const DAVA::Selectable& parent, DAVA::int32 positionInParent) const override;
    bool Drop(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
              const DAVA::Selectable& parent, DAVA::int32 positionInParent, DAVA::SceneEditor2* scene) const override;
};

class ParticleForceTraits final : public BaseSceneTreeTraits
{
public:
    ParticleForceTraits(DAVA::ContextAccessor* accessor);
    QIcon GetIcon(const DAVA::Selectable& object) const override;
    QString GetName(const DAVA::Selectable& object) const override;
    DAVA::int32 GetChildrenCount(const DAVA::Selectable& object) const override;

    void BuildUnfetchedList(const DAVA::Selectable& object, const DAVA::Function<bool(const DAVA::Selectable&)>& isFetchedFn,
                            DAVA::Vector<DAVA::int32>& unfetchedIndexes) const override;
    void FetchMore(const DAVA::Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                   const DAVA::Function<void(DAVA::int32, const DAVA::Selectable&)>& fetchCallback) const override;

    Qt::ItemFlags GetItemFlags(const DAVA::Selectable& object, Qt::ItemFlags defaultFlags) const override;
    bool CanBeDropped(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
                      const DAVA::Selectable& parent, DAVA::int32 positionInParent) const override;
    bool Drop(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
              const DAVA::Selectable& parent, DAVA::int32 positionInParent, DAVA::SceneEditor2* scene) const override;
};

class SceneTreeTraitsManager final : public BaseSceneTreeTraits
{
public:
    struct FetchBlocker
    {
        FetchBlocker(SceneTreeTraitsManager* mng)
            : manager(mng)
        {
            DVASSERT(manager->fetchBlocked == false);
            manager->fetchBlocked = true;
        }

        ~FetchBlocker()
        {
            manager->fetchBlocked = false;
        }

        SceneTreeTraitsManager* manager;
    };

    SceneTreeTraitsManager(DAVA::ContextAccessor* accessor);

    QIcon GetIcon(const DAVA::Selectable& object) const override;
    QString GetName(const DAVA::Selectable& object) const override;
    QString GetTooltip(const DAVA::Selectable& object) const override;
    DAVA::int32 GetChildrenCount(const DAVA::Selectable& object) const override;

    void BuildUnfetchedList(const DAVA::Selectable& object, const DAVA::Function<bool(const DAVA::Selectable&)>& isFetchedFn,
                            DAVA::Vector<DAVA::int32>& unfetchedIndexes) const override;
    void FetchMore(const DAVA::Selectable& object, const DAVA::Vector<DAVA::int32>& unfetchedIndexes,
                   const DAVA::Function<void(DAVA::int32, const DAVA::Selectable&)>& fetchCallback) const override;

    Qt::ItemFlags GetItemFlags(const DAVA::Selectable& object, Qt::ItemFlags defaultFlags) const override;
    QVariant GetValue(const DAVA::Selectable& object, int itemRole) const override;
    bool SetValue(const DAVA::Selectable& object, const QVariant& value, int itemRole, DAVA::SceneEditor2* scene) const override;

    bool CanBeDropped(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
                      const DAVA::Selectable& parent, DAVA::int32 positionInParent) const override;
    bool Drop(const DAVA::ReflectedMimeData* mimeData, Qt::DropAction action,
              const DAVA::Selectable& parent, DAVA::int32 positionInParent, DAVA::SceneEditor2* scene) const override;

private:
    template <typename TObject, typename TTraits>
    void AddTraitsNode(DAVA::ContextAccessor* accessor);
    const BaseSceneTreeTraits* GetTraits(const DAVA::Selectable& object) const;

    struct TraitsNode
    {
        const DAVA::ReflectedType* type = nullptr;
        DAVA::Any traits;
    };
    DAVA::Vector<TraitsNode> traits;
    bool fetchBlocked = false;
};
