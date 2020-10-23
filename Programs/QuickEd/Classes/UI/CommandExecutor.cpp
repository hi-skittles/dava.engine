#include "UI/CommandExecutor.h"

#include "Modules/DocumentsModule/DocumentData.h"
#include "Modules/ProjectModule/ProjectData.h"

#include "QECommands/ChangePropertyValueCommand.h"
#include "QECommands/ChangeBindingCommand.h"
#include "QECommands/InsertControlCommand.h"
#include "QECommands/RemoveControlCommand.h"
#include "QECommands/InsertImportedPackageCommand.h"
#include "QECommands/RemoveImportedPackageCommand.h"
#include "QECommands/AddComponentCommand.h"
#include "QECommands/RemoveComponentCommand.h"
#include "QECommands/AttachComponentPrototypeSectionCommand.h"
#include "QECommands/RemoveStyleCommand.h"
#include "QECommands/InsertStyleCommand.h"
#include "QECommands/RemoveStylePropertyCommand.h"
#include "QECommands/RemoveStyleSelectorCommand.h"
#include "QECommands/AddStylePropertyCommand.h"
#include "QECommands/AddStyleSelectorCommand.h"
#include "QECommands/SetGuidesCommand.h"

#include "QECommands/ChangeStylePropertyCommand.h"

#include "Model/PackageHierarchy/PackageControlsNode.h"
#include "Model/PackageHierarchy/ControlNode.h"
#include "Model/PackageHierarchy/PackageNode.h"
#include "Model/PackageHierarchy/StyleSheetsNode.h"
#include "Model/PackageHierarchy/StyleSheetNode.h"
#include "Model/PackageHierarchy/ImportedPackagesNode.h"

#include "Model/ControlProperties/ComponentPropertiesSection.h"
#include "Model/ControlProperties/NameProperty.h"
#include "Model/ControlProperties/RootProperty.h"
#include "Model/ControlProperties/StyleSheetRootProperty.h"
#include "Model/ControlProperties/StyleSheetProperty.h"
#include "Model/ControlProperties/StyleSheetSelectorProperty.h"

#include "Model/YamlPackageSerializer.h"
#include "Model/QuickEdPackageBuilder.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/WindowSubSystem/UI.h>

#include <FileSystem/YamlParser.h>
#include <UI/Components/UIComponentUtils.h>
#include <UI/UIControl.h>
#include <UI/UIPackageLoader.h>
#include <UI/Styles/UIStyleSheetPropertyDataBase.h>
#include <QtTools/ConsoleWidget/PointerSerializer.h>
#include <Logger/Logger.h>
#include <Utils/StringFormat.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Command/CommandStack.h>

using namespace DAVA;

namespace CommandExecutorDetails
{
template <typename T>
String FormatNodeNames(const DAVA::Vector<T*>& nodes)
{
    const size_t maxControlNames = 3;
    String list;
    for (size_t i = 0; i < nodes.size() && i < maxControlNames; i++)
    {
        if (i != 0)
            list += ", ";
        list += nodes[i]->GetName();
    }

    if (nodes.size() > maxControlNames)
        list += ", etc.";

    return list;
}

bool IsNameExists(const String& name, const ControlsContainerNode* dest, const ControlsContainerNode* siblingContainer)
{
    auto isEqual = [&name](ControlNode* siblingControl)
    {
        return (siblingControl->GetName() == name);
    };

    return std::any_of(dest->begin(), dest->end(), isEqual) || (siblingContainer != nullptr && std::any_of(siblingContainer->begin(), siblingContainer->end(), isEqual));
}

void SplitName(const String& name, String& nameMainPart, uint32& namePostfix)
{
    size_t underlinePos = name.rfind('_');
    if (underlinePos != String::npos && (underlinePos + 1) < name.size())
    {
        char anySymbol; // dummy symbol indicating that there are some symbols after %u in name string
        int res = sscanf(name.data() + underlinePos + 1, "%u%c", &namePostfix, &anySymbol);
        if (res == 1)
        {
            nameMainPart = name.substr(0, underlinePos);
            return;
        }
    }

    nameMainPart = name;
    namePostfix = 0;
}

String ProduceUniqueName(const String& name, const ControlsContainerNode* dest, const ControlsContainerNode* siblingContainer)
{
    String nameMainPart;
    uint32 namePostfixCounter = 0;
    SplitName(name, nameMainPart, namePostfixCounter);

    for (++namePostfixCounter; namePostfixCounter <= UINT32_MAX; ++namePostfixCounter)
    {
        String newName = Format("%s_%u", nameMainPart.c_str(), namePostfixCounter);
        if (!IsNameExists(newName, dest, siblingContainer))
        {
            return newName;
        }
    }

    DAVA::Logger::Warning("Can't produce unique name: reaching uint32 max");
    return name;
}

void EnsureControlNameIsUnique(ControlNode* control, const PackageNode* package, const ControlsContainerNode* dest)
{
    ControlsContainerNode* siblingTopContainer = nullptr;

    if (dest->GetParent() == package)
    {
        ControlsContainerNode* topPrototypes = static_cast<ControlsContainerNode*>(package->GetPrototypes());
        if (dest == topPrototypes) // If we are inserting into top controls container, then prototypes scope will also be taking into account.
        {
            siblingTopContainer = static_cast<ControlsContainerNode*>(package->GetPackageControlsNode());
        }
        else // Vice versa, if we are inserting into top prototypes container, then top controls scope will also be taking into account
        {
            siblingTopContainer = topPrototypes;
        }
    }

    String origName = control->GetName();
    if (IsNameExists(origName, dest, siblingTopContainer))
    {
        String newName = ProduceUniqueName(origName, dest, siblingTopContainer);
        control->GetRootProperty()->GetNameProperty()->SetValue(newName);
    }
}

ControlNode* GetInstanceRoot(ControlNode* instance, ControlNode* prototypeRoot)
{
    for (ControlNode* currentNode = instance;
         currentNode != nullptr;
         currentNode = dynamic_cast<ControlNode*>(currentNode->GetParent()))
    {
        if (currentNode->GetPrototype() == prototypeRoot)
        {
            return currentNode;
        }
    }

    return nullptr;
}
}

