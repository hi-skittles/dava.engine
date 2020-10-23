#include "Classes/Application/REApplication.h"

int DAVAMain(DAVA::Vector<DAVA::String> cmdline)
{
    REApplication app(std::move(cmdline));
    return app.Run();
}
