#include "UI/DataBinding/Private/UIDataChildFactory.h"

#include "UI/DataBinding/UIDataChildFactoryComponent.h"
#include "UI/DataBinding/Private/UIDataModel.h"

#include "UI/Formula/Private/FormulaExpression.h"
#include "UI/Formula/Private/FormulaParser.h"
#include "UI/Formula/Private/FormulaExecutor.h"

#include "UI/UIControl.h"
#include "UI/Components/UIControlSourceComponent.h"
#include "UI/Layouts/UILayoutSourceRectComponent.h"
#include "UI/UIPackageLoader.h"
#include "UI/DefaultUIPackageBuilder.h"

namespace DAVA
{
UIDataChildFactory::UIDataChildFactory(UIDataChildFactoryComponent* component_, bool editorMode)
    : UIDataModel(component_, PRIORITY_FACTORY, editorMode)
    , component(component_)
{
}

UIDataChildFactory::~UIDataChildFactory()
{
    if (control.Valid())
    {
        control->RemoveFromParent();
        control = nullptr;
    }
}

UIDataChildFactoryComponent* UIDataChildFactory::GetComponent() const
{
    return component;
}

void UIDataChildFactory::MarkAsUnprocessed()
{
    processed = false;
}

bool UIDataChildFactory::Process(UIDataBindingDependenciesManager* dependenciesManager)
{
    if (!UIDataModel::Process(dependenciesManager))
    {
        return false;
    }
    bool processed = false;

    bool hasToResetError = false;
    bool expChanged = false;

    context = GetParent()->GetFormulaContext();

    if (component->IsDirty())
    {
        component->SetDirty(false);
        packageExpression.reset();
        controlExpression.reset();
        hasToResetError = true;
        expChanged = true;

        if (!component->GetPackageExpression().empty() && !component->GetControlExpression().empty())
        {
            try
            {
                FormulaParser packageParser(component->GetPackageExpression());
                packageExpression = packageParser.ParseExpression();

                FormulaParser controlParser(component->GetControlExpression());
                controlExpression = controlParser.ParseExpression();
            }
            catch (const FormulaException& error)
            {
                packageExpression.reset();
                controlExpression.reset();
                hasToResetError = false;
                NotifyError(error.GetFormattedMessage(), "UIDataChildFactoryComponent/package");
            }
        }
        processed = true;
    }

    std::shared_ptr<FormulaContext> parentContext = parent->GetFormulaContext();

    if (expChanged || parent->IsDirty() || dependenciesManager->IsDirty(dependencyId))
    {
        if (packageExpression && controlExpression)
        {
            Any anyPackageName;
            Any anyControlName;
            FormulaExecutor executor(parentContext);
            try
            {
                anyPackageName = executor.Calculate(packageExpression.get());
                anyControlName = executor.Calculate(controlExpression.get());
            }
            catch (const FormulaException& error)
            {
                hasToResetError = false;
                NotifyError(error.GetFormattedMessage(), "UIDataListComponent/dataContainer");
            }

            if (!anyPackageName.IsEmpty() && !anyControlName.IsEmpty())
            {
                const Vector<void*>& dependencies = executor.GetDependencies();

                if (!dependencies.empty())
                {
                    dependencyId = dependenciesManager->MakeDependency(dependencyId, dependencies);
                }
                else if (dependencyId != UIDataBindingDependenciesManager::UNKNOWN_DEPENDENCY)
                {
                    dependenciesManager->ReleaseDepencency(dependencyId);
                }

                if (!anyPackageName.CanCast<String>())
                {
                    hasToResetError = false;
                    NotifyError(Format("Result of package expression '%s' must be String", component->GetPackageExpression().c_str()), "UIDataListComponent/package");
                }
                else if (!anyControlName.CanCast<String>())
                {
                    hasToResetError = false;
                    NotifyError(Format("Result of control expression '%s' must be String", component->GetControlExpression().c_str()), "UIDataListComponent/control");
                }
                else
                {
                    String newPackageName = anyPackageName.Cast<String>();
                    String newControlName = anyControlName.Cast<String>();

                    if (packageName != newPackageName || controlName != newControlName)
                    {
                        packageName = newPackageName;
                        controlName = newControlName;

                        if (control.Valid())
                        {
                            control->RemoveFromParent();
                            control = nullptr;
                        }

                        bool recursiveControl = false;
                        {
                            UIControl* p = component->GetControl();
                            while (p != nullptr)
                            {
                                UIControlSourceComponent* objComp = p->GetComponent<UIControlSourceComponent>();
                                if (objComp && objComp->GetPackagePath() == packageName && objComp->GetControlName() == controlName)
                                {
                                    recursiveControl = true;
                                    break;
                                }
                                p = p->GetParent();
                            }
                        }

                        if (!recursiveControl)
                        {
                            DefaultUIPackageBuilder pkgBuilder;
                            pkgBuilder.SetEditorMode(editorMode);
                            UIPackageLoader().LoadPackage(packageName, &pkgBuilder);

                            if (pkgBuilder.GetPackage() == nullptr)
                            {
                                hasToResetError = false;
                                NotifyError(Format("Can't load package '%s'",
                                                   component->GetPackageExpression().c_str()),
                                            "UIDataListComponent/package");
                            }
                            else
                            {
                                control = pkgBuilder.GetPackage()->ExtractControl(controlName);
                                if (control.Valid())
                                {
                                    UIControlSourceComponent* objComp = control->GetOrCreateComponent<UIControlSourceComponent>();
                                    objComp->SetPackagePath(packageName);
                                    objComp->SetControlName(controlName);

                                    if (editorMode)
                                    {
                                        UILayoutSourceRectComponent* sourceRect = control->GetOrCreateComponent<UILayoutSourceRectComponent>();
                                        sourceRect->SetSize(control->GetSize());
                                        sourceRect->SetPosition(control->GetPosition());
                                    }

                                    component->GetControl()->AddControl(control.Get());
                                }
                                else
                                {
                                    hasToResetError = false;
                                    NotifyError(Format("Can't find control in package '%s'",
                                                       component->GetControlExpression().c_str(),
                                                       component->GetPackageExpression().c_str()),
                                                "UIDataListComponent/control");
                                }
                            }
                        }
                        else
                        {
                            hasToResetError = false;
                            NotifyError(Format("Trying to load control '%s' from package '%s' recursively",
                                               component->GetControlExpression().c_str(),
                                               component->GetPackageExpression().c_str()),
                                        "UIDataListComponent/control");
                        }
                    }
                }
            }
        }
        processed = true;
    }

    if (hasToResetError)
    {
        ResetError();
    }

    return processed;
}
}
