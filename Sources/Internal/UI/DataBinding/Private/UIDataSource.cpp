#include "UI/DataBinding/Private/UIDataSource.h"

#include "FileSystem/File.h"

#include "UI/Formula/FormulaContext.h"
#include "UI/Formula/Private/FormulaException.h"
#include "UI/Formula/Private/FormulaParser.h"
#include "UI/Formula/Private/FormulaExecutor.h"
#include "UI/UIControl.h"

#include "Reflection/ReflectedTypeDB.h"

namespace DAVA
{
UIDataSource::UIDataSource(UIDataSourceComponent* component_, int32 componentIndex, bool editorMode)
    : UIDataModel(component_, PRIORITY_SOURCE + componentIndex, editorMode)
    , component(component_)
{
}

UIDataSource::~UIDataSource()
{
}

UIComponent* UIDataSource::GetComponent() const
{
    return component;
}

bool UIDataSource::Process(UIDataBindingDependenciesManager* dependenciesManager)
{
    if (!UIDataModel::Process(dependenciesManager))
    {
        return false;
    }

    if (component->IsDirty() ||
        GetParent()->IsDirty() ||
        (dependencyId != UIDataBindingDependenciesManager::UNKNOWN_DEPENDENCY && dependenciesManager->IsDirty(dependencyId)))
    {
        UIDataErrorGuard errorGuard(this);

        if (component->IsDirty())
        {
            sourceData.reset();
            expression.reset();
        }
        dirty = true;

        if (component->GetSourceType() == UIDataSourceComponent::FROM_REFLECTION &&
            component->GetData().IsValid())
        {
            ReadDataFromReflection(dependenciesManager, &errorGuard);
        }
        else if (component->GetSourceType() == UIDataSourceComponent::FROM_REFLECTION ||
                 component->GetSourceType() == UIDataSourceComponent::FROM_FILE)
        {
            ReadDataFromFile(dependenciesManager, &errorGuard);
        }
        else
        {
            DVASSERT(component->GetSourceType() == UIDataSourceComponent::FROM_EXPRESSION);
            ReadDataFromExpression(dependenciesManager, &errorGuard);
        }
        component->SetDirty(false);

        return true;
    }

    return false;
}

void UIDataSource::ReadDataFromReflection(UIDataBindingDependenciesManager* dependenciesManager, UIDataErrorGuard* errorGuard)
{
    DVASSERT(component->GetData().IsValid());

    context = std::make_shared<FormulaReflectionContext>(component->GetData(), GetParent()->GetFormulaContext());
    Vector<void*> dependencies;
    dependencies.push_back(component->GetData().GetValueObject().GetVoidPtr());
    dependencyId = dependenciesManager->MakeDependency(dependencyId, dependencies);
}

void UIDataSource::ReadDataFromFile(UIDataBindingDependenciesManager* dependenciesManager, UIDataErrorGuard* errorGuard)
{
    ReleaseDependencies(dependenciesManager);
    sourceData.reset();

    if (!component->GetSource().empty())
    {
        try
        {
            sourceData = LoadDataMap(component->GetSource());
            context = std::make_shared<FormulaReflectionContext>(Reflection::Create(ReflectedObject(sourceData.get())), GetParent()->GetFormulaContext());
        }
        catch (const FormulaException& error)
        {
            errorGuard->NotifyError(error.GetFormattedMessage(), "UIDataSourceComponent/source");
            context = GetParent()->GetFormulaContext();
        }
    }
    else
    {
        context = GetParent()->GetFormulaContext();
    }
}

void UIDataSource::ReadDataFromExpression(UIDataBindingDependenciesManager* dependenciesManager, UIDataErrorGuard* errorGuard)
{
    bool expChanged = false;

    if (component->IsDirty())
    {
        expChanged = true;
        FormulaParser parser(component->GetSource());

        ReleaseDependencies(dependenciesManager);
        expression.reset();

        try
        {
            expression = parser.ParseExpression();
        }
        catch (const FormulaException& error)
        {
            errorGuard->NotifyError(error.GetFormattedMessage(), "UIDataScopeComponent/source");
        }
    }

    if (expression.get())
    {
        try
        {
            FormulaExecutor executor(GetParent()->GetFormulaContext());

            Reflection ref = executor.GetDataReference(expression.get());
            if (ref.IsValid())
            {
                context = std::make_shared<FormulaReflectionContext>(ref, GetParent()->GetFormulaContext());
                const Vector<void*>& dependencies = executor.GetDependencies();
                if (!dependencies.empty())
                {
                    dependencyId = dependenciesManager->MakeDependency(dependencyId, dependencies);
                }
                else
                {
                    ReleaseDependencies(dependenciesManager);
                }
            }
            else
            {
                DAVA_THROW(FormulaException, Format("Can't get data %s", component->GetSource().c_str()), expression->GetLineNumber(), expression->GetPositionInLine());
            }
        }
        catch (const FormulaException& error)
        {
            context = GetParent()->GetFormulaContext();
            errorGuard->NotifyError(error.GetFormattedMessage(), "UIDataScopeComponent/source");
        }
    }
    else
    {
        context = GetParent()->GetFormulaContext();
    }
}

void UIDataSource::ReleaseDependencies(UIDataBindingDependenciesManager* dependenciesManager)
{
    if (dependencyId != UIDataBindingDependenciesManager::UNKNOWN_DEPENDENCY)
    {
        dependenciesManager->ReleaseDepencency(dependencyId);
        dependencyId = UIDataBindingDependenciesManager::UNKNOWN_DEPENDENCY;
    }
}

std::shared_ptr<FormulaDataMap> UIDataSource::LoadDataMap(const FilePath& path)
{
    RefPtr<File> file(File::Create(path, File::OPEN | File::READ));
    if (file.Valid())
    {
        String str;
        uint32 fileSize = static_cast<int32>(file->GetSize());
        str.resize(fileSize);
        uint32 readSize = file->Read(&str.front(), static_cast<int32>(fileSize));
        if (readSize == fileSize)
        {
            FormulaParser parser(str);
            return parser.ParseMap();
        }
        else
        {
            DAVA_THROW(FormulaException, Format("Can't read file: %s", path.GetAbsolutePathname().c_str()), 0, 0);
        }
    }
    DAVA_THROW(FormulaException, Format("Can't open file: %s", path.GetAbsolutePathname().c_str()), 0, 0);
}
}
