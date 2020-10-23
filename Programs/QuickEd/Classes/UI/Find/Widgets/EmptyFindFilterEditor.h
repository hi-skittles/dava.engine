#pragma once

#include "UI/Find/Widgets/FindFilterEditor.h"

#include <Base/BaseTypes.h>
#include <Functional/Functional.h>

class EmptyFindFilterEditor
: public FindFilterEditor
{
public:
    using FindFilterBuilder = DAVA::Function<std::unique_ptr<FindFilter>()>;

    EmptyFindFilterEditor(QWidget* parent, const FindFilterBuilder& findFilterBuilder_);

    std::unique_ptr<FindFilter> BuildFindFilter() override;

private:
    FindFilterBuilder findFilterBuilder;
};
