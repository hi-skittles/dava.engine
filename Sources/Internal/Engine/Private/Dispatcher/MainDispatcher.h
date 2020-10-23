#pragma once

#include "Base/BaseTypes.h"

#include "Concurrency/Dispatcher.h"
#include "Engine/Private/Dispatcher/MainDispatcherEvent.h"

namespace DAVA
{
namespace Private
{
using MainDispatcher = Dispatcher<MainDispatcherEvent>;

} // namespace Private
} // namespace DAVA
