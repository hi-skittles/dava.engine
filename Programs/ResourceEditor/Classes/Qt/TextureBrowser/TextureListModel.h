#ifndef __TEXTURE_LIST_MODEL_H__
#define __TEXTURE_LIST_MODEL_H__

#include "DAVAEngine.h"

#include <REPlatform/DataNodes/SelectableGroup.h>
#include <REPlatform/Scene/SceneEditor2.h>
#include <REPlatform/Scene/SceneHelper.h>

#include <QAbstractListModel>
#include <QVector>

class TextureListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum TextureListSortMode
    {
        SortByName,
        SortByFileSize,
        SortByImageSize,
        SortByDataSize
    };

    TextureListModel(QObject* parent = 0);
    ~TextureListModel();

    void setScene(DAVA::SceneEditor2* scene);
    void setHighlight(const DAVA::SelectableGroup& selection);
    void setFilter(QString filter);
    void setFilterBySelectedNode(bool enabled);
    void setSortMode(TextureListModel::TextureListSortMode sortMode);

    int rowCount(const QModelIndex& parent = QModelIndex()) const;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

    void dataReady(const DAVA::TextureDescriptor* descriptor);

    QModelIndex getIndex(const DAVA::TextureDescriptor* texture);
    DAVA::Texture* getTexture(const QModelIndex& index) const;
    DAVA::Texture* getTexture(const DAVA::TextureDescriptor* descriptor) const;
    DAVA::TextureDescriptor* getDescriptor(const QModelIndex& index) const;

    bool isHighlited(const QModelIndex& index) const;

private:
    QVector<DAVA::TextureDescriptor*> textureDescriptorsAll;
    QVector<DAVA::TextureDescriptor*> textureDescriptorsFiltredSorted;
    QVector<DAVA::TextureDescriptor*> textureDescriptorsHighlight;
    QMap<const DAVA::TextureDescriptor*, DAVA::Texture*> texturesAll;

    TextureListSortMode curSortMode;
    QString curFilter;
    bool curFilterBySelectedNode;

    void clear();
    void applyFilterAndSort();

    DAVA::SceneEditor2* activeScene;
};

struct SortFnByName
{
    bool operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2);
};

struct SortFnByFileSize
{
    bool operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2);
};

struct SortFnByImageSize
{
    SortFnByImageSize(const TextureListModel* m)
        : model(m)
    {
    }
    bool operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2);

protected:
    const TextureListModel* model;
};

struct SortFnByDataSize
{
    SortFnByDataSize(const TextureListModel* m)
        : model(m)
    {
    }
    bool operator()(const DAVA::TextureDescriptor* t1, const DAVA::TextureDescriptor* t2);

protected:
    const TextureListModel* model;
};

#endif // __TEXTURE_LIST_MODEL_H__
