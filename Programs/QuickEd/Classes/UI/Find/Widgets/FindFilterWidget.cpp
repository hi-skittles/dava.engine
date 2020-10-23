#include "UI/Find/Widgets/FindFilterWidget.h"
#include "UI/Find/Filters/AcceptsInputFilter.h"
#include "UI/Find/Filters/ControlNameFilter.h"
#include "UI/Find/Filters/HasComponentFilter.h"
#include "UI/Find/Filters/HasClassesFilter.h"
#include "UI/Find/Filters/NegationFilter.h"
#include "UI/Find/Filters/HasErrorsFilter.h"
#include "UI/Find/Filters/PackageVersionFilter.h"
#include "UI/Find/Widgets/EmptyFindFilterEditor.h"
#include "UI/Find/Widgets/ComboBoxFilterEditor.h"
#include "UI/Find/Widgets/RegExpStringFindFilterEditor.h"
#include "UI/Find/Widgets/StringFindFilterEditor.h"
#include "UI/Find/Widgets/EnumAndStringFindFilterEditor.h"
#include <Engine/Engine.h>
#include <Entity/ComponentManager.h>
#include <Reflection/ReflectedTypeDB.h>
#include <Utils/Utils.h>

using namespace DAVA;

namespace FindFilterWidgetDetail
{
template <typename Meta>
bool HasMeta(const Type* t)
{
    if (t == nullptr)
    {
        return false;
    }

    const ReflectedType* refType = ReflectedTypeDB::GetByType(t);
    if (refType == nullptr)
    {
        return false;
    }

    const ReflectedStructure* structure = refType->GetStructure();
    if (structure == nullptr || structure->meta == nullptr)
    {
        return false;
    }

    return structure->meta->GetMeta<Meta>() != nullptr;
}

class AbstractFindFilter
{
public:
    virtual ~AbstractFindFilter() = default;

    virtual const char* GetName() = 0;
    virtual FindFilterEditor* CreateEditor(QWidget* parent) = 0;
};

class NameFindFilter
: public AbstractFindFilter
{
public:
    const char* GetName() override
    {
        return "Name";
    }

    FindFilterEditor* CreateEditor(QWidget* parent) override
    {
        return new RegExpStringFindFilterEditor(parent,
                                                [](const RegExpStringFindFilterEditor* editor)
                                                {
                                                    return std::make_unique<ControlNameFilter>(".*" + editor->GetString() + ".*", editor->IsCaseSensitive());
                                                });
    }
};

class HasComponentFindFilter
: public AbstractFindFilter
{
public:
    const char* GetName() override
    {
        return "Has component";
    }

    FindFilterEditor* CreateEditor(QWidget* parent) override
    {
        Vector<ComboBoxFilterEditor::ComboBoxData> data;

        auto& components = GetEngineContext()->componentManager->GetRegisteredUIComponents();
        int32 i = 0;
        for (auto& c : components)
        {
            if (!HasMeta<M::HiddenField>(c))
            {
                ComboBoxFilterEditor::ComboBoxData d;
                d.description = ReflectedTypeDB::GetByType(c)->GetPermanentName();
                d.userData = reinterpret_cast<uint64>(c);
                data.push_back(d);
            }
        }

        return new ComboBoxFilterEditor(parent,
                                        data,
                                        [](const ComboBoxFilterEditor* editor)
                                        {
                                            return std::make_unique<HasComponentFilter>(reinterpret_cast<const Type*>(editor->GetUserData()));
                                        });
    }
};

class HasClassFindFilter
: public AbstractFindFilter
{
public:
    const char* GetName() override
    {
        return "Has classes";
    }

    FindFilterEditor* CreateEditor(QWidget* parent) override
    {
        return new StringFindFilterEditor(parent,
                                          [](const StringFindFilterEditor* editor)
                                          {
                                              Vector<String> classes;
                                              Split(editor->GetString(), " ", classes);
                                              return std::make_unique<HasClassesFilter>(classes);
                                          });
    }
};

class AcceptsInputFindFilter
: public AbstractFindFilter
{
public:
    const char* GetName() override
    {
        return "Accepts input";
    }

    FindFilterEditor* CreateEditor(QWidget* parent) override
    {
        return new EmptyFindFilterEditor(parent,
                                         []()
                                         {
                                             return std::make_unique<AcceptsInputFilter>();
                                         });
    }
};

class HasErrorsFindFilter
: public AbstractFindFilter
{
public:
    const char* GetName() override
    {
        return "Errors";
    }

    FindFilterEditor* CreateEditor(QWidget* parent) override
    {
        return new EmptyFindFilterEditor(parent,
                                         []()
                                         {
                                             return std::make_unique<HasErrorsFilter>();
                                         });
    }
};

class PackageVersionFindFilter
: public AbstractFindFilter
{
public:
    const char* GetName() override
    {
        return "Package version";
    }

    FindFilterEditor* CreateEditor(QWidget* parent) override
    {
        return new EnumAndStringFindFilterEditor(parent,
                                                 GlobalEnumMap<PackageVersionFilter::eCmpType>::Instance(),
                                                 [](const EnumAndStringFindFilterEditor* editor)
                                                 {
                                                     int32 version = QString::fromStdString(editor->GetString()).toInt();
                                                     PackageVersionFilter::eCmpType cmpType = static_cast<PackageVersionFilter::eCmpType>(editor->GetEnumValue());
                                                     return std::make_unique<PackageVersionFilter>(version, cmpType);
                                                 });
    }
};

static const Array<std::unique_ptr<AbstractFindFilter>, 6> FILTERS
{
  std::make_unique<NameFindFilter>(),
  std::make_unique<HasComponentFindFilter>(),
  std::make_unique<HasClassFindFilter>(),
  std::make_unique<AcceptsInputFindFilter>(),
  std::make_unique<HasErrorsFindFilter>(),
  std::make_unique<PackageVersionFindFilter>()
};
}

