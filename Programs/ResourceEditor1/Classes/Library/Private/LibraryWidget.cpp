#include "Classes/Library/Private/LibraryWidget.h"
#include "Classes/Library/Private/LibraryFileSystemModel.h"
#include "Classes/Library/Private/LibraryData.h"

#include "Classes/Project/ProjectManagerData.h"
#include "Classes/SceneManager/SceneData.h"
#include "Classes/Application/REGlobal.h"

#include "Classes/Qt/Main/QtUtils.h"

#include <TArc/Core/FieldBinder.h>
#include <TArc/Core/ContextAccessor.h>
#include <TArc/Utils/QtConnections.h>
#include <TArc/WindowSubSystem/QtAction.h>
#include <TArc/DataProcessing/Common.h>

#include <TArc/Utils/Utils.h>
#include <QtHelpers/HelperFunctions.h>

#include <Reflection/ReflectedType.h>
#include <Render/Image/ImageFormatInterface.h>
#include <Render/RenderBase.h>

#include <QToolBar>
#include <QLineEdit>
#include <QComboBox>
#include <QHeaderView>
#include <QProgressBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QProcess>
#include <QFileSystemModel>
#include <QMenu>
#include <QAction>
#include <QFileInfo>

namespace LibraryWidgetDetail
{
QStringList GetExtensions(DAVA::ImageFormat imageFormat)
{
    QStringList extList;
    auto extensions = DAVA::ImageSystem::GetExtensionsFor(imageFormat);
    for (auto& ex : extensions)
    {
        extList << QString("*") + ex.c_str();
    }

    return extList;
}

struct FileType
{
    FileType()
    {
    }

    FileType(const QString& n)
    {
        name = n;
    }

    FileType(const QString& n, const QString& f)
    {
        name = n;
        filter << f;
    }

    FileType(const QString& n, const QStringList& _filter)
    {
        name = n;
        filter = _filter;
    }

    QString name;
    QStringList filter;
};

QVector<FileType> fileTypeValues;
}

LibraryWidget::LibraryWidget(DAVA::TArc::ContextAccessor* contextAccessor_, QWidget* parent)
    : QWidget(parent)
    , contextAccessor(contextAccessor_)
{
    DVASSERT(contextAccessor != nullptr);

    SetupFileTypes();
    SetupToolbar();
    SetupView();
    SetupLayout();

    ViewAsList();
    OnFilesTypeChanged(0);

    QObject::connect(filesView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &LibraryWidget::SelectionChanged);
    QObject::connect(filesView, &QTreeView::customContextMenuRequested, this, &LibraryWidget::ShowContextMenu);
    QObject::connect(filesView, &QTreeView::doubleClicked, this, &LibraryWidget::fileDoubleClicked);

    fieldBinder.reset(new DAVA::TArc::FieldBinder(contextAccessor));
    DAVA::TArc::FieldDescriptor projectFieldDescriptor(DAVA::ReflectedTypeDB::Get<ProjectManagerData>(), DAVA::FastName(ProjectManagerData::ProjectPathProperty));
    fieldBinder->BindField(projectFieldDescriptor, DAVA::MakeFunction(this, &LibraryWidget::OnProjectChanged));
}

