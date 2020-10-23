#include "Classes/Qt/MaterialEditor/MaterialEditor.h"
#include "Classes/Qt/MaterialEditor/MaterialModel.h"
#include "Classes/Qt/MaterialEditor/MaterialFilterModel.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataIntrospection.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataInspMember.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyData/QtPropertyDataInspDynamic.h"
#include "Classes/Qt/Tools/QtPropertyEditor/QtPropertyDataValidator/TexturePathValidator.h"
#include "Classes/Qt/Tools/PathDescriptor/PathDescriptor.h"

#include "ui_materialeditor.h"

#include <REPlatform/Commands/ApplyMaterialPresetCommand.h>
#include <REPlatform/Commands/MaterialConfigCommands.h>
#include <REPlatform/Commands/MaterialGlobalCommand.h>
#include <REPlatform/Commands/MaterialRemoveTexture.h>
#include <REPlatform/Commands/RECommandNotificationObject.h>
#include <REPlatform/DataNodes/ProjectManagerData.h>
#include <REPlatform/DataNodes/SelectionData.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Global/GlobalOperations.h>
#include <REPlatform/Scene/SceneHelper.h>
#include <REPlatform/Scene/Utils/RETextureDescriptorUtils.h>
#include <REPlatform/Scene/Utils/Utils.h>

#include <QtTools/FileDialogs/FileDialog.h>
#include <QtTools/Updaters/LazyUpdater.h>

#include <TArc/Core/Deprecated.h>
#include <TArc/Core/FieldBinder.h>
#include <TArc/Utils/Utils.h>
#include <TArc/Controls/CheckBox.h>

#include <Base/Introspection.h>
#include <Functional/Function.h>
#include <Render/Texture.h>
#include <Scene3D/Systems/QualitySettingsSystem.h>

#include <QAction>
#include <QBoxLayout>
#include <QCheckBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileInfo>
#include <QGroupBox>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextStream>
#include <QVariant>
#include "Utils/StringFormat.h"

namespace UIName
{
const DAVA::FastName Template("Template");

const DAVA::FastName Name("Name");
const DAVA::FastName Group("Group");

const DAVA::FastName Base("Base");
const DAVA::FastName Flags("Flags");
const DAVA::FastName Illumination("Illumination");
const DAVA::FastName Properties("Properties");
const DAVA::FastName Textures("Textures");
}

namespace NMaterialSectionName
{
const DAVA::FastName LocalFlags("localFlags");
const DAVA::FastName LocalProperties("localProperties");
const DAVA::FastName LocalTextures("localTextures");
}

namespace MaterialEditorDetail
{
const char* CURRENT_TAB_CHANGED_UPDATE = "CurrentTabChangedUpdate";
class PropertyLock
{
public:
    PropertyLock(QObject* propertyHolder_, const char* propertyName_)
        : propertyHolder(propertyHolder_)
        , propertyName(propertyName_)
    {
        if (!propertyHolder->property(propertyName).toBool())
        {
            locked = true;
            propertyHolder->setProperty(propertyName, true);
        }
    }

    ~PropertyLock()
    {
        if (locked)
        {
            propertyHolder->setProperty(propertyName, false);
        }
    }

    bool IsLocked() const
    {
        return locked;
    }

private:
    QObject* propertyHolder = nullptr;
    const char* propertyName = nullptr;
    bool locked = false;
};

struct InvalidTexturesCollector
{
    DAVA::Set<DAVA::FastName> validTextures;
    DAVA::Map<DAVA::FastName, DAVA::Vector<DAVA::FilePath>> invalidTextures;

    void CollectValidTextures(QtPropertyData* data)
    {
        DVASSERT(data != nullptr);
        validTextures.insert(data->GetName());
        for (int i = 0; i < data->ChildCount(); ++i)
        {
            CollectValidTextures(data->ChildGet(i));
        }
    }

    void CollectInvalidTextures(DAVA::NMaterial* material)
    {
        while (material != nullptr)
        {
            using TTexturesMap = DAVA::UnorderedMap<DAVA::FastName, DAVA::MaterialTextureInfo*>;

            const TTexturesMap& localTextures = material->GetLocalTextures();
            for (const auto& lc : localTextures)
            {
                // DF-10204, we don't allow change heightmap in material for new Landscape.
                if (validTextures.count(lc.first) == 0 && lc.first != DAVA::NMaterialTextureName::TEXTURE_HEIGHTMAP)
                {
                    invalidTextures[lc.first].push_back(lc.second->path);
                }
            }

            material = material->GetParent();
        }
    }
};

bool LoadHiddenFieldsConfig(DAVA::RefPtr<DAVA::KeyedArchive> rootArchive, const DAVA::FilePath& path)
{
    DAVA::RefPtr<DAVA::YamlParser> parser(DAVA::YamlParser::Create(path));
    if (parser.Get() == nullptr)
    {
        return false;
    }

    DAVA::Vector<std::pair<DAVA::String, DAVA::YamlNode::eType>> fields = {
        std::make_pair(DAVA::String("TemplateName"), DAVA::YamlNode::TYPE_ARRAY),
        std::make_pair(DAVA::String("Flags"), DAVA::YamlNode::TYPE_ARRAY),
        std::make_pair(DAVA::String("Illumination"), DAVA::YamlNode::TYPE_ARRAY),
        std::make_pair(DAVA::String("Properties"), DAVA::YamlNode::TYPE_ARRAY),
        std::make_pair(DAVA::String("Textures"), DAVA::YamlNode::TYPE_ARRAY),
    };

    DAVA::Function<void(const DAVA::YamlNode*)> parseFn = [&](const DAVA::YamlNode* node) {
        if (node->GetType() != DAVA::YamlNode::TYPE_MAP)
        {
            DAVA::Logger::Warning("Material editor config loading failed.");
            return;
        }

        DAVA::Vector<const DAVA::YamlNode*> groupNodes;
        groupNodes.reserve(fields.size());
        for (size_t i = 0; i < fields.size(); ++i)
        {
            const DAVA::String& fieldName = fields[i].first;
            const DAVA::YamlNode* fieldNode = node->Get(fieldName);
            if (fieldNode == nullptr && i == 0)
            {
                DAVA::Logger::Warning("Material editor config loading failed. Field %s wasn't found in config.", fieldName.c_str());
                return;
            }

            if (fieldNode->GetType() != fields[i].second)
            {
                DAVA::Logger::Warning("Material editor config loading failed. Field %s has incorrect format", fieldName.c_str());
                return;
            }

            groupNodes.push_back(fieldNode);
        }

        DAVA::RefPtr<DAVA::KeyedArchive> templateNameArchive(new DAVA::KeyedArchive());

        for (size_t i = 1; i < groupNodes.size(); ++i)
        {
            DAVA::RefPtr<DAVA::KeyedArchive> groupArchive(new DAVA::KeyedArchive());

            const DAVA::YamlNode* groupNode = groupNodes[i];
            const auto& hiddenProps = groupNode->AsVector();
            for (const auto& prop : hiddenProps)
            {
                if (prop->GetType() != DAVA::YamlNode::TYPE_STRING)
                {
                    DAVA::Logger::Warning("Material Editor config parse. Group %s contain node with incorrect format", fields[i].first.c_str());
                }
                else
                {
                    groupArchive->SetBool(prop->AsString(), true);
                }
            }

            templateNameArchive->SetArchive(fields[i].first, groupArchive.Get());
        }

        const auto& templates = groupNodes[0]->AsVector();
        for (const auto& t : templates)
        {
            if (t->GetType() != DAVA::YamlNode::TYPE_STRING)
            {
                DAVA::Logger::Warning("Material Editor config parse. Element of Template name should be a string");
            }
            else
            {
                rootArchive->SetArchive(t->AsString(), templateNameArchive.Get());
            }
        }
    };

    DAVA::YamlNode* rootNode = parser->GetRootNode();
    if (rootNode->GetType() != DAVA::YamlNode::TYPE_ARRAY)
    {
        DAVA::Logger::Warning("Material editor config loading failed. Root item is not ARRAY");
        return false;
    }

    const auto& nodes = rootNode->AsVector();
    for (const auto& node : nodes)
    {
        parseFn(node.Get());
    }

    return true;
}
}