FindFilterWidget::FindFilterWidget(QWidget* parent)
    : QWidget(parent)
{
    using namespace FindFilterWidgetDetail;

    layout = new QHBoxLayout(this);

    addFilterButton = new QToolButton(this);
    addFilterButton->setText("+");

    removeFilterButton = new QToolButton(this);
    removeFilterButton->setText("-");

    negationButton = new QToolButton(this);
    negationButton->setText("Not");
    negationButton->setCheckable(true);

    filterCombobox = new QComboBox(this);
    filterCombobox->setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToContents);

    for (const std::unique_ptr<AbstractFindFilter>& filter : FILTERS)
    {
        filterCombobox->addItem(tr(filter->GetName()));
    }

    layout->setSpacing(0);
    layout->setMargin(0);

    innerLayout = new QHBoxLayout();

    innerLayout->setSpacing(0);
    innerLayout->setMargin(0);

    layout->addWidget(addFilterButton);
    layout->addWidget(removeFilterButton);
    layout->addSpacing(10);
    layout->addWidget(negationButton);
    layout->addSpacing(10);
    layout->addWidget(filterCombobox);
    layout->addSpacing(10);
    layout->addLayout(innerLayout);
    layout->addSpacing(10);

    QObject::connect(addFilterButton, SIGNAL(clicked()), this, SIGNAL(AddAnotherFilter()));
    QObject::connect(removeFilterButton, SIGNAL(clicked()), this, SIGNAL(RemoveFilter()));
    QObject::connect(filterCombobox, SIGNAL(currentIndexChanged(int)), this, SLOT(FilterSelected(int)));
    QObject::connect(negationButton, SIGNAL(toggled(bool)), this, SIGNAL(FilterChanged()));

    FilterSelected(0);
}

std::shared_ptr<FindFilter> FindFilterWidget::BuildFindFilter() const
{
    if (editor != nullptr)
    {
        std::shared_ptr<FindFilter> filter = editor->BuildFindFilter();
        if (negationButton->isChecked())
        {
            return std::make_shared<NegationFilter>(filter);
        }
        else
        {
            return filter;
        }
    }
    else
    {
        return nullptr;
    }
}

void FindFilterWidget::FilterSelected(int index)
{
    innerLayout->removeWidget(editor);
    delete editor;

    editor = FindFilterWidgetDetail::FILTERS[index]->CreateEditor(nullptr);
    innerLayout->addWidget(editor);
    QObject::connect(editor, SIGNAL(FilterChanged()), this, SIGNAL(FilterChanged()));

    setFocusProxy(editor);

    emit FilterChanged();
}