CommandExecutor::CommandExecutor(DAVA::ContextAccessor* accessor_, DAVA::UI* ui_)
    : accessor(accessor_)
    , ui(ui_)
{
    DVASSERT(accessor != nullptr);
    DVASSERT(ui != nullptr);
}

void CommandExecutor::AddImportedPackagesIntoPackage(const DAVA::Vector<DAVA::FilePath> packagePaths, const PackageNode* package)
{
    Vector<PackageNode*> importedPackages;
    Result result;
    const EngineContext* engineContext = GetEngineContext();
    for (const FilePath& path : packagePaths)
    {
        if (package->FindImportedPackage(path) == nullptr && package->GetPath().GetFrameworkPath() != path.GetFrameworkPath())
        {
            QuickEdPackageBuilder builder(engineContext);

            ProjectData* projectData = GetProjectData();
            if (UIPackageLoader(projectData->GetPrototypes()).LoadPackage(path, &builder))
            {
                if (!builder.GetResults().HasErrors())
                {
                    RefPtr<PackageNode> importedPackage = builder.BuildPackage();
                    if (package->GetImportedPackagesNode()->CanInsertImportedPackage(importedPackage.Get()))
                    {
                        importedPackages.push_back(SafeRetain(importedPackage.Get()));
                    }
                    else
                    {
                        result = Result(Result::RESULT_ERROR, Format("Package '%s' make cyclic import", path.GetFilename().c_str()));
                        break;
                    }
                }
                else
                {
                    result = Result(Result::RESULT_ERROR, Format("Package '%s' has errors", path.GetFilename().c_str()));
                    break;
                }
            }
            else
            {
                result = Result(Result::RESULT_ERROR, Format("Can't load package '%s'", path.GetFilename().c_str()));
                break;
            }
        }
        else
        {
            result = Result(Result::RESULT_ERROR, "Can't import package into themselves");
            break;
        }
    }

    if (!importedPackages.empty() && result.type == Result::RESULT_SUCCESS)
    {
        DocumentData* documentData = GetDocumentData();
        documentData->BeginBatch("Insert Packages", static_cast<uint32>(importedPackages.size()));
        for (PackageNode* importedPackage : importedPackages)
        {
            AddImportedPackageIntoPackageImpl(importedPackage, package);
            SafeRelease(importedPackage);
        }
        importedPackages.clear();
        documentData->EndBatch();
    }

    if (result.type == Result::RESULT_ERROR)
    {
        NotificationParams params;
        params.title = "Can't import package";
        params.message = result;
        ui->ShowNotification(mainWindowKey, params);
    }
}