DAVA_VIRTUAL_REFLECTION_IMPL(MaterialEditorSettings)
{
    DAVA::ReflectionRegistrator<MaterialEditorSettings>::Begin()[DAVA::M::DisplayName("Material Editor")]
    .ConstructorByPointer()
    .Field("hiddenFieldsConfig", &MaterialEditorSettings::hiddenFieldsConfig)[DAVA::M::File("*.yaml"), DAVA::M::DisplayName("Hidden fields config")]
    .End();
}

class MaterialEditor::PropertiesBuilder
{
public:
    PropertiesBuilder(MaterialEditor* editor_)
        : editor(editor_)
    {
        DAVA::SceneEditor2* scene = editor->activeScene;
        if (scene != nullptr)
        {
            globalMaterial = scene->GetGlobalMaterial();
        }
    }

    void Build(const QList<DAVA::NMaterial*>& materials, DAVA::Vector<std::unique_ptr<QtPropertyData>>& properties)
    {
        DAVA::ProjectManagerData* projectManagerData = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
        DVASSERT(projectManagerData != nullptr);
        const DAVA::Vector<DAVA::MaterialTemplateInfo>* templates = projectManagerData->GetMaterialTemplatesInfo();
        DAVA::UnorderedMap<DAVA::String, DAVA::String> templMap;

        for (const DAVA::MaterialTemplateInfo& templ : *templates)
        {
            templMap[templ.path] = templ.name;
        }

        std::unique_ptr<QtPropertyData> baseRoot = CreateHeader(UIName::Base);
        std::unique_ptr<QtPropertyData> flagsRoot = CreateHeader(UIName::Flags);
        std::unique_ptr<QtPropertyData> illuminationRoot = CreateHeader(UIName::Illumination);
        std::unique_ptr<QtPropertyData> propertiesRoot = CreateHeader(UIName::Properties);
        std::unique_ptr<QtPropertyData> texturesRoot = CreateHeader(UIName::Textures);

        foreach (DAVA::NMaterial* material, materials)
        {
            DAVA::KeyedArchive* hiddenFlags = nullptr;
            DAVA::KeyedArchive* hiddenIllums = nullptr;
            DAVA::KeyedArchive* hiddenProps = nullptr;
            DAVA::KeyedArchive* hiddenTextures = nullptr;

            DAVA::FastName fxName = material->GetEffectiveFXName();
            auto iter = templMap.end();
            if (fxName.IsValid() == true && editor->applyHiddenPropsFilter)
            {
                iter = templMap.find(fxName.c_str());
            }

            if (editor->hiddenFieldsConfig.Get() != nullptr && iter != templMap.end())
            {
                DAVA::KeyedArchive* templateArchive = editor->hiddenFieldsConfig->GetArchive(iter->second);
                if (templateArchive != nullptr)
                {
                    hiddenFlags = templateArchive->GetArchive(UIName::Flags.c_str());
                    hiddenIllums = templateArchive->GetArchive(UIName::Illumination.c_str());
                    hiddenProps = templateArchive->GetArchive(UIName::Properties.c_str());
                    hiddenTextures = templateArchive->GetArchive(UIName::Textures.c_str());
                }
            }

            FillBase(baseRoot.get(), material);
            FillDynamic(flagsRoot.get(), material, NMaterialSectionName::LocalFlags, hiddenFlags);
            FillIllumination(illuminationRoot.get(), material, hiddenIllums);
            FillDynamic(propertiesRoot.get(), material, NMaterialSectionName::LocalProperties, hiddenProps);
            FillDynamic(texturesRoot.get(), material, NMaterialSectionName::LocalTextures, hiddenTextures);
        }

        ApplyTextureValidator(texturesRoot.get());
        UpdateAllAddRemoveButtons(flagsRoot.get());
        UpdateAllAddRemoveButtons(propertiesRoot.get());
        UpdateAllAddRemoveButtons(illuminationRoot.get());
        UpdateAllAddRemoveButtons(texturesRoot.get());
        CreateReloadTextureButton(texturesRoot.get());

        FillInvalidTextures(texturesRoot.get(), materials);

        properties.push_back(std::move(baseRoot));
        properties.push_back(std::move(flagsRoot));
        properties.push_back(std::move(illuminationRoot));
        properties.push_back(std::move(propertiesRoot));
        properties.push_back(std::move(texturesRoot));
    }

    void UpdateAllAddRemoveButtons(QtPropertyData* root)
    {
        QtPropertyDataInspDynamic* dynamicData = dynamic_cast<QtPropertyDataInspDynamic*>(root);
        if (nullptr != dynamicData)
        {
            UpdateAddRemoveButtonState(dynamicData);
        }

        for (int i = 0; i < root->ChildCount(); ++i)
        {
            UpdateAllAddRemoveButtons(root->ChildGet(i));
        }
    }

    void UpdateAddRemoveButtonState(QtPropertyDataInspDynamic* data)
    {
        // don't create/update buttons for global material
        DAVA::SceneEditor2* curScene = editor->activeScene;
        if (curScene == nullptr)
            return;

        // extract member flags from dynamic info
        int memberFlags = data->dynamicInfo->MemberFlags(data->ddata, data->name);

        QtPropertyToolButton* addRemoveButton = FindOrCreateButton(data, QStringLiteral("dynamicAddRemoveButton"));

        QBrush bgColor;
        bool editEnabled = false;

        // self property - should be remove button
        if (memberFlags & DAVA::I_EDIT)
        {
            editEnabled = true;
            addRemoveButton->setIcon(DAVA::SharedIcon(":/QtIcons/cminus.png"));
            addRemoveButton->setToolTip(QStringLiteral("Remove property"));

            // isn't set in parent or shader
            if (!(memberFlags & DAVA::I_VIEW))
            {
                bgColor = QBrush(QColor(255, 0, 0, 25));
            }
        }
        // inherited from parent property - should be add button
        else
        {
            editEnabled = false;
            bgColor = QBrush(QColor(0, 0, 0, 25));
            addRemoveButton->setIcon(DAVA::SharedIcon(":/QtIcons/cplus.png"));
            addRemoveButton->setToolTip(QStringLiteral("Add property"));
        }

        // don't allow editing for members that are inherited
        data->SetEnabled(editEnabled);
        data->SetBackground(bgColor);
        for (int m = 0; m < data->ChildCount(); ++m)
        {
            data->ChildGet(m)->SetEnabled(editEnabled);
            data->ChildGet(m)->SetBackground(bgColor);
        }
    }

private:
    QtPropertyToolButton* FindOrCreateButton(QtPropertyData* data, const QString& buttonName)
    {
        for (int i = 0; i < data->GetButtonsCount(); ++i)
        {
            QtPropertyToolButton* btn = data->GetButton(i);
            if (btn->objectName() == buttonName)
            {
                return btn;
            }
        }

        QtPropertyToolButton* button = data->AddButton();
        button->setObjectName(buttonName);
        button->setIconSize(QSize(14, 14));
        QObject::connect(button, &QAbstractButton::clicked, editor, &MaterialEditor::OnAddRemoveButton);

        return button;
    }

    void CreateReloadTextureButton(QtPropertyData* data)
    {
        QtPropertyDataInspDynamic* dynamicData = dynamic_cast<QtPropertyDataInspDynamic*>(data);
        if (nullptr != dynamicData)
        {
            bool buttonAlreadyExists = false;
            QString reloadButtonName = QStringLiteral("reloadTexture");
            for (int i = 0; i < data->GetButtonsCount(); ++i)
            {
                QtPropertyToolButton* btn = data->GetButton(i);
                if (btn->objectName() == reloadButtonName)
                {
                    buttonAlreadyExists = true;
                    break;
                }
            }

            if (buttonAlreadyExists == false)
            {
                QtPropertyToolButton* button = data->AddButton();
                button->setObjectName(QStringLiteral("reloadTexture"));
                button->setIcon(DAVA::SharedIcon(":/QtIcons/reloadtextures.png"));
                button->setIconSize(QSize(14, 14));
                QObject::connect(button, &QAbstractButton::clicked, editor, &MaterialEditor::OnReloadTexture);
            }
        }

        for (int i = 0; i < data->ChildCount(); ++i)
        {
            CreateReloadTextureButton(data->ChildGet(i));
        }
    }

    std::unique_ptr<QtPropertyData> CreateHeader(const DAVA::FastName& sectionName)
    {
        std::unique_ptr<QtPropertyData> section(new QtPropertyData(sectionName));
        editor->ui->materialProperty->ApplyStyle(section.get(), QtPropertyEditor::HEADER_STYLE);
        return section;
    }

