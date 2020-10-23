#include "Core/ConsoleTasks/ConsoleTasksCollection.h"

#include <QMetaType>

ConsoleTasksCollection* ConsoleTasksCollection::Instance()
{
    static ConsoleTasksCollection self;
    return &self;
}

void ConsoleTasksCollection::RegisterConsoleTask(const char* name)
{
    tasks.append(name);
}

const QList<const char*>& ConsoleTasksCollection::GetClassNames() const
{
    return tasks;
}
