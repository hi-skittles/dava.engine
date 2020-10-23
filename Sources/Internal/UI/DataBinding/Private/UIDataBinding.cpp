#include "UI/DataBinding/Private/UIDataBinding.h"

#include "Reflection/ReflectedTypeDB.h"

#include "UI/DataBinding/UIDataBindingIssueDelegate.h"
#include "UI/DataBinding/UIDataBindingComponent.h"

#include "UI/DataBinding/Private/UIDataBindingDependenciesManager.h"
#include "UI/DataBinding/Private/UIDataModel.h"

#include "UI/Formula/Private/FormulaExpression.h"
#include "UI/Formula/Private/FormulaParser.h"
#include "UI/Formula/Private/FormulaExecutor.h"
#include "UI/Formula/Private/FormulaFormatter.h"

#include "UI/Styles/UIStyleSheetPropertyDataBase.h"

#include "UI/UIControl.h"
#include "Utils/Utils.h"

namespace DAVA
{
UIDataBinding::UIDataBinding(UIDataBindingComponent* component_, bool editorMode)
    : UIDataNode(editorMode)
    , component(component_)
{
}

UIDataBinding::~UIDataBinding()
{
}

UIComponent* UIDataBinding::GetComponent() const
{
    return component;
}

void UIDataBinding::ProcessReadFromModel(UIDataBindingDependenciesManager* dependenciesManager)
{
    bool hasToResetError = false;
    bool expChanged = false;

    if (component->IsDirty())
    {
        component->SetDirty(false);
        expression = nullptr;
        hasToResetError = true;
        expChanged = true;

        BaseObject* obj = component->GetControl();
        const Type* componentType = nullptr;

        String key = component->GetControlFieldName();

        Vector<String> pathParts;
        Split(component->GetControlFieldName(), ".", pathParts);
        if (pathParts.size() > 1)
        {
            DVASSERT(pathParts.size() == 2);

            const Vector<UIComponent*>& components = component->GetControl()->GetComponents();
            for (UIComponent* c : components)
            {
                if (ReflectedTypeDB::GetByType(c->GetType())->GetPermanentName() == pathParts[0])
                {
                    obj = c;
                    key = pathParts[1];
                    componentType = c->GetType();
                    break;
                }
            }
        }
        int32 propertyIndex = UIStyleSheetPropertyDataBase::Instance()->FindStyleSheetProperty(componentType, FastName(key));
        if (propertyIndex >= 0)
        {
            component->GetControl()->SetPropertyLocalFlag(propertyIndex, true);
        }

        controlReflection = Reflection::Create(ReflectedObject(obj)).GetField(key);
        DVASSERT(controlReflection.IsValid(), Format("Can't find control field: %s", component->GetControlFieldName().c_str()).c_str());

        FormulaParser parser(component->GetBindingExpression());
        try
        {
            expression = parser.ParseExpression();
        }
        catch (const FormulaException& error)
        {
            expression = nullptr;
            hasToResetError = false;
            NotifyError(error.GetFormattedMessage(), component->GetControlFieldName());
        }
    }

    if (expression.get() && component->GetUpdateMode() != UIDataBindingComponent::MODE_WRITE && (parent->IsDirty() || expChanged || dependenciesManager->IsDirty(dependencyId)))
    {
        std::shared_ptr<FormulaContext> context = parent->GetFormulaContext();
        hasToResetError = true;
        try
        {
            FormulaExecutor executor(context);
            Any val = executor.Calculate(expression.get());
            const Vector<void*>& dependencies = executor.GetDependencies();

            if (!dependencies.empty())
            {
                dependencyId = dependenciesManager->MakeDependency(dependencyId, dependencies);
            }
            else if (dependencyId != UIDataBindingDependenciesManager::UNKNOWN_DEPENDENCY)
            {
                dependenciesManager->ReleaseDepencency(dependencyId);
            }

            if (controlReflection.IsValid())
            {
                if (controlReflection.GetValueType() == val.GetType())
                {
                    controlReflection.SetValue(val);
                }
                else if (controlReflection.GetValueType() == Type::Instance<String>())
                {
                    controlReflection.SetValue(FormulaFormatter::AnyToString(val));
                }
                else if (controlReflection.GetValueType() == Type::Instance<FilePath>())
                {
                    controlReflection.SetValue(FilePath(FormulaFormatter::AnyToString(val)));
                }
                else
                {
                    if (!val.IsEmpty())
                    {
                        controlReflection.SetValueWithCast(val);
                    }
                    else
                    {
                        DVASSERT(false);
                    }
                }
            }
            else
            {
                DVASSERT(false);
            }
        }
        catch (const FormulaException& error)
        {
            hasToResetError = false;
            NotifyError(error.GetFormattedMessage(), component->GetControlFieldName());
        }
        catch (const Exception& exception)
        {
            hasToResetError = false;
            NotifyError(exception.what(), component->GetControlFieldName());
        }
    }
    if (hasToResetError)
    {
        ResetError();
    }
}

bool UIDataBinding::ProcessWriteToModel(UIDataBindingDependenciesManager* dependenciesManager)
{
    bool result = false;
    if (expression.get() && component->GetUpdateMode() != UIDataBindingComponent::MODE_READ && !dependenciesManager->IsDirty(dependencyId))
    {
        std::shared_ptr<FormulaContext> context = parent->GetFormulaContext();
        Any uiValue = controlReflection.GetValue();
        bool hasToResetError = true;
        try
        {
            Reflection ref = FormulaExecutor(context).GetDataReference(expression.get());
            if (ref.GetValue() != uiValue)
            {
                ref.SetValue(uiValue);
                dependenciesManager->SetDirty(ref.GetValueObject().GetVoidPtr());
                result = true;
            }
        }
        catch (const FormulaException& error)
        {
            hasToResetError = false;
            NotifyError(error.GetErrorMessage(), component->GetControlFieldName());
        }

        if (hasToResetError)
        {
            ResetError();
        }
    }

    return result;
}
}