void LibraryWidget::SetupFileTypes()
{
    DAVA::UnorderedSet<DAVA::String> sourceFiles;
    sourceFiles.insert(DAVA::TextureDescriptor::GetDescriptorExtension());
    for (auto formatType : DAVA::TextureDescriptor::sourceTextureTypes)
    {
        auto extensions = DAVA::ImageSystem::GetExtensionsFor(formatType);
        for (auto& ex : extensions)
        {
            sourceFiles.insert(ex.c_str());
        }
    }

    QStringList sourceImagesList;
    for (auto& sf : sourceFiles)
    {
        sourceImagesList << (QString("*") + sf.c_str());
    }

    DAVA::UnorderedSet<DAVA::String> compressedFiles;
    for (auto formatType : DAVA::TextureDescriptor::compressedTextureTypes)
    {
        auto extensions = DAVA::ImageSystem::GetExtensionsFor(formatType);
        for (auto& ex : extensions)
        {
            compressedFiles.insert(ex.c_str());
        }
    }

    QStringList compressedImagesList;
    for (auto& cf : compressedFiles)
    {
        compressedImagesList << (QString("*") + cf.c_str());
    }

    LibraryWidgetDetail::FileType allFiles("All files");
    allFiles.filter << "*.dae";
    allFiles.filter << "*.sc2";
    allFiles.filter << sourceImagesList;

    LibraryWidgetDetail::fileTypeValues.reserve(10);

    LibraryWidgetDetail::fileTypeValues.push_back(allFiles);
    QStringList models;
    models << "*.dae"
           << "*.sc2";
    LibraryWidgetDetail::fileTypeValues.push_back(LibraryWidgetDetail::FileType("Models", models));

    LibraryWidgetDetail::fileTypeValues.push_back(LibraryWidgetDetail::FileType("Source Textures", sourceImagesList));
    LibraryWidgetDetail::fileTypeValues.push_back(LibraryWidgetDetail::FileType("Compressed Textures", compressedImagesList));

    LibraryWidgetDetail::fileTypeValues.push_back(LibraryWidgetDetail::FileType("DAE", "*.dae"));
    LibraryWidgetDetail::fileTypeValues.push_back(LibraryWidgetDetail::FileType("SC2", "*.sc2"));
    LibraryWidgetDetail::fileTypeValues.push_back(LibraryWidgetDetail::FileType("TEX", QString("*") + DAVA::TextureDescriptor::GetDescriptorExtension().c_str()));

    for (auto formatType : DAVA::TextureDescriptor::sourceTextureTypes)
    {
        DAVA::ImageFormatInterface* formatHelper = DAVA::ImageSystem::GetImageFormatInterface(formatType);
        LibraryWidgetDetail::fileTypeValues.push_back(LibraryWidgetDetail::FileType(QString::fromStdString(formatHelper->GetName()), LibraryWidgetDetail::GetExtensions(formatType)));
    }
}

void LibraryWidget::SetupToolbar()
{
    toolbar = new QToolBar(this);
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar->setMovable(false);

    filesTypeFilter = new QComboBox(toolbar);
    filesTypeFilter->setEditable(false);
    filesTypeFilter->setMinimumWidth(100);
    filesTypeFilter->setMaximumWidth(100);
    for (int i = 0; i < LibraryWidgetDetail::fileTypeValues.size(); ++i)
    {
        filesTypeFilter->addItem(LibraryWidgetDetail::fileTypeValues[i].name);
    }
    filesTypeFilter->setCurrentIndex(0);

    QIcon asListIcon(QString::fromUtf8(":/QtIconsTextureDialog/view_list.png"));
    actionViewAsList = new QAction(asListIcon, "View as list", toolbar);
    actionViewAsList->setCheckable(true);
    actionViewAsList->setChecked(true);

    QIcon asDetailedIcon(QString::fromUtf8(":/QtIcons/all.png"));
    actionViewDetailed = new QAction(asDetailedIcon, "View detailed", toolbar);
    actionViewDetailed->setCheckable(true);
    actionViewDetailed->setChecked(false);

    QObject::connect(filesTypeFilter, SIGNAL(currentIndexChanged(int)), this, SLOT(OnFilesTypeChanged(int)));
    QObject::connect(actionViewAsList, SIGNAL(triggered()), this, SLOT(ViewAsList()));
    QObject::connect(actionViewDetailed, SIGNAL(triggered()), this, SLOT(ViewDetailed()));

    toolbar->addWidget(filesTypeFilter);

    toolbar->addAction(actionViewAsList);
    toolbar->addAction(actionViewDetailed);
}

