#include "QualitySwitcher.h"
#include "Classes/Qt/MaterialEditor/MaterialEditor.h"

#include <REPlatform/DataNodes/SceneData.h>
#include <REPlatform/DataNodes/Settings/RESettings.h>
#include <REPlatform/Global/GlobalOperations.h>
#include <REPlatform/Scene/Systems/EditorMaterialSystem.h>
#include <REPlatform/Commands/ParticleEditorCommands.h>
#include <REPlatform/Scene/SceneEditor2.h>

#include <TArc/Core/ContextAccessor.h>
#include <TArc/Core/Deprecated.h>
#include <TArc/WindowSubSystem/UI.h>

#include <Base/Set.h>
#include <Render/Highlevel/RenderSystem.h>
#include <Render/Material/NMaterial.h>
#include <Scene3D/Systems/QualitySettingsSystem.h>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

QualitySwitcher* QualitySwitcher::switcherDialog = nullptr;

QualitySwitcher::QualitySwitcher()
    : QDialog(DAVA::Deprecated::GetUI()->GetWindow(DAVA::mainWindowKey), Qt::Dialog | Qt::WindowStaysOnTopHint) //https://bugreports.qt.io/browse/QTBUG-34767
{
    const int spacing = 5;
    const int minColumnW = 150;

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QGroupBox* texturesGroup = new QGroupBox(this);
    QGroupBox* materialsGroup = new QGroupBox(this);
    QGroupBox* optionsGroup = new QGroupBox(this);
    QGroupBox* particlesGroup = new QGroupBox(this);
    QWidget* buttonsGroup = new QWidget(this);

    // textures quality
    {
        QGridLayout* texturesLayout = new QGridLayout(texturesGroup);
        texturesLayout->setColumnMinimumWidth(0, minColumnW);
        texturesLayout->setColumnMinimumWidth(1, minColumnW);

        texturesGroup->setTitle("Textures");
        texturesGroup->setLayout(texturesLayout);

        QLabel* labTx = new QLabel("Textures:", texturesGroup);
        QComboBox* comboTx = new QComboBox(texturesGroup);
        comboTx->setObjectName("TexturesCombo");

        QObject::connect(comboTx, SIGNAL(activated(int)), this, SLOT(OnSetSettingsDirty(int)));

        texturesLayout->addWidget(labTx, 0, 0);
        texturesLayout->addWidget(comboTx, 0, 1);

        DAVA::FastName curTxQuality = DAVA::QualitySettingsSystem::Instance()->GetCurTextureQuality();

        for (size_t i = 0; i < DAVA::QualitySettingsSystem::Instance()->GetTextureQualityCount(); ++i)
        {
            DAVA::FastName txQualityName = DAVA::QualitySettingsSystem::Instance()->GetTextureQualityName(i);
            comboTx->addItem(txQualityName.c_str());

            if (txQualityName == curTxQuality)
            {
                comboTx->setCurrentIndex(comboTx->count() - 1);
            }
        }

        if (DAVA::QualitySettingsSystem::Instance()->GetAnisotropyQualityCount() > 0)
        {
            QLabel* labAn = new QLabel("Anisotropy:", texturesGroup);
            texturesLayout->addWidget(labAn, 1, 0);

            QComboBox* comboAn = new QComboBox(texturesGroup);
            comboAn->setObjectName("AnisotropyCombo");
            QObject::connect(comboAn, SIGNAL(activated(int)), this, SLOT(OnSetSettingsDirty(int)));
            texturesLayout->addWidget(comboAn, 1, 1);

            DAVA::FastName curAnQuality = DAVA::QualitySettingsSystem::Instance()->GetCurAnisotropyQuality();
            for (size_t i = 0; i < DAVA::QualitySettingsSystem::Instance()->GetAnisotropyQualityCount(); ++i)
            {
                DAVA::FastName anQualityName = DAVA::QualitySettingsSystem::Instance()->GetAnisotropyQualityName(i);
                comboAn->addItem(anQualityName.c_str());
                if (anQualityName == curAnQuality)
                {
                    comboAn->setCurrentIndex(comboAn->count() - 1);
                }
            }
        }

        if (DAVA::QualitySettingsSystem::Instance()->GetMSAAQualityCount() > 0)
        {
            QLabel* lab = new QLabel("Multisampling:", texturesGroup);
            texturesLayout->addWidget(lab, 2, 0);

            QComboBox* combo = new QComboBox(texturesGroup);
            combo->setObjectName("MSAACombo");
            QObject::connect(combo, SIGNAL(activated(int)), this, SLOT(OnSetSettingsDirty(int)));
            texturesLayout->addWidget(combo, 2, 1);

            DAVA::FastName curQuality = DAVA::QualitySettingsSystem::Instance()->GetCurMSAAQuality();
            for (size_t i = 0; i < DAVA::QualitySettingsSystem::Instance()->GetMSAAQualityCount(); ++i)
            {
                DAVA::FastName qualityName = DAVA::QualitySettingsSystem::Instance()->GetMSAAQualityName(i);
                combo->addItem(qualityName.c_str());
                if (qualityName == curQuality)
                {
                    combo->setCurrentIndex(combo->count() - 1);
                }
            }
        }
    }

    // materials quality
    {
        QGridLayout* materialsLayout = new QGridLayout(materialsGroup);
        materialsLayout->setColumnMinimumWidth(0, minColumnW);
        materialsLayout->setColumnMinimumWidth(1, minColumnW);

        materialsGroup->setTitle("Materials");
        materialsGroup->setLayout(materialsLayout);

        for (size_t i = 0; i < DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityGroupCount(); ++i)
        {
            DAVA::FastName groupName = DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityGroupName(i);
            DAVA::FastName curGroupQuality = DAVA::QualitySettingsSystem::Instance()->GetCurMaterialQuality(groupName);

            QLabel* labMa = new QLabel(QString(groupName.c_str()) + ":", materialsGroup);
            QComboBox* comboMa = new QComboBox(materialsGroup);
            comboMa->setObjectName(QString(groupName.c_str()) + "Combo");

            QObject::connect(comboMa, SIGNAL(activated(int)), this, SLOT(OnSetSettingsDirty(int)));

            materialsLayout->addWidget(labMa, static_cast<int>(i), 0);
            materialsLayout->addWidget(comboMa, static_cast<int>(i), 1);

            for (size_t j = 0; j < DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityCount(groupName); ++j)
            {
                DAVA::FastName maQualityName = DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityName(groupName, j);
                comboMa->addItem(maQualityName.c_str(), QString(groupName.c_str()));

                if (curGroupQuality == maQualityName)
                {
                    comboMa->setCurrentIndex(comboMa->count() - 1);
                }
            }
        }
    }

    // particles quality
    {
        QGridLayout* particlesLayout = new QGridLayout(particlesGroup);
        particlesLayout->setColumnMinimumWidth(0, minColumnW);
        particlesLayout->setColumnMinimumWidth(1, minColumnW);

        particlesGroup->setTitle("Particles");
        particlesGroup->setLayout(particlesLayout);

        QLabel* labQuality = new QLabel("Quality:", particlesGroup);
        QComboBox* comboQuality = new QComboBox(particlesGroup);
        comboQuality->setObjectName("ParticlesQualityCombo");

        const DAVA::ParticlesQualitySettings& particlesSettings = DAVA::QualitySettingsSystem::Instance()->GetParticlesQualitySettings();
        bool qualityAvailable = particlesSettings.GetQualitiesCount() > 0;

        DAVA::FastName currentQualityName = particlesSettings.GetCurrentQuality();
        for (size_t i = 0, size = particlesSettings.GetQualitiesCount(); i < size; ++i)
        {
            DAVA::FastName qualityName = particlesSettings.GetQualityName(i);
            comboQuality->addItem(qualityName.c_str());

            if (qualityName == currentQualityName)
            {
                comboQuality->setCurrentIndex(comboQuality->count() - 1);
            }
        }
        comboQuality->setEnabled(qualityAvailable);

        particlesLayout->addWidget(labQuality, 0, 0);
        particlesLayout->addWidget(comboQuality, 0, 1);
        connect(comboQuality, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &QualitySwitcher::OnSetSettingsDirty);

        QLabel* labTagsCloud = new QLabel("Tags cloud:", particlesGroup);
        QLineEdit* editTagsCloud = new QLineEdit(particlesGroup);
        editTagsCloud->setObjectName("ParticlesTagsCloudEdit");

        QString tagsCloudText;
        for (const DAVA::FastName& tag : particlesSettings.GetTagsCloud())
        {
            if (!tagsCloudText.isEmpty())
            {
                tagsCloudText += QString(" ");
            }
            tagsCloudText += QString(tag.c_str());
        }

        editTagsCloud->setText(tagsCloudText);
        editTagsCloud->setEnabled(qualityAvailable);

        particlesLayout->addWidget(labTagsCloud, 1, 0);
        particlesLayout->addWidget(editTagsCloud, 1, 1);
        connect(editTagsCloud, &QLineEdit::textChanged, this, &QualitySwitcher::OnParticlesTagsCloudChanged);
    }

    // quality options
    {
        QGridLayout* optionsLayout = new QGridLayout(optionsGroup);
        optionsLayout->setColumnMinimumWidth(0, minColumnW);
        optionsLayout->setColumnMinimumWidth(1, minColumnW);

        optionsGroup->setTitle("Options");
        optionsGroup->setLayout(optionsLayout);

        DAVA::int32 optionsCount = DAVA::QualitySettingsSystem::Instance()->GetOptionsCount();
        for (DAVA::int32 i = 0; i < optionsCount; ++i)
        {
            DAVA::FastName optionName = DAVA::QualitySettingsSystem::Instance()->GetOptionName(i);

            QLabel* labOp = new QLabel(QString(optionName.c_str()) + ":", materialsGroup);
            QCheckBox* checkOp = new QCheckBox(materialsGroup);
            checkOp->setObjectName(QString(optionName.c_str()) + "CheckBox");
            checkOp->setChecked(DAVA::QualitySettingsSystem::Instance()->IsOptionEnabled(optionName));
            checkOp->setProperty("qualityOptionName", QVariant(optionName.c_str()));

            QObject::connect(checkOp, SIGNAL(clicked(bool)), this, SLOT(OnOptionClick(bool)));

            optionsLayout->addWidget(labOp, i, 0);
            optionsLayout->addWidget(checkOp, i, 1);
        }
    }

    // buttons
    {
        QHBoxLayout* buttonsLayout = new QHBoxLayout(buttonsGroup);
        buttonsGroup->setLayout(buttonsLayout);

        QPushButton* buttonOk = new QPushButton("OK", particlesGroup);

        QPushButton* buttonCancel = new QPushButton("Cancel", particlesGroup);

        QPushButton* buttonApply = new QPushButton("Apply", particlesGroup);
        buttonApply->setObjectName("ApplyButton");

        connect(buttonOk, &QPushButton::released, this, &QualitySwitcher::OnOkPressed);
        connect(buttonCancel, &QPushButton::released, this, &QualitySwitcher::OnCancelPressed);
        connect(buttonApply, &QPushButton::released, this, &QualitySwitcher::OnApplyPressed);

        buttonsLayout->addStretch();
        buttonsLayout->addWidget(buttonOk);
        buttonsLayout->addWidget(buttonCancel);
        buttonsLayout->addWidget(buttonApply);
        buttonsLayout->setMargin(5);
        buttonsLayout->setSpacing(spacing);
    }

    mainLayout->addWidget(texturesGroup);
    mainLayout->addWidget(materialsGroup);
    mainLayout->addWidget(particlesGroup);
    mainLayout->addWidget(optionsGroup);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonsGroup);
    mainLayout->setMargin(5);
    mainLayout->setSpacing(spacing);

    setLayout(mainLayout);

    SetSettingsDirty(false);

    adjustSize();
}

