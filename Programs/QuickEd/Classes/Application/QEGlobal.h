#pragma once

#include <TArc/Core/OperationRegistrator.h>
#include <TArc/WindowSubSystem/UI.h>

namespace QEGlobal
{
//open las opened project. If no one project was opened - do nothing
DECLARE_OPERATION_ID(OpenLastProject);

// if document by this path i already open -make it active
// if no opened documents with current path - creates new one and make it active
DECLARE_OPERATION_ID(OpenDocumentByPath);

// try to close all documents.
// if one or more documents has unsaved changes - request user for permissions of this operation
// if user cancel this operation- nothing will happen, so after this operation you always need to check contexts count
// ask y_rakhuba for extra details
DECLARE_OPERATION_ID(CloseAllDocuments);

//later we will make button to highlight file in fileSystem widget and this operation will be removed
//select file in FileSystemDockWidget by it path
DECLARE_OPERATION_ID(SelectFile);

//Select control by document name and control name
//If document was not open - opens it and than select control
DECLARE_OPERATION_ID(SelectControl);

//Find controls in project using provided filter and dump them to results widget
DECLARE_OPERATION_ID(FindInProject);

//Find controls in current document using provided filter and dump them to results widget
DECLARE_OPERATION_ID(FindInDocument);

// Reload sprites and refresh UIControls
// Has no arguments
DECLARE_OPERATION_ID(ReloadSprites);

// Create passed control by clicking somewhere in central widget
DECLARE_OPERATION_ID(CreateByClick);

// Drop mime data into package base node
// arguments are: const QMimeData* data, Qt::DropAction action, PackageBaseNode* destNode, DAVA::uint32 destIndex, const DAVA::Vector2* pos
DECLARE_OPERATION_ID(DropIntoPackageNode);

//Duplicate selected controls (copy and past them to a parent)
// Has no arguments
DECLARE_OPERATION_ID(Duplicate);
}