void LibraryWidget::SetupView()
{
    filesModel = new LibraryFileSystemModel(this);

    filesView = new LibraryTreeView(this);
    filesView->setContextMenuPolicy(Qt::CustomContextMenu);
    filesView->setDragDropMode(QAbstractItemView::DragOnly);
    filesView->setDragEnabled(true);
    filesView->setUniformRowHeights(true);

    filesView->setModel(filesModel);

    QObject::connect(static_cast<LibraryTreeView*>(filesView), &LibraryTreeView::DragStarted, this, &LibraryWidget::DragStarted);
}

void LibraryWidget::SetupLayout()
{
    // put tab bar into vertical layout
    layout = new QVBoxLayout();
    layout->addWidget(toolbar);
    layout->addWidget(filesView);
    layout->setMargin(0);
    layout->setSpacing(1);
    setLayout(layout);
}

void LibraryWidget::ViewAsList()
{
    viewMode = VIEW_AS_LIST;
    filesView->header()->setVisible(false);

    HideDetailedColumnsAtFilesView(true);

    actionViewAsList->setChecked(true);
    actionViewDetailed->setChecked(false);
}

void LibraryWidget::ViewDetailed()
{
    viewMode = VIEW_DETAILED;
    filesView->header()->setVisible(true);

    // Magic trick for MacOS: call function twice
    HideDetailedColumnsAtFilesView(false);
    HideDetailedColumnsAtFilesView(false);
    //EndOftrick

    actionViewAsList->setChecked(false);
    actionViewDetailed->setChecked(true);
}

void LibraryWidget::HideDetailedColumnsAtFilesView(bool hide)
{
    int columns = (hide) ? 1 : filesModel->columnCount();
    int width = filesView->geometry().width() / columns;

    if (!hide)
    {
        filesView->setColumnWidth(0, width);
    }

    for (int i = 1; i < filesModel->columnCount(); ++i)
    {
        filesView->setColumnHidden(i, hide);
        filesView->setColumnWidth(i, width);
    }
}

void LibraryWidget::SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    LibraryData* libraryData = contextAccessor->GetGlobalContext()->GetData<LibraryData>();
    DVASSERT(libraryData != nullptr);

    if (0 == selected.count())
    {
        libraryData->selectedPath = DAVA::FilePath();
        return;
    }

    const QModelIndex index = selected.indexes().first();
    QFileInfo fileInfo = filesModel->fileInfo(index);
    libraryData->selectedPath = fileInfo.filePath().toStdString();
}

void LibraryWidget::fileDoubleClicked(const QModelIndex& index)
{
    QFileInfo fileInfo = filesModel->fileInfo(index);
    DAVA::FilePath path = fileInfo.absoluteFilePath().toStdString();
    emit DoubleClicked(path);
}