QualitySwitcher::~QualitySwitcher()
{
    switcherDialog = nullptr;
}

void QualitySwitcher::ApplyTx()
{
    DAVA::Deprecated::GetInvoker()->Invoke(DAVA::ReloadAllTextures.ID, DAVA::Deprecated::GetDataNode<DAVA::CommonInternalSettings>()->textureViewGPU);
}

void QualitySwitcher::ApplyMa()
{
    DAVA::Deprecated::GetAccessor()->ForEachContext([](const DAVA::DataContext& ctx) {
        DAVA::SceneData* data = ctx.GetData<DAVA::SceneData>();
        const DAVA::Set<DAVA::NMaterial*>& topParents = data->GetScene()->GetSystem<DAVA::EditorMaterialSystem>()->GetTopParents();

        for (auto material : topParents)
        {
            material->InvalidateRenderVariants();
        }

        data->GetScene()->renderSystem->SetForceUpdateLights();
    });
}

void QualitySwitcher::UpdateEntitiesToQuality(DAVA::Entity* e)
{
    DAVA::QualitySettingsSystem::Instance()->UpdateEntityVisibility(e);
    for (DAVA::int32 i = 0, sz = e->GetChildrenCount(); i < sz; ++i)
    {
        UpdateEntitiesToQuality(e->GetChild(i));
    }
}