void CommandExecutor::RemoveImportedPackagesFromPackage(const DAVA::Vector<PackageNode*>& importedPackages, const PackageNode* package)
{
    DAVA::Vector<PackageNode*> checkedPackages;
    for (PackageNode* testPackage : importedPackages)
    {
        bool canRemove = true;
        for (int i = 0; i < package->GetPackageControlsNode()->GetCount(); i++)
        {
            ControlNode* control = package->GetPackageControlsNode()->Get(i);
            if (control->IsDependsOnPackage(testPackage))
            {
                canRemove = false;
                break;
            }
        }
        if (canRemove)
            checkedPackages.push_back(testPackage);
    }

    if (!checkedPackages.empty())
    {
        DocumentData* documentData = GetDocumentData();
        documentData->BeginBatch("Remove Imported Packages", static_cast<uint32>(checkedPackages.size()));
        for (PackageNode* importedPackage : checkedPackages)
        {
            documentData->ExecCommand<RemoveImportedPackageCommand>(importedPackage);
        }
        documentData->EndBatch();
    }
}

void CommandExecutor::ChangeProperty(ControlNode* node, AbstractProperty* property, const Any& value)
{
    if (!property->IsReadOnly())
    {
        DocumentData* documentData = GetDocumentData();
        documentData->ExecCommand<ChangePropertyValueCommand>(node, property, value);
    }
}

void CommandExecutor::ChangeBindingProperty(ControlNode* node, AbstractProperty* property, const DAVA::String& value, DAVA::int32 mode)
{
    if (!property->IsReadOnly())
    {
        DocumentData* documentData = GetDocumentData();
        documentData->ExecCommand<ChangeBindingCommand>(node, property, value, mode);
    }
}

void CommandExecutor::ResetProperty(ControlNode* node, AbstractProperty* property)
{
    if (!property->IsReadOnly())
    {
        DocumentData* documentData = GetDocumentData();
        documentData->ExecCommand<ChangePropertyValueCommand>(node, property, Any());
    }
}

void CommandExecutor::AddComponent(ControlNode* node, const Type* componentType)
{
    if (node->GetRootProperty()->CanAddComponent(componentType))
    {
        const String& componentName = ReflectedTypeDB::GetByPointer(componentType)->GetPermanentName();
        DocumentData* data = GetDocumentData();
        data->BeginBatch(Format("Add Component %s", componentName.c_str()));
        int32 index = node->GetControl()->GetComponentCount(componentType);
        AddComponentImpl(node, componentType, index, nullptr);
        data->EndBatch();
    }
}

void CommandExecutor::RemoveComponent(ControlNode* node, const Type* componentType, DAVA::uint32 componentIndex)
{
    if (node->GetRootProperty()->CanRemoveComponent(componentType, componentIndex))
    {
        ComponentPropertiesSection* section = node->GetRootProperty()->FindComponentPropertiesSection(componentType, componentIndex);
        if (section)
        {
            const String& componentName = ReflectedTypeDB::GetByPointer(componentType)->GetPermanentName();
            DocumentData* data = GetDocumentData();
            data->BeginBatch(Format("Remove Component %s", componentName.c_str()));
            RemoveComponentImpl(node, section);
            data->EndBatch();
        }
    }
}

void CommandExecutor::ChangeProperty(StyleSheetNode* node, AbstractProperty* property, const DAVA::Any& value)
{
    if (!property->IsReadOnly())
    {
        DocumentData* data = GetDocumentData();
        data->ExecCommand<ChangeStylePropertyCommand>(node, property, value);
    }
}

void CommandExecutor::AddStyleProperty(StyleSheetNode* node, uint32 propertyIndex)
{
    if (node->GetRootProperty()->CanAddProperty(propertyIndex))
    {
        UIStyleSheetProperty prop(propertyIndex, UIStyleSheetPropertyDataBase::Instance()->GetStyleSheetPropertyByIndex(propertyIndex).defaultValue);
        ScopedPtr<StyleSheetProperty> property(new StyleSheetProperty(prop));
        DocumentData* data = GetDocumentData();
        data->ExecCommand<AddStylePropertyCommand>(node, property);
    }
}

void CommandExecutor::RemoveStyleProperty(StyleSheetNode* node, DAVA::uint32 propertyIndex)
{
    if (node->GetRootProperty()->CanRemoveProperty(propertyIndex))
    {
        StyleSheetProperty* property = node->GetRootProperty()->FindPropertyByPropertyIndex(propertyIndex);
        if (property)
        {
            DocumentData* data = GetDocumentData();
            data->ExecCommand<RemoveStylePropertyCommand>(node, property);
        }
    }
}

