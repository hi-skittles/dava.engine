#ifndef __PARTICLE_EFFECT_PROPERTIES_WIDGET__H__
#define __PARTICLE_EFFECT_PROPERTIES_WIDGET__H__

#include <QWidget>
#include "BaseParticleEditorContentWidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QPushButton>
#include <QSpinBox>
#include <QTreeWidget>
#include <QTableWidget>
#include <QStyledItemDelegate>

#include "DockParticleEditor/TimeLineWidget.h"
#include "DockParticleEditor/GradientPickerWidget.h"

struct EffectTreeData
{
    DAVA::ParticleEmitter* emmiter;
    DAVA::ParticleLayer* layer;
    DAVA::ParticleForceSimplified* force;
    DAVA::int32 externalParamId;
};

Q_DECLARE_METATYPE(EffectTreeData);

static const DAVA::String EXTERNAL_NAMES[] =
{
  "Emission Vector", //emmiter
  "Emission Range",
  "Radius",
  "Size",
  "Color Over Life",
  "Life", //layer
  "Life Variation",
  "Number",
  "Number Variation",
  "Size",
  "Size Variation",
  "Size Over Life",
  "Velocity",
  "Velocity Variation",
  "Velocity Over Life",
  "Spin",
  "Spin Variation",
  "Spin Over Life",
  "Color Random",
  "Alpha Over Life",
  "Color Over Life",
  "Angle",
  "Angle Variation",
  "Anim Speed Over Life",
  "Force", //force
  "Force Variation",
  "Force Over Life"
};

class EditModificationLineDialog : public QDialog
{
    Q_OBJECT
public:
    explicit EditModificationLineDialog(QWidget* parent)
        : QDialog(parent)
    {
        setMinimumWidth(400);
    }
    template <class T>
    void Init(DAVA::ModifiablePropertyLine<T>* line, bool onAdd);
    template <class T>
    void UpdateLine(DAVA::ModifiablePropertyLine<T>* line); //note! - name would be updated explicitly as it may require re-register in effect
    DAVA::String GetVariableName();

private:
    void InitName(const DAVA::String& name, bool onAdd);
    void InitButtons();

    QVBoxLayout* dialogLayout;
    QLineEdit* variableName;
    TimeLineWidget* timeLine;
    GradientPickerWidget* gradientLine;
};

class VariableEditDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    VariableEditDelegate(QObject* parent, QTableWidget* table)
        : QStyledItemDelegate(parent)
        , editTable(table)
    {
    }

    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    void setEditorData(QWidget* editor, const QModelIndex& index) const;
    void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

protected:
    QTableWidget* editTable;
};

class ParticleEffectPropertiesWidget : public BaseParticleEditorContentWidget
{
    Q_OBJECT

public:
    enum EmitterExternals
    {
        EE_EMISSION_VECTOR = 0,
        EE_EMISSION_RANGE,
        EE_RADUS,
        EE_SIZE,
        EE_COLOR_OVER_LIFE,
        EE_TOTAL
    };

    enum LayerExternals
    {
        EL_LIFE = EE_TOTAL,
        EL_LIFE_VARIATION,
        EL_NUMBER,
        EL_NUMBER_VARIATION,
        EL_SIZE,
        EL_SIZE_VARIATION,
        EL_SIZE_OVERLIFE,
        EL_VELOCITY,
        EL_VELOCITY_VARIATON,
        EL_VELOCITY_OVERLIFE,
        EL_SPIN,
        EL_SPIN_VARIATION,
        EL_SPIN_OVERLIFE,
        EL_COLOR,
        EL_ALPHA_OVERLIFE,
        EL_COLOR_OVERLIFE,
        EL_ANGLE,
        EL_ANGLE_VARIATION,
        EL_ANIM_SPEED_OVERLIFE,
        EL_TOTAL
    };
    enum ForceExternals
    {
        EF_FORCE = EL_TOTAL,
        EF_FORCE_OVERLIFE,
        EF_TOTAL
    };

    explicit ParticleEffectPropertiesWidget(QWidget* parent = nullptr);

    void Init(SceneEditor2* scene, DAVA::ParticleEffectComponent* effect);

