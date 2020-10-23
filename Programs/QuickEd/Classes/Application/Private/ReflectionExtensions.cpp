#include "Classes/Application/ReflectionExtensions.h"
#include "Classes/Modules/DocumentsModule/DocumentData.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/DataContext.h>
#include <TArc/Qt/QtString.h>
#include <TArc/Utils/CommonFieldNames.h>

#include <Reflection/ReflectionRegistrator.h>

namespace ReflectionExtensionsDetail
{
QString GetDocumentName(DAVA::DataContext* ctx)
{
    DocumentData* data = ctx->GetData<DocumentData>();
    DVASSERT(nullptr != data);
    return data->GetName();
}

QString GetDocumentPath(DAVA::DataContext* ctx)
{
    DocumentData* data = ctx->GetData<DocumentData>();
    DVASSERT(nullptr != data);
    return data->GetPackageAbsolutePath();
}

bool IsDocumentChanged(DAVA::DataContext* ctx)
{
    DocumentData* data = ctx->GetData<DocumentData>();
    DVASSERT(nullptr != data);
    return data->CanSave();
}
}

void RegisterDataContextExt()
{
    using namespace ReflectionExtensionsDetail;
    DAVA::ReflectionRegistrator<DAVA::DataContext>::Begin()
    .Field(DAVA::ContextNameFieldName, &GetDocumentName, nullptr)
    .Field(DAVA::ContextToolTipFieldName, &GetDocumentPath, nullptr)
    .Field(DAVA::IsContextModifiedFieldName, &IsDocumentChanged, nullptr)
    .End();

    DAVA::ReflectionRegistrator<DAVA::ContextAccessor>::Begin()
    .Field(DAVA::MainObjectName, [](DAVA::ContextAccessor*) { return "Document"; }, nullptr)
    .End();
}

void RegisterReflectionExtensions()
{
    RegisterDataContextExt();
}