void QualitySwitcher::UpdateParticlesToQuality()
{
    SceneSignals* sceneSignals = SceneSignals::Instance();
    DAVA::Deprecated::GetAccessor()->ForEachContext([sceneSignals, this](const DAVA::DataContext& ctx) {
        DAVA::SceneEditor2* scene = ctx.GetData<DAVA::SceneData>()->GetScene().Get();
        scene->BeginBatch("Switch particle quality");
        ReloadEntityEmitters(scene, scene);
        sceneSignals->EmitStructureChanged(scene, nullptr);
        scene->EndBatch();
    });
}

void QualitySwitcher::ReloadEntityEmitters(DAVA::SceneEditor2* scene, DAVA::Entity* e)
{
    DAVA::ParticleEffectComponent* comp = GetEffectComponent(e);
    if (comp)
    {
        scene->Exec(std::make_unique<DAVA::CommandReloadEmitters>(comp));
    }

    for (DAVA::int32 i = 0, sz = e->GetChildrenCount(); i < sz; ++i)
    {
        ReloadEntityEmitters(scene, e->GetChild(i));
    }
}

void QualitySwitcher::SetSettingsDirty(bool dirty)
{
    settingsDirty = dirty;
    QPushButton* applyButton = findChild<QPushButton*>("ApplyButton");
    applyButton->setEnabled(settingsDirty);
}