void CommandExecutor::AddStyleSelector(StyleSheetNode* node)
{
    if (node->GetRootProperty()->CanAddSelector())
    {
        UIStyleSheetSelectorChain chain;
        DocumentData* data = GetDocumentData();
        UIStyleSheetSourceInfo sourceInfo(data->GetPackagePath());

        ScopedPtr<StyleSheetSelectorProperty> property(new StyleSheetSelectorProperty(chain, sourceInfo));
        data->ExecCommand<AddStyleSelectorCommand>(node, property);
    }
}

void CommandExecutor::RemoveStyleSelector(StyleSheetNode* node, DAVA::int32 selectorIndex)
{
    if (node->GetRootProperty()->CanRemoveSelector())
    {
        UIStyleSheetSelectorChain chain;
        StyleSheetSelectorProperty* property = node->GetRootProperty()->GetSelectorAtIndex(selectorIndex);
        DocumentData* data = GetDocumentData();
        data->ExecCommand<RemoveStyleSelectorCommand>(node, property);
    }
}

void CommandExecutor::InsertControl(ControlNode* control, ControlsContainerNode* dest, DAVA::int32 destIndex) const
{
    if (dest->CanInsertControl(control, destIndex))
    {
        DocumentData* data = GetDocumentData();
        CommandExecutorDetails::EnsureControlNameIsUnique(control, data->GetPackageNode(), dest);

        data->BeginBatch(Format("Insert Control %s(%s)", control->GetName().c_str(), control->GetClassName().c_str()));
        InsertControlImpl(control, dest, destIndex);
        data->EndBatch();
    }
    else
    {
        Logger::Warning("%s", String("Can not insert control!" + PointerSerializer::FromPointer(control)).c_str());
    }
}

Vector<ControlNode*> CommandExecutor::InsertInstances(const DAVA::Vector<ControlNode*>& controls, ControlsContainerNode* dest, DAVA::int32 destIndex)
{
    Vector<ControlNode*> nodesToInsert;
    nodesToInsert.reserve(controls.size());
    for (ControlNode* node : controls)
    {
        if (node->CanCopy() && dest->CanInsertControl(node, destIndex))
            nodesToInsert.push_back(node);
    }
    Vector<ControlNode*> insertedNodes;
    insertedNodes.reserve(nodesToInsert.size());
    if (!nodesToInsert.empty())
    {
        DocumentData* data = GetDocumentData();
        data->BeginBatch(Format("Instance Controls %s", CommandExecutorDetails::FormatNodeNames(nodesToInsert).c_str()), static_cast<uint32>(nodesToInsert.size()));

        int index = destIndex;
        for (ControlNode* node : nodesToInsert)
        {
            ControlNode* copy = ControlNode::CreateFromPrototype(node);
            insertedNodes.push_back(copy);
            InsertControlImpl(copy, dest, index);
            SafeRelease(copy);
            index++;
        }

        data->EndBatch();
    }
    return insertedNodes;
}

Vector<ControlNode*> CommandExecutor::CopyControls(const DAVA::Vector<ControlNode*>& nodes, ControlsContainerNode* dest, DAVA::int32 destIndex)
{
    YamlPackageSerializer serializer;
    serializer.SerializePackageNodes(dest->GetPackage(), nodes, {});
    DAVA::String serializedControls = serializer.WriteToString();
    DAVA::Set<PackageBaseNode*> pastedNodes = Paste(dest->GetPackage(), dest, destIndex, serializedControls);

    Vector<ControlNode*> result;
    for (PackageBaseNode* node : pastedNodes)
    {
        ControlNode* control = dynamic_cast<ControlNode*>(node);
        if (control)
        {
            result.push_back(control);
        }
    }
    return result;
}