void LibraryWidget::ShowContextMenu(const QPoint& point)
{
    const QModelIndex index = filesView->indexAt(point);

    if (!index.isValid())
        return;

    QFileInfo fileInfo = filesModel->fileInfo(index);
    if (!fileInfo.isFile())
        return;

    QMenu contextMenu;
    QVariant fileInfoAsVariant = QVariant::fromValue<QFileInfo>(fileInfo);

    DAVA::FilePath pathname = fileInfo.absoluteFilePath().toStdString();
    if (pathname.IsEqualToExtension(".sc2"))
    {
        DAVA::TArc::QtAction* actionAdd = new DAVA::TArc::QtAction(contextAccessor, "Add Model", &contextMenu);
        DAVA::TArc::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<SceneData>();
        fieldDescr.fieldName = DAVA::FastName(SceneData::scenePropertyName);
        actionAdd->SetStateUpdationFunction(DAVA::TArc::QtAction::Enabled, fieldDescr, [](const DAVA::Any& v) {
            return v.IsEmpty() == false;
        });
        QObject::connect(actionAdd, &QAction::triggered, this, &LibraryWidget::OnAddModel);
        contextMenu.addAction(actionAdd);

        actionAdd->setEnabled(contextAccessor->GetActiveContext() != nullptr);
        QAction* actionEdit = contextMenu.addAction("Edit Model", this, SLOT(OnEditModel()));

        actionAdd->setData(fileInfoAsVariant);
        actionEdit->setData(fileInfoAsVariant);
    }
    else if (pathname.IsEqualToExtension(".dae"))
    {
        QAction* actionConvert = contextMenu.addAction("Convert", this, SLOT(OnConvertDae()));
        actionConvert->setData(fileInfoAsVariant);

        QAction* actionConvertAnimations = contextMenu.addAction("Convert Animations", this, SLOT(OnConvertAnimationsDae()));
        actionConvertAnimations->setData(fileInfoAsVariant);
    }

    contextMenu.addSeparator();
    QAction* actionRevealAt = contextMenu.addAction("Reveal at folder", this, SLOT(OnRevealAtFolder()));
    actionRevealAt->setData(fileInfoAsVariant);

    contextMenu.exec(filesView->mapToGlobal(point));
}

void LibraryWidget::OnFilesTypeChanged(int typeIndex)
{
    if (curTypeIndex == typeIndex)
        return;

    curTypeIndex = typeIndex;

    filesModel->SetExtensionFilter(LibraryWidgetDetail::fileTypeValues[curTypeIndex].filter);
    filesView->setRootIndex(filesModel->index(rootPathname));
}

void LibraryWidget::OnAddModel()
{
    if (contextAccessor->GetActiveContext() == nullptr)
    {
        return;
    }

    QVariant indexAsVariant = qobject_cast<QAction*>(sender())->data();
    const QFileInfo fileInfo = indexAsVariant.value<QFileInfo>();

    emit AddSceneRequested(fileInfo.absoluteFilePath().toStdString());
}

void LibraryWidget::OnEditModel()
{
    QVariant indexAsVariant = qobject_cast<QAction*>(sender())->data();
    const QFileInfo fileInfo = indexAsVariant.value<QFileInfo>();

    emit EditSceneRequested(fileInfo.absoluteFilePath().toStdString());
}

void LibraryWidget::OnConvertDae()
{
    QVariant indexAsVariant = qobject_cast<QAction*>(sender())->data();
    const QFileInfo fileInfo = indexAsVariant.value<QFileInfo>();

    emit DAEConvertionRequested(fileInfo.absoluteFilePath().toStdString());
}

void LibraryWidget::OnConvertAnimationsDae()
{
    QVariant indexAsVariant = qobject_cast<QAction*>(sender())->data();
    const QFileInfo fileInfo = indexAsVariant.value<QFileInfo>();

    emit DAEAnimationConvertionRequested(fileInfo.absoluteFilePath().toStdString());
}

void LibraryWidget::OnRevealAtFolder()
{
    QVariant indexAsVariant = qobject_cast<QAction*>(sender())->data();
    const QFileInfo fileInfo = indexAsVariant.value<QFileInfo>();

    QtHelpers::ShowInOSFileManager(fileInfo.absoluteFilePath());
}

void LibraryWidget::OnProjectChanged(const DAVA::Any& projectFieldValue)
{
    ProjectManagerData* projectData = contextAccessor->GetGlobalContext()->GetData<ProjectManagerData>();
    if (projectData->GetProjectPath().IsEmpty())
    {
        setEnabled(false);

        rootPathname = QDir::rootPath();
        filesView->setRootIndex(filesModel->index(rootPathname));
        filesView->collapseAll();
    }
    else
    {
        setEnabled(true);

        rootPathname = QString::fromStdString(projectData->GetDataSource3DPath().GetAbsolutePathname());

        filesModel->Load(rootPathname);
        filesView->setRootIndex(filesModel->index(rootPathname));
    }
}