    void FillBase(QtPropertyData* baseRoot, DAVA::NMaterial* material)
    {
        const DAVA::InspInfo* info = material->GetTypeInfo();

        // fill material name
        const DAVA::InspMember* nameMember = info->Member(DAVA::FastName("materialName"));
        if (nullptr != nameMember)
        {
            baseRoot->MergeChild(std::unique_ptr<QtPropertyData>(new QtPropertyDataInspMember(UIName::Name, material, nameMember)));
        }

        // fill material group, only for material type
        const DAVA::InspMember* groupMember = info->Member(DAVA::FastName("qualityGroup"));
        if ((nullptr != groupMember) && (globalMaterial != material))
        {
            QtPropertyDataInspMember* group = new QtPropertyDataInspMember(UIName::Group, material, groupMember);
            baseRoot->MergeChild(std::unique_ptr<QtPropertyData>(group));

            // Add unknown value:
            group->AddAllowedValue(DAVA::VariantType(DAVA::FastName()), "Unknown");

            // fill allowed values for material group
            for (size_t i = 0; i < DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityGroupCount(); ++i)
            {
                DAVA::FastName groupName = DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityGroupName(i);
                group->AddAllowedValue(DAVA::VariantType(groupName), groupName.c_str());
            }
        }
    }

    void FillDynamic(QtPropertyData* root, DAVA::NMaterial* material, const DAVA::FastName& dynamicName, DAVA::KeyedArchive* hiddenProps)
    {
        if (material == globalMaterial && dynamicName == NMaterialSectionName::LocalTextures)
        {
            return;
        }

        const DAVA::InspInfo* info = material->GetTypeInfo();
        const DAVA::InspMember* materialMember = info->Member(dynamicName);

        // fill material flags
        if ((nullptr != materialMember) && (nullptr != materialMember->Dynamic()))
        {
            DAVA::InspInfoDynamic* dynamicInfo = materialMember->Dynamic()->GetDynamicInfo();
            FillDynamicMembers(root, dynamicInfo, material, material == globalMaterial, hiddenProps);
        }
    }

    void FillInvalidTextures(QtPropertyData* root, const QList<DAVA::NMaterial*>& materials)
    {
        MaterialEditorDetail::InvalidTexturesCollector collector;
        collector.CollectValidTextures(root);
        foreach (DAVA::NMaterial* material, materials)
        {
            collector.CollectInvalidTextures(material);
        }

        for (const auto& t : collector.invalidTextures)
        {
            DVASSERT(!t.second.empty());
            for (size_t i = 0; i < t.second.size(); ++i)
            {
                QVariant qValue(QString::fromStdString(t.second[i].GetAbsolutePathname()));
                std::unique_ptr<QtPropertyData> textureSlot(new QtPropertyData(t.first, qValue));

                QtPropertyToolButton* addRemoveButton = textureSlot->AddButton();
                addRemoveButton->setObjectName("dynamicAddRemoveButton");
                addRemoveButton->setIconSize(QSize(14, 14));
                addRemoveButton->setIcon(DAVA::SharedIcon(":/QtIcons/cminus.png"));
                addRemoveButton->setToolTip(QStringLiteral("Remove property"));
                QObject::connect(addRemoveButton, &QAbstractButton::clicked, editor, &MaterialEditor::removeInvalidTexture);

                textureSlot->SetEnabled(false);
                textureSlot->SetBackground(QBrush(QColor(255, 0, 0, 25)));
                root->MergeChild(std::move(textureSlot));
            }
        }
    }

    void FillIllumination(QtPropertyData* root, DAVA::NMaterial* material, DAVA::KeyedArchive* hiddenProps)
    {
        if (material->GetEffectiveFlagValue(DAVA::NMaterialFlagName::FLAG_ILLUMINATION_USED) != 1)
        {
            return;
        }

        const DAVA::InspInfo* info = material->GetTypeInfo();
        const DAVA::InspMember* flagsMember = info->Member(NMaterialSectionName::LocalFlags);
        if (flagsMember != nullptr && (nullptr != flagsMember->Dynamic()))
        {
            DAVA::InspInfoDynamic* dynamicInfo = flagsMember->Dynamic()->GetDynamicInfo();
            FillDynamicMember(root, dynamicInfo, material, DAVA::NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_CASTER, hiddenProps);
            FillDynamicMember(root, dynamicInfo, material, DAVA::NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_RECEIVER, hiddenProps);
        }

        const DAVA::InspMember* propertiesMember = info->Member(NMaterialSectionName::LocalProperties);
        if (propertiesMember != nullptr && (nullptr != propertiesMember->Dynamic()))
        {
            DAVA::InspInfoDynamic* dynamicInfo = propertiesMember->Dynamic()->GetDynamicInfo();
            FillDynamicMember(root, dynamicInfo, material, DAVA::NMaterialParamName::PARAM_LIGHTMAP_SIZE, hiddenProps);
        }
    }

    void FillDynamicMember(QtPropertyData* root, DAVA::InspInfoDynamic* dynamic, DAVA::NMaterial* material, const DAVA::FastName& memberName, DAVA::KeyedArchive* hiddenProps)
    {
        if (hiddenProps != nullptr && hiddenProps->GetBool(DAVA::String(memberName.c_str())) == true)
        {
            return;
        }

        DAVA::InspInfoDynamic::DynamicData ddata = dynamic->Prepare(material, false);
        root->MergeChild(std::unique_ptr<QtPropertyData>(new QtPropertyDataInspDynamic(memberName, dynamic, ddata)));
    }

    void FillDynamicMembers(QtPropertyData* root, DAVA::InspInfoDynamic* dynamic, DAVA::NMaterial* material, bool isGlobal, DAVA::KeyedArchive* hiddenProps)
    {
        DAVA::InspInfoDynamic::DynamicData ddata = dynamic->Prepare(material, isGlobal);
        DAVA::Vector<DAVA::FastName> membersList = dynamic->MembersList(ddata);

        std::sort(membersList.begin(), membersList.end(), [](const DAVA::FastName& left, const DAVA::FastName& right) {
            return strcmp(left.c_str(), right.c_str()) < 0;
        });

        bool rootIsFlags = root->GetName() == UIName::Flags;
        bool rootIsProps = root->GetName() == UIName::Properties;

        // enumerate dynamic members and add them
        for (size_t i = 0; i < membersList.size(); ++i)
        {
            const DAVA::FastName& name = membersList[i];
            if (hiddenProps != nullptr && hiddenProps->GetBool(DAVA::String(name.c_str())) == true)
            {
                continue;
            }

            if (rootIsFlags && (name == DAVA::NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_CASTER || name == DAVA::NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_RECEIVER))
            { // it will be shown in section illumination
                continue;
            }

            if (rootIsProps && (name == DAVA::NMaterialParamName::PARAM_LIGHTMAP_SIZE))
            { // it will be shown in section illumination
                continue;
            }

            root->MergeChild(std::unique_ptr<QtPropertyData>(new QtPropertyDataInspDynamic(name, dynamic, ddata)));
        }
    }

    void ApplyTextureValidator(QtPropertyData* data)
    {
        QtPropertyDataInspDynamic* dynamicData = dynamic_cast<QtPropertyDataInspDynamic*>(data);
        if (dynamicData)
        {
            DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
            DVASSERT(data != nullptr);
            QString defaultPath = data->GetProjectPath().GetAbsolutePathname().c_str();
            DAVA::FilePath dataSourcePath = data->GetDataSource3DPath();

            // calculate appropriate default path
            if (DAVA::FileSystem::Instance()->Exists(dataSourcePath))
            {
                defaultPath = dataSourcePath.GetAbsolutePathname().c_str();
            }

            DAVA::SceneEditor2* activeScene = editor->activeScene;
            if ((nullptr != activeScene) && DAVA::FileSystem::Instance()->Exists(activeScene->GetScenePath()))
            {
                DAVA::String scenePath = activeScene->GetScenePath().GetDirectory().GetAbsolutePathname();
                if (DAVA::String::npos != scenePath.find(dataSourcePath.GetAbsolutePathname()))
                {
                    defaultPath = scenePath.c_str();
                }
            }

            // create validator
            dynamicData->SetDefaultOpenDialogPath(defaultPath);

            dynamicData->SetOpenDialogFilter(PathDescriptor::GetPathDescriptor(PathDescriptor::PATH_TEXTURE).fileFilter);
            QStringList path;
            path.append(dataSourcePath.GetAbsolutePathname().c_str());
            dynamicData->SetValidator(new TexturePathValidator(path));
        }

        for (int i = 0; i < data->ChildCount(); ++i)
        {
            ApplyTextureValidator(data->ChildGet(i));
        }
    }

private:
    MaterialEditor* editor = nullptr;
    DAVA::NMaterial* globalMaterial = nullptr;
};

