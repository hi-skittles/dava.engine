#include "CompletionsProviderForUIReflection.h"

#include "Model/ControlProperties/AbstractProperty.h"
#include "Model/ControlProperties/RootProperty.h"
#include "Model/PackageHierarchy/ControlNode.h"

#include <Reflection/Reflection.h>
#include <Reflection/ReflectedTypeDB.h>
#include <UI/UIControl.h>
#include <UI/Components/UIComponent.h>

using namespace DAVA;

CompletionsProviderForUIReflection::CompletionsProviderForUIReflection(const String& propertyName, const String& componentName)
    : propertyName(propertyName)
{
    if (!componentName.empty())
    {
        const ReflectedType* reflectedType = ReflectedTypeDB::GetByPermanentName(componentName);
        if (reflectedType != nullptr)
        {
            componentType = reflectedType->GetType();
        }
    }
}

CompletionsProviderForUIReflection::~CompletionsProviderForUIReflection()
{
}

QStringList CompletionsProviderForUIReflection::GetCompletions(AbstractProperty* property)
{
    QStringList list;

    RootProperty* root = dynamic_cast<RootProperty*>(property->GetRootProperty());
    if (root)
    {
        PackageBaseNode* currentNode = root->GetControlNode();
        UIControl* control = currentNode->GetControl();

        Reflection ref;
        if (componentType)
        {
            ref = Reflection::Create(ReflectedObject(control->GetComponent(componentType)));
        }
        else
        {
            ref = Reflection::Create(ReflectedObject(control));
        }

        if (ref.IsValid())
        {
            ref = ref.GetField(propertyName);
            if (ref.IsValid())
            {
                for (auto r : ref.GetFields())
                {
                    list << QString::fromStdString(r.ref.GetValue().Cast<String>());
                }
            }
        }
    }

    return list;
}