    void StoreVisualState(DAVA::KeyedArchive* visualStateProps) override;
    void RestoreVisualState(DAVA::KeyedArchive* visualStateProps) override;

public slots:
    void OnValueChanged();
    void OnPlay();
    void OnStop();
    void OnStopAndDelete();
    void OnPause();
    void OnRestart();
    void OnStepForward();

    void ShowContextMenuForEffectTree(const QPoint& pos);
    void OnContextMenuCommand(QAction* action);
    void OnTreeItemDoubleClck(QTreeWidgetItem* treeItem, int column);

    void OnVariableValueChanged(int row, int col);
    void OnGlobalVariableValueChanged(int row, int col);
    void OnAddGlobalExternal();

protected:
    void InitWidget(QWidget* widget, bool connectWidget = true);
    void BuildEffectTree();
    void UpdatePlaybackSpeedLabel();

    void UpdateVaribleTables();

    DAVA::ModifiablePropertyLineBase* GetEmitterLine(DAVA::ParticleEmitter* emitter, EmitterExternals lineId);
    DAVA::ModifiablePropertyLineBase* GetLayerLine(DAVA::ParticleLayer* layer, LayerExternals lineId);
    DAVA::ModifiablePropertyLineBase* GetForceLine(DAVA::ParticleForceSimplified* force, ForceExternals lineId);

    void SetEmitterLineModifiable(DAVA::ParticleEmitter* emitter, EmitterExternals lineId);
    void SetLayerLineModifiable(DAVA::ParticleLayer* layer, LayerExternals lineId);
    void SetForceLineModifiable(DAVA::ParticleForceSimplified* force, ForceExternals lineId);

    void RemoveEmitterLineModifiable(DAVA::ParticleEmitter* emitter, EmitterExternals lineId);
    void RemoveLayerLineModifiable(DAVA::ParticleLayer* layer, LayerExternals lineId);
    void RemoveForceLineModifiable(DAVA::ParticleForceSimplified* force, ForceExternals lineId);

    bool EditEmitterModifiable(DAVA::ParticleEmitter* emitter, EmitterExternals lineId, bool onAdd = false);
    bool EditLayerModifiable(DAVA::ParticleLayer* layer, LayerExternals lineId, bool onAdd = false);
    bool EditForceModifiable(DAVA::ParticleForceSimplified* force, ForceExternals lineId, bool onAdd = false);

    template <class T>
    bool EditModificationLine(DAVA::RefPtr<DAVA::PropertyLine<T>>& line, bool onAdd)
    {
        DAVA::ParticleEffectComponent* effect = GetEffect(GetActiveScene());
        DAVA::ModifiablePropertyLine<T>* editLine = dynamic_cast<DAVA::ModifiablePropertyLine<T>*>(line.Get());
        EditModificationLineDialog dialog(this);
        dialog.Init(editLine, onAdd);
        if (dialog.exec())
        {
            dialog.UpdateLine(editLine);
            DAVA::String resName = dialog.GetVariableName();
            if (editLine->GetValueName() != resName)
            {
                effect->UnRegisterModifiable(editLine);
                editLine->SetValueName(resName);
                effect->RegisterModifiable(editLine);
                UpdateVaribleTables();
            }

            return true;
        }
        return false;
    }

private:
    QVBoxLayout* mainLayout;

    QLabel* effectPlaybackSpeedLabel;
    QSlider* effectPlaybackSpeed;

    QPushButton* playBtn;
    QPushButton* stopBtn;
    QPushButton* stopAndDeleteBtn;
    QPushButton* pauseBtn;
    QPushButton* restartBtn;
    QPushButton* stepForwardBtn;
    QSpinBox* stepForwardFPSSpin;

    QIcon iconEmitter, iconLayer, iconForce, iconExternal;

    QTreeWidget* effectTree;
    QTreeWidgetItem* currSelectedTreeItem;

    QTableWidget* effectVariables;
    VariableEditDelegate* effectEditDelegate;
    QTableWidget* globalVariables;
    VariableEditDelegate* globalEditDelegate;

    bool blockSignals;
    bool blockTables;
};

class AddGlobalExternalDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddGlobalExternalDialog(QWidget* parent);
    DAVA::String GetVariableName();
    DAVA::float32 GetVariableValue();

private:
    QLineEdit* variableName;
    QDoubleSpinBox* variableValue;
};

#endif /* defined(__PARTICLE_EFFECT_PROPERTIES_WIDGET__H__) */