class MaterialEditor::ConfigNameValidator : public QValidator
{
public:
    ConfigNameValidator(QObject* parent)
        : QValidator(parent)
    {
    }

    State validate(QString& input, int& pos) const override
    {
        DVASSERT(material != nullptr);
        if (input.isEmpty())
            return Intermediate;

        DAVA::uint32 index = material->FindConfigByName(DAVA::FastName(input.toStdString()));
        return (index < material->GetConfigCount()) ? Intermediate : Acceptable;
    }

    void SetCurrentMaterial(DAVA::NMaterial* material_)
    {
        material = material_;
    }

private:
    DAVA::NMaterial* material = nullptr;
};

MaterialEditor::MaterialEditor(QWidget* parent /* = 0 */)
    : QDialog(parent)
    , ui(new Ui::MaterialEditor)
    , templatesFilterModel(nullptr)
    , validator(new ConfigNameValidator(this))
{
    lastCheckState = DAVA::ApplyMaterialPresetCommand::PROPERTIES | DAVA::ApplyMaterialPresetCommand::TEMPLATE | DAVA::ApplyMaterialPresetCommand::GROUP;
    DAVA::Function<void()> fn(this, &MaterialEditor::RefreshMaterialProperties);
    materialPropertiesUpdater = new LazyUpdater(fn, this);

    ui->setupUi(this);
    setWindowFlags(DAVA::WINDOWFLAG_ON_TOP_OF_APPLICATION);

    ui->tabbar->setNameValidator(validator);
    ui->tabbar->setUsesScrollButtons(true);
    ui->tabbar->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(ui->tabbar, &EditableTabBar::tabNameChanged, this, &MaterialEditor::onTabNameChanged);
    QObject::connect(ui->tabbar, &EditableTabBar::currentChanged, this, &MaterialEditor::onCurrentConfigChanged);
    QObject::connect(ui->tabbar, &EditableTabBar::tabCloseRequested, this, &MaterialEditor::onTabRemove);
    QObject::connect(ui->tabbar, &EditableTabBar::customContextMenuRequested, this, &MaterialEditor::onTabContextMenuRequested);

    ui->materialTree->setDragEnabled(true);
    ui->materialTree->setAcceptDrops(true);
    ui->materialTree->setDragDropMode(QAbstractItemView::DragDrop);

    ui->materialProperty->SetEditTracking(true);
    ui->materialProperty->setContextMenuPolicy(Qt::CustomContextMenu);

    // global scene manager signals
    QObject::connect(SceneSignals::Instance(), &SceneSignals::Activated, this, &MaterialEditor::sceneActivated);
    QObject::connect(SceneSignals::Instance(), &SceneSignals::Deactivated, this, &MaterialEditor::sceneDeactivated);
    QObject::connect(SceneSignals::Instance(), &SceneSignals::CommandExecuted, this, &MaterialEditor::commandExecuted);

    selectionFieldBinder.reset(new DAVA::FieldBinder(DAVA::Deprecated::GetAccessor()));
    {
        DAVA::FieldDescriptor fieldDescr;
        fieldDescr.type = DAVA::ReflectedTypeDB::Get<DAVA::SelectionData>();
        fieldDescr.fieldName = DAVA::FastName(DAVA::SelectionData::selectionPropertyName);
        selectionFieldBinder->BindField(fieldDescr, [this](const DAVA::Any&) { autoExpand(); });
    }

    {
        DAVA::Reflection model = DAVA::Reflection::Create(DAVA::ReflectedObject(this));
        DAVA::CheckBox::Params p(DAVA::Deprecated::GetAccessor(), DAVA::Deprecated::GetUI(), DAVA::mainWindowKey);
        p.fields[DAVA::CheckBox::Fields::Checked] = "applyPropsFilter";
        p.fields[DAVA::CheckBox::Fields::TextHint].BindConstValue("Apply properties filter");
        DAVA::CheckBox* w = new DAVA::CheckBox(p, p.accessor, model, ui->toolbar);
        ui->toolbar->addWidget(w->ToWidgetCast());
    }

    // material tree
    QObject::connect(ui->materialTree->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this, SLOT(materialSelected(const QItemSelection&, const QItemSelection&)));
    QObject::connect(ui->materialTree, SIGNAL(Updated()), this, SLOT(autoExpand()));
    QObject::connect(ui->materialTree, SIGNAL(ContextMenuPrepare(QMenu*)), this, SLOT(onContextMenuPrepare(QMenu*)));

    // material properties
    QObject::connect(ui->materialProperty, SIGNAL(PropertyEdited(const QModelIndex&)), this, SLOT(OnPropertyEdited(const QModelIndex&)));
    QObject::connect(ui->materialProperty, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(OnMaterialPropertyEditorContextMenuRequest(const QPoint&)));
    QObject::connect(ui->templateBox, SIGNAL(activated(int)), this, SLOT(OnTemplateChanged(int)));
    QObject::connect(ui->templateButton, SIGNAL(clicked()), this, SLOT(OnTemplateButton()));
    QObject::connect(ui->actionAddGlobalMaterial, SIGNAL(triggered(bool)), this, SLOT(OnMaterialAddGlobal(bool)));
    QObject::connect(ui->actionRemoveGlobalMaterial, SIGNAL(triggered(bool)), this, SLOT(OnMaterialRemoveGlobal(bool)));
    QObject::connect(ui->actionSaveMaterialPreset, SIGNAL(triggered(bool)), this, SLOT(OnMaterialSave(bool)));
    QObject::connect(ui->actionSaveMaterialCurrentConfigPreset, SIGNAL(triggered(bool)), this, SLOT(OnMaterialSaveCurrentConfig(bool)));
    QObject::connect(ui->actionLoadMaterialPreset, SIGNAL(triggered(bool)), this, SLOT(OnMaterialLoad(bool)));

    QObject::connect(SceneSignals::Instance(), &SceneSignals::QualityChanged, this, &MaterialEditor::OnQualityChanged);

    posSaver.Attach(this);
    new QtPosSaver(ui->splitter);
    treeStateHelper = new PropertyEditorStateHelper(ui->materialProperty, (QtPropertyModel*)ui->materialProperty->model());

    DAVA::VariantType v1 = posSaver.LoadValue("splitPosProperties");
    DAVA::VariantType v2 = posSaver.LoadValue("splitPosPreview");
    if (v1.GetType() == DAVA::VariantType::TYPE_INT32)
        ui->materialProperty->header()->resizeSection(0, v1.AsInt32());
    if (v2.GetType() == DAVA::VariantType::TYPE_INT32)
        ui->materialProperty->header()->resizeSection(1, v2.AsInt32());

    DAVA::VariantType savePath = posSaver.LoadValue("lastSavePath");
    DAVA::VariantType loadState = posSaver.LoadValue("lastLoadPresetState");
    if (savePath.GetType() == DAVA::VariantType::TYPE_FILEPATH)
        lastSavePath = savePath.AsFilePath();
    if (loadState.GetType() == DAVA::VariantType::TYPE_UINT32)
        lastCheckState = loadState.AsUInt32();

    DAVA::VariantType applyPropsFilterV = posSaver.LoadValue("applyPropsFilter");
    if (applyPropsFilterV.GetType() == DAVA::VariantType::TYPE_BOOLEAN)
    {
        applyHiddenPropsFilter = applyPropsFilterV.AsBool();
    }

    expandMap[MaterialFilteringModel::SHOW_ALL] = false;
    expandMap[MaterialFilteringModel::SHOW_ONLY_INSTANCES] = true;
    expandMap[MaterialFilteringModel::SHOW_INSTANCES_AND_MATERIALS] = true;

    initActions();

    DAVA::FieldDescriptor descr;
    descr.fieldName = DAVA::FastName("hiddenFieldsConfig");
    descr.type = DAVA::ReflectedTypeDB::Get<MaterialEditorSettings>();
    selectionFieldBinder->BindField(descr, [this](const DAVA::Any& v) {
        hiddenFieldsConfig.Set(new DAVA::KeyedArchive());
        bool loaded = false;
        if (v.CanGet<DAVA::FilePath>() == true)
        {
            DAVA::FileSystem* fileSystem = DAVA::GetEngineContext()->fileSystem;
            DAVA::FilePath path = v.Get<DAVA::FilePath>();
            if (path.IsEmpty() == false && fileSystem->Exists(path))
            {
                loaded = MaterialEditorDetail::LoadHiddenFieldsConfig(hiddenFieldsConfig, path);
            }
        }

        if (loaded == false)
        {
            MaterialEditorDetail::LoadHiddenFieldsConfig(hiddenFieldsConfig, DAVA::FilePath("~res:/Materials/hiddenMaterialProperties.yaml"));
        }
    });
}