void QualitySwitcher::ApplySettings()
{
    bool someQualityChanged = false;
    // textures
    {
        QComboBox* combo = findChild<QComboBox*>("TexturesCombo");
        if (nullptr != combo)
        {
            DAVA::FastName newTxQuality(combo->currentText().toLatin1());
            if (newTxQuality != DAVA::QualitySettingsSystem::Instance()->GetCurTextureQuality())
            {
                DAVA::QualitySettingsSystem::Instance()->SetCurTextureQuality(newTxQuality);
                ApplyTx();
            }
        }
    }

    // materials
    bool materialSettingsChanged = false;
    bool optionSettingsChanged = false;
    {
        QComboBox* combo = findChild<QComboBox*>("AnisotropyCombo");
        if (nullptr != combo)
        {
            DAVA::FastName newAnQuality(combo->currentText().toLatin1());
            if (newAnQuality != DAVA::QualitySettingsSystem::Instance()->GetCurAnisotropyQuality())
            {
                materialSettingsChanged = true;
                DAVA::QualitySettingsSystem::Instance()->SetCurAnisotropyQuality(newAnQuality);
            }
        }

        combo = findChild<QComboBox*>("MSAACombo");
        if (nullptr != combo)
        {
            DAVA::FastName newQuality(combo->currentText().toLatin1());
            if (newQuality != DAVA::QualitySettingsSystem::Instance()->GetCurMSAAQuality())
            {
                materialSettingsChanged = true;
                DAVA::QualitySettingsSystem::Instance()->SetCurMSAAQuality(newQuality);
            }
        }

        for (size_t i = 0; i < DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityGroupCount(); ++i)
        {
            DAVA::FastName groupName = DAVA::QualitySettingsSystem::Instance()->GetMaterialQualityGroupName(i);
            combo = findChild<QComboBox*>(QString(groupName.c_str()) + "Combo");
            if (nullptr != combo)
            {
                DAVA::FastName newMaQuality(combo->currentText().toLatin1());
                DAVA::FastName group(combo->currentData().toString().toLatin1());
                if (newMaQuality != DAVA::QualitySettingsSystem::Instance()->GetCurMaterialQuality(group))
                {
                    materialSettingsChanged = true;
                    DAVA::QualitySettingsSystem::Instance()->SetCurMaterialQuality(group, newMaQuality);
                }
            }
        }
    }

    //particles
    {
        DAVA::ParticlesQualitySettings& particlesSettings = DAVA::QualitySettingsSystem::Instance()->GetParticlesQualitySettings();
        if (particlesSettings.GetQualitiesCount() > 0)
        {
            bool settingsChanged = false;
            QComboBox* combo = findChild<QComboBox*>("ParticlesQualityCombo");
            if (nullptr != combo)
            {
                DAVA::FastName newParticlesQuality(combo->currentText().toLatin1());
                if (particlesSettings.GetCurrentQuality() != newParticlesQuality)
                {
                    particlesSettings.SetCurrentQuality(newParticlesQuality);
                    settingsChanged = true;
                }
            }

            QLineEdit* edit = findChild<QLineEdit*>("ParticlesTagsCloudEdit");
            if (nullptr != edit)
            {
                DAVA::Vector<DAVA::String> tags;
                DAVA::Split(edit->text().toStdString(), " ", tags);

                DAVA::Set<DAVA::FastName> newTagsCloud;
                for (const DAVA::String& tag : tags)
                {
                    newTagsCloud.insert(DAVA::FastName(tag));
                }

                if (particlesSettings.GetTagsCloud() != newTagsCloud)
                {
                    particlesSettings.SetTagsCloud(newTagsCloud);
                    settingsChanged = true;
                }
            }

            if (settingsChanged)
            {
                UpdateParticlesToQuality();
                someQualityChanged = true;
            }
        }
    }

    // options
    {
        DAVA::int32 optionsCount = DAVA::QualitySettingsSystem::Instance()->GetOptionsCount();
        for (DAVA::int32 i = 0; i < optionsCount; ++i)
        {
            DAVA::FastName optionName = DAVA::QualitySettingsSystem::Instance()->GetOptionName(i);
            QCheckBox* checkBox = findChild<QCheckBox*>(QString(optionName.c_str()) + "CheckBox");
            if (nullptr != checkBox)
            {
                DAVA::FastName optionName(checkBox->property("qualityOptionName").toString().toStdString().c_str());
                bool checked = checkBox->isChecked();
                if (DAVA::QualitySettingsSystem::Instance()->IsOptionEnabled(optionName) != checked)
                {
                    DAVA::QualitySettingsSystem::Instance()->EnableOption(optionName, checked);
                    optionSettingsChanged = true;
                }
            }
        }
    }

    if (materialSettingsChanged)
    {
        ApplyMa();
        someQualityChanged = true;
    }

    if (materialSettingsChanged || optionSettingsChanged)
    {
        DAVA::Deprecated::GetAccessor()->ForEachContext([&](const DAVA::DataContext& ctx) {
            DAVA::SceneEditor2* scene = ctx.GetData<DAVA::SceneData>()->GetScene().Get();
            UpdateEntitiesToQuality(scene);
            scene->foliageSystem->SyncFoliageWithLandscape();
        });
    }

    if (someQualityChanged)
    {
        SceneSignals::Instance()->EmitQualityChanged();
    }
}

void QualitySwitcher::ShowDialog()
{
    if (switcherDialog == nullptr)
    {
        //we don't need synchronization because of working in UI thread
        switcherDialog = new QualitySwitcher();
        switcherDialog->setAttribute(Qt::WA_DeleteOnClose, true);

        switcherDialog->show();
    }

    switcherDialog->raise();
    switcherDialog->activateWindow();
}

void QualitySwitcher::OnSetSettingsDirty(int index)
{
    SetSettingsDirty(true);
}

void QualitySwitcher::OnParticlesTagsCloudChanged(const QString& text)
{
    SetSettingsDirty(true);
}

void QualitySwitcher::OnOptionClick(bool checked)
{
    SetSettingsDirty(true);
}

void QualitySwitcher::OnOkPressed()
{
    if (settingsDirty)
    {
        ApplySettings();
    }

    accept();
}

void QualitySwitcher::OnCancelPressed()
{
    reject();
}

void QualitySwitcher::OnApplyPressed()
{
    SetSettingsDirty(false);

    ApplySettings();
}