DAVA::Vector<ControlNode*> CommandExecutor::MoveControls(const DAVA::Vector<ControlNode*>& nodes, ControlsContainerNode* dest, DAVA::int32 destIndex) const
{
    using namespace DAVA;

    Vector<ControlNode*> nodesToMove;
    nodesToMove.reserve(nodes.size());
    for (ControlNode* node : nodes)
    {
        if (node->CanMoveTo(dest, destIndex))
            nodesToMove.push_back(node);
    }
    Vector<ControlNode*> movedNodes;
    movedNodes.reserve(nodesToMove.size());
    if (!nodesToMove.empty())
    {
        DocumentData* data = GetDocumentData();
        data->BeginBatch(Format("Move Controls %s", CommandExecutorDetails::FormatNodeNames(nodes).c_str()), static_cast<uint32>(nodesToMove.size()));
        int index = destIndex;

        Vector<ControlNode*> notMovedNodes;
        for (ControlNode* node : nodesToMove)
        {
            ControlsContainerNode* src = dynamic_cast<ControlsContainerNode*>(node->GetParent());
            if (src)
            {
                int32 srcIndex = src->GetIndex(node);

                if (src == dest && index > srcIndex)
                    index--;

                if (MoveControlImpl(node, dest, index))
                {
                    movedNodes.push_back(node);
                }

                index++;
            }
            else
            {
                notMovedNodes.push_back(node);
            }
        }

        data->EndBatch();

        if (notMovedNodes.empty() == false)
        {
            NotificationParams notificationParams;
            notificationParams.title = "Can not move controls";
            String message = "Can not move controls: " + CommandExecutorDetails::FormatNodeNames(notMovedNodes);

            notificationParams.message = Result(Result::RESULT_WARNING, message);
            ui->ShowNotification(DAVA::mainWindowKey, notificationParams);
        }
    }

    return movedNodes;
}

ResultList CommandExecutor::InsertStyle(StyleSheetNode* styleSheetNode, StyleSheetsNode* dest, DAVA::int32 destIndex)
{
    ResultList resultList;
    if (dest->CanInsertStyle(styleSheetNode, destIndex))
    {
        DocumentData* data = GetDocumentData();
        data->ExecCommand<InsertStyleCommand>(styleSheetNode, dest, destIndex);
    }
    else
    {
        resultList.AddResult(Result::RESULT_ERROR, "Can not instert style sheet!");
    }

    return resultList;
}

void CommandExecutor::CopyStyles(const DAVA::Vector<StyleSheetNode*>& nodes, StyleSheetsNode* dest, DAVA::int32 destIndex)
{
    Vector<StyleSheetNode*> nodesToCopy;
    for (StyleSheetNode* node : nodes)
    {
        if (node->CanCopy() && dest->CanInsertStyle(node, destIndex))
            nodesToCopy.push_back(node);
    }

    if (!nodesToCopy.empty())
    {
        DocumentData* data = GetDocumentData();
        data->BeginBatch(Format("Copy Styles %s", CommandExecutorDetails::FormatNodeNames(nodes).c_str()), static_cast<uint32>(nodesToCopy.size()));

        int index = destIndex;
        for (StyleSheetNode* node : nodesToCopy)
        {
            StyleSheetNode* copy = node->Clone();
            data->ExecCommand<InsertStyleCommand>(copy, dest, index);
            SafeRelease(copy);
            index++;
        }

        data->EndBatch();
    }
}

void CommandExecutor::MoveStyles(const DAVA::Vector<StyleSheetNode*>& nodes, StyleSheetsNode* dest, DAVA::int32 destIndex)
{
    Vector<StyleSheetNode*> nodesToMove;
    for (StyleSheetNode* node : nodes)
    {
        if (node->CanRemove() && dest->CanInsertStyle(node, destIndex))
            nodesToMove.push_back(node);
    }

    if (!nodesToMove.empty())
    {
        DocumentData* data = GetDocumentData();
        data->BeginBatch(Format("Move Styles %s", CommandExecutorDetails::FormatNodeNames(nodes).c_str()), static_cast<uint32>(nodesToMove.size()));
        int index = destIndex;
        for (StyleSheetNode* node : nodesToMove)
        {
            StyleSheetsNode* src = dynamic_cast<StyleSheetsNode*>(node->GetParent());
            if (src)
            {
                int32 srcIndex = src->GetIndex(node);

                if (src == dest && index > srcIndex)
                    index--;

                node->Retain();
                data->ExecCommand<RemoveStyleCommand>(node, src, srcIndex);
                if (IsNodeInHierarchy(dest))
                {
                    data->ExecCommand<InsertStyleCommand>(node, dest, index);
                }
                node->Release();

                index++;
            }
            else
            {
                DVASSERT(false);
            }
        }

        data->EndBatch();
    }
}