MaterialEditor::~MaterialEditor()
{
    delete treeStateHelper;

    DAVA::VariantType v1(ui->materialProperty->header()->sectionSize(0));
    DAVA::VariantType v2(ui->materialProperty->header()->sectionSize(1));
    posSaver.SaveValue("splitPosProperties", v1);
    posSaver.SaveValue("splitPosPreview", v2);
    posSaver.SaveValue("lastSavePath", DAVA::VariantType(lastSavePath));
    posSaver.SaveValue("lastLoadPresetState", DAVA::VariantType(lastCheckState));
    posSaver.SaveValue("applyPropsFilter", DAVA::VariantType(applyHiddenPropsFilter));
}

QtPropertyData* MaterialEditor::AddSection(const DAVA::FastName& sectionName)
{
    QtPropertyData* section = new QtPropertyData(sectionName);
    ui->materialProperty->AppendProperty(std::unique_ptr<QtPropertyData>(section));
    ui->materialProperty->ApplyStyle(section, QtPropertyEditor::HEADER_STYLE);
    return section;
}

void MaterialEditor::initActions()
{
    ui->actionShowAll->setData(MaterialFilteringModel::SHOW_ALL);
    ui->actionInstances->setData(MaterialFilteringModel::SHOW_ONLY_INSTANCES);
    ui->actionMaterialsInstances->setData(MaterialFilteringModel::SHOW_INSTANCES_AND_MATERIALS);

    const int filterType = MaterialFilteringModel::SHOW_ALL;
    foreach (QAction* action, ui->filterType->actions())
    {
        QObject::connect(action, SIGNAL(triggered()), SLOT(onFilterChanged()));

        if (action->data().toInt() == filterType)
        {
            action->activate(QAction::Trigger);
        }
    }

    QObject::connect(ui->actionAutoExpand, SIGNAL(triggered(bool)), SLOT(onCurrentExpandModeChange(bool)));
}

void MaterialEditor::initTemplates()
{
    if (nullptr == templatesFilterModel)
    {
        QStandardItemModel* templatesModel = new QStandardItemModel(this);

        DAVA::ProjectManagerData* projectManagerData = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
        DVASSERT(projectManagerData != nullptr);
        const DAVA::Vector<DAVA::MaterialTemplateInfo>* templates = projectManagerData->GetMaterialTemplatesInfo();

        QStandardItem* emptyItem = new QStandardItem(QString());
        templatesModel->appendRow(emptyItem);

        for (const DAVA::MaterialTemplateInfo& t : *templates)
        {
            QStandardItem* item = new QStandardItem();
            item->setText(t.name.c_str());
            item->setData(t.path.c_str(), Qt::UserRole);
            templatesModel->appendRow(item);
        }

        templatesFilterModel = new MaterialTemplateModel(this);
        templatesFilterModel->setSourceModel(templatesModel);

        ui->templateBox->setModel(templatesFilterModel);
    }
}

void MaterialEditor::setTemplatePlaceholder(const QString& text)
{
    QAbstractItemModel* model = ui->templateBox->model();
    const QModelIndex index = model->index(0, 0);
    model->setData(index, text, Qt::DisplayRole);
}

void MaterialEditor::autoExpand()
{
    QAction* action = ui->filterType->checkedAction();
    if (nullptr == action)
        return;

    const int filterType = action->data().toInt();

    if (expandMap[filterType])
        ui->materialTree->expandAll();
}

void MaterialEditor::onFilterChanged()
{
    QAction* action = ui->filterType->checkedAction();
    if (nullptr == action)
        return;

    const int filterType = action->data().toInt();
    ui->materialTree->setFilterType(filterType);

    ui->actionAutoExpand->setChecked(expandMap[filterType]);
    onCurrentExpandModeChange(expandMap[filterType]);
}

void MaterialEditor::SelectMaterial(DAVA::NMaterial* material)
{
    ui->materialTree->SelectMaterial(material);
}

void MaterialEditor::SelectEntities(DAVA::NMaterial* material)
{
    QList<DAVA::NMaterial*> materials;
    materials << material;
    ui->materialTree->SelectEntities(materials);
}

void MaterialEditor::SetCurMaterial(const QList<DAVA::NMaterial*>& materials)
{
    int curScrollPos = ui->materialProperty->verticalScrollBar()->value();

    curMaterials = materials;
    treeStateHelper->SaveTreeViewState(false);
    UpdateTabs();

    ui->materialProperty->RemovePropertyAll();
    PropertiesBuilder builder(this);
    DAVA::Vector<std::unique_ptr<QtPropertyData>> properties;
    builder.Build(curMaterials, properties);
    ui->materialProperty->AppendProperties(std::move(properties));

    FillTemplates(materials);

    // Restore back the tree view state from the shared storage.
    if (!treeStateHelper->IsTreeStateStorageEmpty())
    {
        treeStateHelper->RestoreTreeViewState();
    }
    else
    {
        // Expand the root elements as default value.
        ui->materialProperty->expandToDepth(0);
    }

    // check if there is global material and enable appropriate actions
    if (nullptr != activeScene)
    {
        bool isGlobalMaterialPresent = (nullptr != activeScene->GetGlobalMaterial());
        ui->actionAddGlobalMaterial->setEnabled(!isGlobalMaterialPresent);
        ui->actionRemoveGlobalMaterial->setEnabled(isGlobalMaterialPresent);
    }
    else
    {
        ui->actionAddGlobalMaterial->setEnabled(false);
        ui->actionRemoveGlobalMaterial->setEnabled(false);
    }

    ui->materialProperty->verticalScrollBar()->setValue(curScrollPos);
}

void MaterialEditor::sceneActivated(DAVA::SceneEditor2* scene)
{
    activeScene = scene;
    UpdateContent(activeScene);
}

void MaterialEditor::sceneDeactivated(DAVA::SceneEditor2* scene)
{
    activeScene = nullptr;
    UpdateContent(activeScene);
}

void MaterialEditor::materialSelected(const QItemSelection& selected, const QItemSelection& deselected)
{
    QList<DAVA::NMaterial*> materials;
    QItemSelectionModel* selection = ui->materialTree->selectionModel();
    const QModelIndexList selectedRows = selection->selectedRows();

    foreach (const QModelIndex& index, selectedRows)
    {
        if (index.column() == 0)
        {
            DAVA::NMaterial* material = ui->materialTree->GetMaterial(index);
            if (material)
                materials << material;
        }
    }

    SetCurMaterial(materials);
}

void MaterialEditor::commandExecuted(DAVA::SceneEditor2* scene, const DAVA::RECommandNotificationObject& commandNotification)
{
    if (scene != activeScene)
    {
        return;
    }

    int curScrollPos = ui->materialProperty->verticalScrollBar()->value();
    SCOPE_EXIT
    {
        ui->materialProperty->verticalScrollBar()->setValue(curScrollPos);
    };

    if (commandNotification.MatchCommandTypes<DAVA::MaterialGlobalSetCommand>())
    {
        sceneActivated(scene);
        materialPropertiesUpdater->Update();
    }

    if (commandNotification.MatchCommandTypes<DAVA::MaterialRemoveTexture>())
    {
        materialPropertiesUpdater->Update();
    }

    if (commandNotification.MatchCommandTypes<DAVA::MaterialChangeCurrentConfig, DAVA::MaterialCreateConfig,
                                              DAVA::MaterialRemoveConfig, DAVA::ApplyMaterialPresetCommand>())
    {
        RefreshMaterialProperties();
    }

    commandNotification.ForEach<DAVA::InspMemberModifyCommand>([&](const DAVA::InspMemberModifyCommand* cmd) {
        const DAVA::String memberName(cmd->member->Name().c_str());
        if (memberName == DAVA::NMaterialSerializationKey::QualityGroup || memberName == DAVA::NMaterialSerializationKey::FXName)
        {
            for (auto& m : curMaterials)
            {
                m->InvalidateRenderVariants();
            }
            materialPropertiesUpdater->Update();
        }
        if (memberName == "configName")
        {
            materialPropertiesUpdater->Update();
        }
    });

    commandNotification.ForEach<DAVA::InspDynamicModifyCommand>([&](const DAVA::InspDynamicModifyCommand* cmd) {
        // if material flag was changed we should rebuild list of all properties because their set can be changed
        if (cmd->dynamicInfo->GetMember()->Name() == NMaterialSectionName::LocalFlags)
        {
            if (cmd->key == DAVA::NMaterialFlagName::FLAG_ILLUMINATION_USED)
            {
                DAVA::NMaterial* material = static_cast<DAVA::NMaterial*>(cmd->ddata.object);
                if (material->HasLocalFlag(DAVA::NMaterialFlagName::FLAG_ILLUMINATION_USED) && material->GetLocalFlagValue(DAVA::NMaterialFlagName::FLAG_ILLUMINATION_USED) == 1)
                {
                    AddMaterialFlagIfNeed(material, DAVA::NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_CASTER);
                    AddMaterialFlagIfNeed(material, DAVA::NMaterialFlagName::FLAG_ILLUMINATION_SHADOW_RECEIVER);
                    material->InvalidateRenderVariants();
                }
            }

            materialPropertiesUpdater->Update();
        }
        else
        {
            PropertiesBuilder(this).UpdateAllAddRemoveButtons(ui->materialProperty->GetRootProperty());
        }
    });
}

