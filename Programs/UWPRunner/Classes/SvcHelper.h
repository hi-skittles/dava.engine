#ifndef SVCHELPER_H
#define SVCHELPER_H

#include "Base/BaseTypes.h"

class SvcHelper
{
public:
    SvcHelper(const DAVA::WideString& name);
    ~SvcHelper();

    DAVA::WideString ServiceName() const;
    DAVA::WideString ServiceDescription() const;

    bool IsInstalled() const;
    bool IsRunning() const;
    bool Start();
    bool Stop();

private:
    DAVA::WideString serviceName;
    SC_HANDLE serviceControlManager = nullptr;
    SC_HANDLE service = nullptr;
};

#endif // SVCHELPER_H