void CommandExecutor::Remove(const Vector<ControlNode*>& controls, const Vector<StyleSheetNode*>& styles) const
{
    Vector<PackageBaseNode*> nodesToRemove;

    Vector<ControlNode*> controlsToRemove;
    for (ControlNode* control : controls)
    {
        if (control->CanRemove())
        {
            bool hasPrototype = std::find_if(controls.begin(), controls.end(), [control](const ControlNode* otherControl) {
                                    return control->GetPrototype() == otherControl;
                                }) != controls.end();
            if (!hasPrototype)
            {
                controlsToRemove.push_back(control);
                nodesToRemove.push_back(control);
            }
        }
    }

    Vector<StyleSheetNode*> stylesToRemove;
    for (StyleSheetNode* style : styles)
    {
        if (style->CanRemove())
        {
            stylesToRemove.push_back(style);
            nodesToRemove.push_back(style);
        }
    }

    if (!nodesToRemove.empty())
    {
        DocumentData* data = GetDocumentData();
        data->BeginBatch(Format("Remove %s", CommandExecutorDetails::FormatNodeNames(nodesToRemove).c_str()), static_cast<uint32>(stylesToRemove.size()));
        for (ControlNode* control : controlsToRemove)
        {
            if (dynamic_cast<PackageControlsNode*>(control->GetParent()) != nullptr)
            {
                if (FindRootWithSameName(control, data->GetPackageNode()) == false)
                {
                    data->ExecCommand<SetGuidesCommand>(control->GetName(), Vector2::AXIS_X, PackageNode::AxisGuides());
                    data->ExecCommand<SetGuidesCommand>(control->GetName(), Vector2::AXIS_Y, PackageNode::AxisGuides());
                }
            }
            RemoveControlImpl(control);
        }
        for (StyleSheetNode* style : stylesToRemove)
        {
            StyleSheetsNode* src = dynamic_cast<StyleSheetsNode*>(style->GetParent());
            if (src)
            {
                int32 srcIndex = src->GetIndex(style);
                data->ExecCommand<RemoveStyleCommand>(style, src, srcIndex);
            }
        }
        data->EndBatch();
    }
}

SelectedNodes CommandExecutor::Paste(PackageNode* root, PackageBaseNode* dest, int32 destIndex, const DAVA::String& data)
{
    auto ShowWarnNotification = [this](String msg)
    {
        NotificationParams notificationParams;
        notificationParams.title = "Can't paste";
        notificationParams.message = Result(Result::RESULT_WARNING, msg);
        ui->ShowNotification(DAVA::mainWindowKey, notificationParams);
    };

    if (dest->IsReadOnly())
    {
        ShowWarnNotification("paste destination is read-only");
        return SelectedNodes();
    }

    ControlsContainerNode* controlsDest = dynamic_cast<ControlsContainerNode*>(dest);
    StyleSheetsNode* stylesDest = dynamic_cast<StyleSheetsNode*>(dest);

    if (controlsDest == nullptr && stylesDest == nullptr)
    {
        ShowWarnNotification("destination is not a controls/styles container");
        return SelectedNodes();
    }

    RefPtr<YamlParser> parser(YamlParser::CreateAndParseString(data));
    if (!parser.Valid() || !parser->GetRootNode())
    {
        DAVA::Logger::Error("pasted data is not a valid yaml");
        return SelectedNodes();
    }

    QuickEdPackageBuilder builder(GetEngineContext());

    builder.AddImportedPackage(root);
    for (int32 i = 0; i < root->GetImportedPackagesNode()->GetCount(); i++)
    {
        builder.AddImportedPackage(root->GetImportedPackagesNode()->GetImportedPackage(i));
    }

    ProjectData* projectData = GetProjectData();
    if (UIPackageLoader(projectData->GetPrototypes()).LoadPackage(parser->GetRootNode(), root->GetPath(), &builder))
    {
        if (!builder.GetResults().HasErrors())
        {
            SelectedNodes createdNodes;

            const Vector<PackageNode*>& importedPackages = builder.GetImportedPackages();
            const Vector<ControlNode*>& controls = builder.GetRootControls();
            const Vector<StyleSheetNode*>& styles = builder.GetStyles();

            if (controlsDest != nullptr)
            {
                Vector<ControlNode*> acceptedControls;
                Vector<PackageNode*> acceptedPackages;
                Vector<PackageNode*> declinedPackages;

                for (PackageNode* importedPackage : importedPackages)
                {
                    if (importedPackage != root && importedPackage->GetParent() != root->GetImportedPackagesNode())
                    {
                        if (root->GetImportedPackagesNode()->CanInsertImportedPackage(importedPackage))
                            acceptedPackages.push_back(importedPackage);
                        else
                            declinedPackages.push_back(importedPackage);
                    }
                }

                for (ControlNode* control : controls)
                {
                    if (dest->CanInsertControl(control, destIndex))
                    {
                        bool canInsert = true;
                        for (PackageNode* declinedPackage : declinedPackages)
                        {
                            if (control->IsDependsOnPackage(declinedPackage))
                            {
                                canInsert = false;
                                break;
                            }
                        }

                        if (canInsert)
                        {
                            acceptedControls.push_back(control);
                        }
                    }
                }

                if (!acceptedControls.empty())
                {
                    DocumentData* documentData = GetDocumentData();
                    documentData->BeginBatch("Paste", static_cast<uint32>(acceptedControls.size()));
                    for (PackageNode* importedPackage : acceptedPackages)
                    {
                        AddImportedPackageIntoPackageImpl(importedPackage, root);
                    }

                    int32 index = destIndex;
                    for (ControlNode* control : acceptedControls)
                    {
                        createdNodes.insert(control);
                        InsertControl(control, controlsDest, index);
                        index++;
                    }

                    documentData->EndBatch();
                }
            }
            else if (stylesDest != nullptr && !styles.empty())
            {
                DocumentData* documentData = GetDocumentData();
                documentData->BeginBatch("Paste");
                int32 index = destIndex;
                for (StyleSheetNode* style : styles)
                {
                    createdNodes.insert(style);
                    documentData->ExecCommand<InsertStyleCommand>(style, stylesDest, index);
                    index++;
                }

                documentData->EndBatch();
            }

            return createdNodes;
        }
        else
        {
            ShowWarnNotification(builder.GetResults().GetResultMessages());
            return SelectedNodes();
        }
    }
    else
    {
        ShowWarnNotification(Format("Can't load package '%s'", root->GetPath().GetAbsolutePathname().c_str()));
        return SelectedNodes();
    }
}