void MaterialEditor::AddMaterialFlagIfNeed(DAVA::NMaterial* material, const DAVA::FastName& flagName)
{
    bool hasFlag = false;
    DAVA::NMaterial* parent = material;
    while (parent)
    {
        if (parent->HasLocalFlag(flagName))
        {
            hasFlag = true;
            break;
        }

        parent = parent->GetParent();
    }

    if (!hasFlag)
    {
        material->AddFlag(flagName, 0);
    }
}

bool MaterialEditor::HasMaterialProperty(DAVA::NMaterial* material, const DAVA::FastName& paramName)
{
    while (material != nullptr)
    {
        if (material->HasLocalProperty(paramName))
        {
            return true;
        }

        material = material->GetParent();
    }

    return false;
}

void MaterialEditor::OnQualityChanged()
{
    RefreshMaterialProperties();
}

void MaterialEditor::onCurrentExpandModeChange(bool mode)
{
    QAction* action = ui->filterType->checkedAction();
    if (nullptr == action)
        return;

    const int filterType = action->data().toInt();
    expandMap[filterType] = mode;

    if (mode)
    {
        ui->materialTree->expandAll();
    }
    else
    {
        ui->materialTree->collapseAll();
    }
}

void MaterialEditor::showEvent(QShowEvent* event)
{
    FillTemplates(QList<DAVA::NMaterial*>());
    UpdateContent(activeScene);
}

void MaterialEditor::closeEvent(QCloseEvent* event)
{
    UpdateContent(activeScene);
    RefreshMaterialProperties();
    QDialog::closeEvent(event);
}

void MaterialEditor::FillTemplates(const QList<DAVA::NMaterial*>& materials)
{
    initTemplates();

    if (1 == materials.size() && materials[0])
    {
        DAVA::NMaterial* material = materials[0];

        //Read material params
        DAVA::NMaterial* globalMaterial = (nullptr == activeScene) ? nullptr : activeScene->GetGlobalMaterial();

        const bool isGlobalMaterial = (material == globalMaterial);
        if (isGlobalMaterial)
        { // reset state
            ui->templateBox->setCurrentIndex(-1);
            ui->templateBox->setEnabled(false);
            ui->templateButton->setEnabled(false);
            ui->templateButton->setIcon(DAVA::SharedIcon(":/QtIcons/cplus.png"));
        }
        else
        {
            bool isAssignableFx = true;
            { //set fx name to fx template box
                int rowToSelect = -1;
                const DAVA::FastName fxName = material->GetEffectiveFXName();
                if (fxName.IsValid())
                {
                    QAbstractItemModel* model = ui->templateBox->model();
                    const int n = model->rowCount();
                    for (int i = 0; i < n; i++)
                    {
                        const QModelIndex index = model->index(i, 0);
                        if (index.data(Qt::UserRole).toString() == fxName.c_str())
                        {
                            rowToSelect = i;
                            break;
                        }
                    }

                    if (-1 == rowToSelect)
                    {
                        setTemplatePlaceholder(QString("NON-ASSIGNABLE: %1").arg(fxName.c_str()));
                        rowToSelect = 0;
                        isAssignableFx = false;
                    }
                }

                ui->templateBox->setCurrentIndex(rowToSelect);
            }

            { //update button state

                const bool hasLocalFxName = material->HasLocalFXName();

                DAVA::NMaterial* parentMaterial = material->GetParent();
                bool hasParentFx = false;
                if (parentMaterial != nullptr)
                {
                    hasParentFx = parentMaterial->HasLocalFXName();
                }

                if (hasLocalFxName)
                {
                    ui->templateButton->setIcon(DAVA::SharedIcon(":/QtIcons/cminus.png"));
                }
                else
                {
                    ui->templateButton->setIcon(DAVA::SharedIcon(":/QtIcons/cplus.png"));
                }

                if (parentMaterial == nullptr || parentMaterial == globalMaterial || isAssignableFx == false)
                {
                    ui->templateButton->setEnabled(false);
                }
                else
                {
                    ui->templateButton->setEnabled(true);
                }

                ui->templateBox->setEnabled(hasLocalFxName && isAssignableFx);
            }
        }
    }
    else
    { // reset state
        ui->templateBox->setCurrentIndex(-1);
        ui->templateBox->setEnabled(false);
        ui->templateButton->setEnabled(false);
        ui->templateButton->setIcon(DAVA::SharedIcon(":/QtIcons/cplus.png"));
    }
}

void MaterialEditor::OnTemplateChanged(int index)
{
    if (curMaterials.size() == 1 && index > 0)
    {
        DAVA::NMaterial* material = curMaterials.at(0);
        QString newTemplatePath = GetTemplatePath(index);
        if (!newTemplatePath.isEmpty())
        {
            const DAVA::InspMember* templateMember = material->GetTypeInfo()->Member(DAVA::FastName("fxName"));

            if (nullptr != templateMember)
            {
                activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::InspMemberModifyCommand(templateMember, material,
                                                                                                   DAVA::VariantType(DAVA::FastName(newTemplatePath.toStdString().c_str())))));
            }
        }
    }

    RefreshMaterialProperties();
}

void MaterialEditor::OnTemplateButton()
{
    if (1 == curMaterials.size())
    {
        DAVA::NMaterial* material = curMaterials[0];
        const DAVA::InspMember* templateMember = material->GetTypeInfo()->Member(DAVA::FastName("fxName"));

        if (nullptr != templateMember)
        {
            DVASSERT(activeScene != nullptr);
            if (material->HasLocalFXName())
            {
                // has local fxname, so button shoud remove it (by setting empty value)
                activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::InspMemberModifyCommand(templateMember, material, DAVA::VariantType(DAVA::FastName()))));
            }
            else
            {
                // no local fxname, so button should add it
                activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::InspMemberModifyCommand(templateMember, material, DAVA::VariantType(material->GetEffectiveFXName()))));
            }

            RefreshMaterialProperties();
        }
    }
}

QString MaterialEditor::GetTemplatePath(int index) const
{
    return ui->templateBox->itemData(index, Qt::UserRole).toString();
}

void MaterialEditor::OnAddRemoveButton()
{
    QtPropertyToolButton* btn = dynamic_cast<QtPropertyToolButton*>(QObject::sender());
    if (nullptr != btn)
    {
        QtPropertyDataInspDynamic* data = static_cast<QtPropertyDataInspDynamic*>(btn->GetPropertyData());
        if (nullptr != data)
        {
            int memberFlags = data->dynamicInfo->MemberFlags(data->ddata, data->name);

            // pressed remove button
            if (memberFlags & DAVA::I_EDIT)
            {
                data->SetValue(QVariant(), QtPropertyData::VALUE_SOURCE_CHANGED);
            }
            // pressed add button
            else
            {
                data->SetValue(data->GetValue(), QtPropertyData::VALUE_EDITED);
            }

            data->EmitDataChanged(QtPropertyData::VALUE_EDITED);
            PropertiesBuilder(this).UpdateAddRemoveButtonState(data);
        }
    }
}

