#pragma once

#include <QList>
#include <QMetaObject>
#include <QMetaType>

class ConsoleTasksCollection
{
public:
    static ConsoleTasksCollection* Instance();

    void RegisterConsoleTask(const char* name);

    const QList<const char*>& GetClassNames() const;

private:
    QList<const char*> tasks;
};

template <typename T>
struct ConsoleTasksRegistrator
{
    ConsoleTasksRegistrator(const char* name)
    {
        qRegisterMetaType<T>();
        ConsoleTasksCollection::Instance()->RegisterConsoleTask(name);
    }
};

#define REGISTER_CLASS(Type) ConsoleTasksRegistrator<Type> registrator_##Type(#Type);