void CommandExecutor::AddImportedPackageIntoPackageImpl(PackageNode* importedPackage, const PackageNode* package)
{
    DocumentData* data = GetDocumentData();
    data->ExecCommand<InsertImportedPackageCommand>(importedPackage, package->GetImportedPackagesNode()->GetCount());
}

void CommandExecutor::InsertControlImpl(ControlNode* control, ControlsContainerNode* dest, DAVA::int32 destIndex) const
{
    DocumentData* data = GetDocumentData();
    data->ExecCommand<InsertControlCommand>(control, dest, destIndex);

    ControlNode* destControl = dynamic_cast<ControlNode*>(dest);
    if (destControl)
    {
        const Vector<ControlNode*>& instances = destControl->GetInstances();
        for (ControlNode* instance : instances)
        {
            ControlNode* copy = ControlNode::CreateFromPrototypeChild(control);
            InsertControlImpl(copy, instance, destIndex);
            SafeRelease(copy);
        }
    }
}

void CommandExecutor::RemoveControlImpl(ControlNode* node) const
{
    ControlsContainerNode* src = dynamic_cast<ControlsContainerNode*>(node->GetParent());
    if (src)
    {
        int32 srcIndex = src->GetIndex(node);
        node->Retain();
        DocumentData* data = GetDocumentData();
        data->ExecCommand<RemoveControlCommand>(node, src, srcIndex);

        Vector<ControlNode*> instances = node->GetInstances();
        for (ControlNode* instance : instances)
            RemoveControlImpl(instance);

        node->Release();
    }
    else
    {
        DVASSERT(false);
    }
}