void MaterialEditor::OnReloadTexture()
{
    QtPropertyToolButton* btn = dynamic_cast<QtPropertyToolButton*>(QObject::sender());
    if (nullptr != btn)
    {
        QtPropertyDataInspDynamic* data = static_cast<QtPropertyDataInspDynamic*>(btn->GetPropertyData());
        if (nullptr != data)
        {
            DAVA::VariantType value = data->dynamicInfo->MemberValueGet(data->ddata, data->name);
            DAVA::FilePath path = value.AsFilePath();
            DAVA::Texture* texture = DAVA::Texture::Get(path);
            if (texture != nullptr)
            {
                DAVA::Vector<DAVA::Texture*> reloadTextures;
                reloadTextures.push_back(texture);
                DAVA::Deprecated::GetInvoker()->Invoke(DAVA::ReloadTextures.ID, reloadTextures);
            }
        }
    }
}

void MaterialEditor::OnPropertyEdited(const QModelIndex& index)
{
    QtPropertyEditor* editor = dynamic_cast<QtPropertyEditor*>(QObject::sender());
    if (editor != nullptr && activeScene != nullptr)
    {
        QtPropertyData* propData = editor->GetProperty(index);
        if (nullptr != propData)
        {
            activeScene->BeginBatch("Property multiedit", propData->GetMergedItemCount() + 1);

            auto commandsAccumulateFn = [this](QtPropertyData* item) {
                std::unique_ptr<DAVA::Command> command = item->CreateLastCommand();
                if (command)
                {
                    activeScene->Exec(std::move(command));
                }
                return true;
            };

            propData->ForeachMergedItem(commandsAccumulateFn);
            commandsAccumulateFn(propData);

            activeScene->EndBatch();
        }
    }
}

void MaterialEditor::OnMaterialAddGlobal(bool checked)
{
    if (nullptr != activeScene)
    {
        DAVA::ScopedPtr<DAVA::NMaterial> global(new DAVA::NMaterial());

        global->SetMaterialName(DAVA::FastName("Scene_Global_Material"));
        activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::MaterialGlobalSetCommand(activeScene, global)));

        sceneActivated(activeScene);
        SelectMaterial(activeScene->GetGlobalMaterial());
    }
}

void MaterialEditor::OnMaterialRemoveGlobal(bool checked)
{
    if (nullptr != activeScene)
    {
        activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::MaterialGlobalSetCommand(activeScene, nullptr)));
        sceneActivated(activeScene);
    }
}

void MaterialEditor::onContextMenuPrepare(QMenu* menu)
{
    if (curMaterials.size() > 0)
    {
        ui->actionSaveMaterialPreset->setEnabled(curMaterials.size() == 1);
        ui->actionSaveMaterialCurrentConfigPreset->setEnabled(curMaterials.size() == 1);
        menu->addSeparator();
        menu->addAction(ui->actionLoadMaterialPreset);
        menu->addAction(ui->actionSaveMaterialPreset);
        menu->addAction(ui->actionSaveMaterialCurrentConfigPreset);
    }
}

void MaterialEditor::OnMaterialPropertyEditorContextMenuRequest(const QPoint& pos)
{
    if (nullptr != activeScene && curMaterials.size() == 1)
    {
        DAVA::NMaterial* globalMaterial = activeScene->GetGlobalMaterial();
        DAVA::NMaterial* material = curMaterials[0];

        QModelIndex index = ui->materialProperty->indexAt(pos);

        if (globalMaterial != material && index.column() == 0 && (nullptr != globalMaterial))
        {
            QtPropertyData* data = ui->materialProperty->GetProperty(index);
            if (nullptr != data && data->Parent()->GetName() == UIName::Properties)
            {
                QtPropertyDataInspDynamic* dynamicData = (QtPropertyDataInspDynamic*)data;
                if (nullptr != dynamicData)
                {
                    const DAVA::FastName& propertyName = dynamicData->name;
                    bool hasProperty = material->HasLocalProperty(propertyName);
                    if (hasProperty)
                    {
                        QMenu menu(this);
                        menu.addAction("Add to Global Material");
                        QAction* resultAction = menu.exec(ui->materialProperty->viewport()->mapToGlobal(pos));

                        if (nullptr != resultAction)
                        {
                            globalMaterial->SetPropertyValue(propertyName, material->GetLocalPropValue(propertyName));
                            activeScene->SetChanged();
                        }
                    }
                }
            }
        }
    }
}

void MaterialEditor::OnMaterialSave(bool checked)
{
    SaveMaterialPresetImpl(DAVA::MakeFunction(&DAVA::ApplyMaterialPresetCommand::StoreAllConfigsPreset));
}

void MaterialEditor::OnMaterialSaveCurrentConfig(bool checked)
{
    SaveMaterialPresetImpl(DAVA::MakeFunction(&DAVA::ApplyMaterialPresetCommand::StoreCurrentConfigPreset));
}

void MaterialEditor::SaveMaterialPresetImpl(const DAVA::Function<void(DAVA::KeyedArchive*, DAVA::NMaterial*, const DAVA::SerializationContext&)>& saveFunction)
{
    if (curMaterials.size() == 1)
    {
        QString outputFile = FileDialog::getSaveFileName(this, "Save Material Preset", lastSavePath.GetAbsolutePathname().c_str(),
                                                         "Material Preset (*.mpreset)");

        if (!outputFile.isEmpty() && (nullptr != activeScene))
        {
            DAVA::ProjectManagerData* data = DAVA::Deprecated::GetDataNode<DAVA::ProjectManagerData>();
            DVASSERT(data != nullptr);

            lastSavePath = outputFile.toLatin1().data();

            DAVA::SerializationContext materialContext;
            materialContext.SetScene(activeScene);
            materialContext.SetScenePath(data->GetProjectPath());
            materialContext.SetVersion(DAVA::VersionInfo::Instance()->GetCurrentVersion().version);

            DAVA::ScopedPtr<DAVA::KeyedArchive> presetArchive(new DAVA::KeyedArchive());
            saveFunction(presetArchive, curMaterials.front(), materialContext);
            presetArchive->SaveToYamlFile(lastSavePath);
        }
    }
    else
    {
        QMessageBox::warning(this, "Material properties save", "It is allowed to save only single material");
    }
}

void MaterialEditor::OnMaterialLoad(bool checked)
{
    if (curMaterials.size() > 0)
    {
        QString inputFile = FileDialog::getOpenFileName(this, "Load Material Preset",
                                                        lastSavePath.GetAbsolutePathname().c_str(), "Material Preset (*.mpreset)");

        if (!inputFile.isEmpty() && (nullptr != activeScene))
        {
            lastSavePath = inputFile.toLatin1().data();

            std::unique_ptr<DAVA::ApplyMaterialPresetCommand> command = std::make_unique<DAVA::ApplyMaterialPresetCommand>(lastSavePath, curMaterials.front(), activeScene);

            // not checking version right now
            // version info is reserved for future use
            if (command->IsValidPreset())
            {
                if (ExecMaterialLoadingDialog(lastCheckState, inputFile))
                {
                    command->Init(lastCheckState);
                    activeScene->Exec(std::move(command));
                }
            }
            else
            {
                QMessageBox::warning(this, "Material preset is not supported",
                                     "Material preset you are trying to open is either old or invalid.");
            }
        }
    }

    RefreshMaterialProperties();
}

