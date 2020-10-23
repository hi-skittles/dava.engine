#include "AbstractPropertyDelegate.h"
#include "PropertiesTreeItemDelegate.h"

AbstractPropertyDelegate::AbstractPropertyDelegate(PropertiesTreeItemDelegate* delegate /*= NULL*/)
    : itemDelegate(delegate)
{
}

AbstractPropertyDelegate::~AbstractPropertyDelegate()
{
}