bool CommandExecutor::MoveControlImpl(ControlNode* moved, ControlsContainerNode* dest, DAVA::int32 destIndex) const
{
    RefPtr<ControlNode> nodeGuard = RefPtr<ControlNode>::ConstructWithRetain(moved);

    ControlsContainerNode* src = dynamic_cast<ControlsContainerNode*>(moved->GetParent());
    bool result = false;
    if (src != nullptr)
    {
        Vector<ControlNode*> instances = moved->GetInstances();
        Vector<ControlNode*> destInstances;

        bool ofSamePrototype = false;
        ControlNode* prototypeRootOfDest = nullptr;

        ControlNode* destControl = dynamic_cast<ControlNode*>(dest);
        if (destControl != nullptr)
        {
            destInstances = destControl->GetInstances();

            if (instances.empty() == false && destInstances.empty() == false)
            {
                ControlNode* prototypeRootOfMoved = GetRootControlNode(moved);
                prototypeRootOfDest = GetRootControlNode(destControl);

                if (prototypeRootOfMoved == prototypeRootOfDest)
                {
                    ofSamePrototype = true;
                }
                else if (instances.size() > 1)
                {
                    DAVA::NotificationParams params;
                    params.title = "Ambiguity of move operation";
                    params.message = Result(Result::RESULT_WARNING, "Can't match source and destination instances. Result may be incorrect");
                    ui->ShowNotification(DAVA::mainWindowKey, params);
                }
            }
        }

        int32 srcIndex = src->GetIndex(moved);
        DocumentData* data = GetDocumentData();
        data->ExecCommand<RemoveControlCommand>(moved, src, srcIndex);

        if (IsNodeInHierarchy(dest) == true)
        {
            data->ExecCommand<InsertControlCommand>(moved, dest, destIndex);

            for (ControlNode* destInstance : destInstances)
            {
                Vector<ControlNode*>::iterator instanceIt = instances.end();

                if (ofSamePrototype == true)
                {
                    ControlNode* instanceRootOfDest = CommandExecutorDetails::GetInstanceRoot(destInstance, prototypeRootOfDest);
                    instanceIt = std::find_if(instances.begin(), instances.end(), [instanceRootOfDest](ControlNode* instance)
                                              {
                                                  return instanceRootOfDest->IsParentOf(instance);
                                              });
                }
                else if (instances.empty() == false)
                {
                    // get first src instance because it is no matter, we have ambiguity there
                    instanceIt = instances.begin();
                }

                if (instanceIt != instances.end())
                {
                    ControlNode* src = *instanceIt;
                    instances.erase(instanceIt);
                    MoveControlImpl(src, destInstance, destIndex);
                }
                else
                {
                    ControlNode* copy = ControlNode::CreateFromPrototypeChild(moved);
                    InsertControlImpl(copy, destInstance, destIndex);
                    SafeRelease(copy);
                }
            }

            result = true;
        }

        for (ControlNode* instance : instances)
            RemoveControlImpl(instance);
    }
    else
    {
        DVASSERT(false);
    }

    return result;
}

void CommandExecutor::AddComponentImpl(ControlNode* node, const Type* type, int32 index, ComponentPropertiesSection* prototypeSection)
{
    ComponentPropertiesSection* destSection = nullptr;
    DocumentData* data = GetDocumentData();

    if (!UIComponentUtils::IsMultiple(type))
    {
        destSection = node->GetRootProperty()->FindComponentPropertiesSection(type, index);
        if (destSection)
        {
            data->ExecCommand<AttachComponentPrototypeSectionCommand>(node, destSection, prototypeSection);
        }
    }

    if (destSection == nullptr)
    {
        ComponentPropertiesSection* section = new ComponentPropertiesSection(node->GetControl(), type, index, prototypeSection);
        data->ExecCommand<AddComponentCommand>(node, section);

        for (ControlNode* instance : node->GetInstances())
            AddComponentImpl(instance, type, index, section);

        SafeRelease(section);
    }
}

void CommandExecutor::RemoveComponentImpl(ControlNode* node, ComponentPropertiesSection* section)
{
    DocumentData* data = GetDocumentData();
    data->ExecCommand<RemoveComponentCommand>(node, section);
    Vector<ControlNode*> instances = node->GetInstances();
    for (ControlNode* instance : instances)
    {
        ComponentPropertiesSection* instanceSection = instance->GetRootProperty()->FindComponentPropertiesSection(section->GetComponentType(), section->GetComponentIndex());
        RemoveComponentImpl(instance, instanceSection);
    }
}

bool CommandExecutor::IsNodeInHierarchy(const PackageBaseNode* node) const
{
    PackageBaseNode* p = node->GetParent();
    const PackageNode* root = GetDocumentData()->GetPackageNode();
    while (p)
    {
        if (p == root)
            return true;
        p = p->GetParent();
    }
    return false;
}

DocumentData* CommandExecutor::GetDocumentData() const
{
    DataContext* context = accessor->GetActiveContext();
    DVASSERT(context != nullptr);
    DocumentData* data = context->GetData<DocumentData>();
    DVASSERT(data != nullptr);
    return data;
}

ProjectData* CommandExecutor::GetProjectData() const
{
    DataContext* globalContext = accessor->GetGlobalContext();
    DVASSERT(globalContext != nullptr);
    ProjectData* data = globalContext->GetData<ProjectData>();
    DVASSERT(data != nullptr);
    return data;
}