bool MaterialEditor::ExecMaterialLoadingDialog(DAVA::uint32& userChoise, const QString& inputFile)
{
    QDialog dlg;
    QVBoxLayout* dlgLayout = new QVBoxLayout();
    dlgLayout->setMargin(10);
    dlgLayout->setSpacing(15);

    dlg.setWindowTitle("Loading material preset...");
    dlg.setWindowFlags(Qt::Tool);
    dlg.setLayout(dlgLayout);

    QLineEdit* pathLine = new QLineEdit(&dlg);
    pathLine->setText(inputFile);
    pathLine->setReadOnly(false);
    pathLine->setToolTip(inputFile);
    dlgLayout->addWidget(pathLine);

    QGroupBox* groupbox = new QGroupBox("Load parameters", &dlg);
    dlgLayout->addWidget(groupbox);

    QCheckBox* templateChBox = new QCheckBox(QString(UIName::Template.c_str()), groupbox);
    QCheckBox* groupChBox = new QCheckBox(QString(UIName::Group.c_str()), groupbox);
    QCheckBox* propertiesChBox = new QCheckBox(QString(UIName::Properties.c_str()), groupbox);
    QCheckBox* texturesChBox = new QCheckBox(QString(UIName::Textures.c_str()), groupbox);

    templateChBox->setChecked((bool)(userChoise & DAVA::ApplyMaterialPresetCommand::TEMPLATE));
    groupChBox->setChecked((bool)(userChoise & DAVA::ApplyMaterialPresetCommand::GROUP));
    propertiesChBox->setChecked((bool)(userChoise & DAVA::ApplyMaterialPresetCommand::PROPERTIES));
    texturesChBox->setChecked((bool)(userChoise & DAVA::ApplyMaterialPresetCommand::TEXTURES));

    QGridLayout* gridLayout = new QGridLayout();
    groupbox->setLayout(gridLayout);
    gridLayout->setHorizontalSpacing(50);
    gridLayout->addWidget(templateChBox, 0, 0);
    gridLayout->addWidget(groupChBox, 1, 0);
    gridLayout->addWidget(propertiesChBox, 0, 1);
    gridLayout->addWidget(texturesChBox, 1, 1);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);
    dlgLayout->addWidget(buttons);

    QObject::connect(buttons, SIGNAL(accepted()), &dlg, SLOT(accept()));
    QObject::connect(buttons, SIGNAL(rejected()), &dlg, SLOT(reject()));

    QDialog::DialogCode retCode = static_cast<QDialog::DialogCode>(dlg.exec());

    if (QDialog::Accepted == retCode)
    {
        userChoise = 0;
        if (templateChBox->checkState() == Qt::Checked)
            userChoise |= DAVA::ApplyMaterialPresetCommand::TEMPLATE;
        if (groupChBox->checkState() == Qt::Checked)
            userChoise |= DAVA::ApplyMaterialPresetCommand::GROUP;
        if (propertiesChBox->checkState() == Qt::Checked)
            userChoise |= DAVA::ApplyMaterialPresetCommand::PROPERTIES;
        if (texturesChBox->checkState() == Qt::Checked)
            userChoise |= DAVA::ApplyMaterialPresetCommand::TEXTURES;
    }

    return retCode == QDialog::Accepted;
}

void MaterialEditor::RefreshMaterialProperties()
{
    SetCurMaterial(curMaterials);
    PropertiesBuilder(this).UpdateAllAddRemoveButtons(ui->materialProperty->GetRootProperty());
    ui->materialProperty->ShowButtonsUnderCursor();
}

void MaterialEditor::removeInvalidTexture()
{
    QtPropertyToolButton* button = dynamic_cast<QtPropertyToolButton*>(sender());
    QtPropertyData* data = button->GetPropertyData();
    DAVA::FastName textureSlot = data->GetName();

    DVASSERT(activeScene != nullptr);

    DAVA::uint32 count = static_cast<DAVA::uint32>(curMaterials.size());
    activeScene->BeginBatch("Remove invalid texture from material", count);
    for (DAVA::uint32 i = 0; i < count; ++i)
    {
        DAVA::NMaterial* material = curMaterials[i];
        while (material != nullptr)
        {
            if (material->HasLocalTexture(textureSlot))
            {
                activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::MaterialRemoveTexture(textureSlot, material)));
                break;
            }

            material = material->GetParent();
        }
    }
    activeScene->EndBatch();
}

void MaterialEditor::UpdateTabs()
{
    MaterialEditorDetail::PropertyLock propertyLock(ui->tabbar, MaterialEditorDetail::CURRENT_TAB_CHANGED_UPDATE);
    if (!propertyLock.IsLocked())
    {
        return;
    }

    while (ui->tabbar->count() > 0)
    {
        ui->tabbar->removeTab(0);
    }

    if (curMaterials.size() == 1)
    {
        DAVA::NMaterial* material = curMaterials.front();
        validator->SetCurrentMaterial(material);
        for (DAVA::uint32 i = 0; i < material->GetConfigCount(); ++i)
        {
            ui->tabbar->addTab(QString(material->GetConfigName(i).c_str()));
        }

        ui->tabbar->setCurrentIndex(material->GetCurrentConfigIndex());
        ui->tabbar->setTabsClosable(material->GetConfigCount() > 1);
    }
    else
    {
        validator->SetCurrentMaterial(nullptr);
    }

    ui->tabbar->setVisible(curMaterials.size() == 1);
}

void MaterialEditor::onTabNameChanged(int index)
{
    DVASSERT(activeScene != nullptr);
    DVASSERT(curMaterials.size() == 1);

    DAVA::NMaterial* material = curMaterials.front();
    const DAVA::InspMember* configNameProperty = material->GetTypeInfo()->Member(DAVA::FastName("configName"));
    DVASSERT(configNameProperty != nullptr);
    DAVA::VariantType newValue(DAVA::FastName(ui->tabbar->tabText(index).toStdString()));
    activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::InspMemberModifyCommand(configNameProperty, material, newValue)));
}

void MaterialEditor::onCreateConfig(int index)
{
    DVASSERT(activeScene != nullptr);
    DVASSERT(curMaterials.size() == 1);
    DAVA::NMaterial* material = curMaterials.front();
    DAVA::MaterialConfig newConfig;
    if (index >= 0)
    {
        newConfig = material->GetConfig(static_cast<DAVA::uint32>(index));
        DAVA::String newConfigName = DAVA::String("_copy");
        if (newConfig.name.IsValid())
        {
            newConfigName = DAVA::String(newConfig.name.c_str()) + newConfigName;
        }

        newConfig.name = DAVA::FastName(newConfigName);
    }
    else
    {
        newConfig.name = DAVA::FastName("Empty");
        newConfig.fxName = material->GetEffectiveFXName();
    }

    DAVA::uint32 counter = 2;
    while (material->FindConfigByName(newConfig.name) < material->GetConfigCount())
    {
        newConfig.name = DAVA::FastName(DAVA::String(newConfig.name.c_str()) + std::to_string(counter));
    }
    activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::MaterialCreateConfig(material, newConfig)));
}

void MaterialEditor::onCurrentConfigChanged(int index)
{
    if (index < 0)
        return;

    MaterialEditorDetail::PropertyLock propertyLock(ui->tabbar, MaterialEditorDetail::CURRENT_TAB_CHANGED_UPDATE);
    if (!propertyLock.IsLocked())
    {
        return;
    }

    DVASSERT(activeScene);
    DVASSERT(curMaterials.size() == 1);
    DAVA::NMaterial* material = curMaterials.front();
    DVASSERT(static_cast<DAVA::uint32>(index) < material->GetConfigCount());
    activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::MaterialChangeCurrentConfig(material, static_cast<DAVA::uint32>(index))));
}

void MaterialEditor::onTabRemove(int index)
{
    DVASSERT(activeScene);
    DVASSERT(curMaterials.size() == 1);
    DAVA::NMaterial* material = curMaterials.front();
    activeScene->Exec(std::unique_ptr<DAVA::Command>(new DAVA::MaterialRemoveConfig(material, static_cast<DAVA::uint32>(index))));
}

void MaterialEditor::onTabContextMenuRequested(const QPoint& pos)
{
    int tabIndex = ui->tabbar->tabAt(pos);

    std::unique_ptr<QMenu> contextMenu(new QMenu());

    if (tabIndex != -1)
    {
        QString actionText = QString("Create copy from %1").arg(ui->tabbar->tabText(tabIndex));
        QAction* createCopy = new QAction(actionText, contextMenu.get());
        QObject::connect(createCopy, &QAction::triggered, [this, tabIndex]() { onCreateConfig(tabIndex); });
        contextMenu->addAction(createCopy);
    }

    QAction* createEmpty = new QAction("Create empty config", contextMenu.get());
    QObject::connect(createEmpty, &QAction::triggered, [this]() { onCreateConfig(-1); });
    contextMenu->addAction(createEmpty);
    contextMenu->exec(ui->tabbar->mapToGlobal(pos));
}

void MaterialEditor::UpdateContent(DAVA::SceneEditor2* scene)
{
    if (isVisible() || scene == nullptr)
    {
        SetCurMaterial(QList<DAVA::NMaterial*>());
        ui->materialTree->SetScene(scene);
        autoExpand();
    }
}

bool MaterialEditor::IsHiddenFilterApplyed() const
{
    return applyHiddenPropsFilter;
}

void MaterialEditor::SetHiddenFilterApplyed(bool isApply)
{
    if (applyHiddenPropsFilter != isApply)
    {
        applyHiddenPropsFilter = isApply;
        RefreshMaterialProperties();
    }
}
