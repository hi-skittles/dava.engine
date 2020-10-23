#include <Base/BaseTypes.h>
#include <Base/RefPtr.h>
#include <Base/RefPtrUtils.h>
#include <UI/DefaultUIPackageBuilder.h>
#include <UI/UIPackageLoader.h>
#include <UI/UIControlSystem.h>
#include <UI/UIScreen.h>
#include <UI/UIStaticText.h>
#include <UI/UITextField.h>
#include <UI/Text/UITextComponent.h>
#include <Reflection/Reflection.h>
#include <Reflection/ReflectionRegistrator.h>
#include <UI/DataBinding/UIDataBindingSystem.h>
#include <UI/DataBinding/UIDataBindingPostProcessingSystem.h>
#include <UI/DataBinding/UIDataBindingComponent.h>
#include <UI/DataBinding/UIDataSourceComponent.h>
#include <UI/DataBinding/UIDataListComponent.h>
#include <UI/DataBinding/UIDataChildFactoryComponent.h>
#include <UI/DataBinding/Private/UIDataModel.h>
#include <UI/Formula/FormulaContext.h>

#include "UnitTests/UnitTests.h"

using namespace DAVA;

DAVA_TESTCLASS (UIDataBindingTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("UIDataBindingSystem.cpp")
    END_FILES_COVERED_BY_TESTS();

    struct DataItem : public ReflectionBase
    {
        DataItem(const String& name_)
            : name(name_)
        {
        }

        String name;

        bool operator==(const DataItem& other) const
        {
            return name == other.name;
        }

        bool operator!=(const DataItem& other) const
        {
            return !this->operator==(other);
        }

        DAVA_VIRTUAL_REFLECTION_IN_PLACE(DataItem)
        {
            DAVA::ReflectionRegistrator<DataItem>::Begin()
            .Field("name", &DataItem::name)
            .End();
        }
    };

    struct Data : public ReflectionBase
    {
        int32 a = 0;
        int32 b = 0;
        String str;
        bool flag = false;
        String cellName;
        String name;

        UnorderedMap<String, int> map;

        Vector<DataItem> items;

        DAVA_VIRTUAL_REFLECTION_IN_PLACE(Data)
        {
            DAVA::ReflectionRegistrator<Data>::Begin()
            .Field("a", &Data::a)
            .Field("b", &Data::b)
            .Field("name", &Data::name)
            .Field("str", &Data::str)
            .Field("flag", &Data::flag)
            .Field("map", &Data::map)
            .Field("items", &Data::items)
            .Field("cellName", &Data::cellName)
            .End();
        }
    };

    class IssueDelegate : public UIDataBindingIssueDelegate
    {
    public:
        int32 GenerateNewId() override
        {
            int32 newId = issueId;
            issueId++;
            return newId;
        }

        void OnIssueAdded(int32 id, const String& message, const UIControl* control, const String& propertyName) override
        {
            totalIssuesCount++;
        }

        void OnIssueChanged(int32 id, const String& message) override
        {
        }

        void OnIssueRemoved(int32 id) override
        {
            totalIssuesCount--;
        }

        int32 issueId = 0;
        int32 totalIssuesCount = 0;
    };

    RefPtr<UIScreen> screen;
    RefPtr<UIStaticText> text;
    RefPtr<UITextField> textField;
    RefPtr<UIControl> list;
    IssueDelegate issueDelegate;
    Data data;

    UIDataBindingTest()
    {
        data.map["a"] = 42;
        data.map["b"] = 5;
        data.a = 123;
        data.b = 234;

        data.name = "Fake Name";

        data.items.push_back(DataItem("i1"));
        data.items.push_back(DataItem("i2"));
        data.items.push_back(DataItem("i3"));

        screen = MakeRef<UIScreen>();
        GetEngineContext()->uiControlSystem->SetScreen(screen.Get());
        GetEngineContext()->uiControlSystem->Update();

        UIDataSourceComponent* source = screen->GetOrCreateComponent<UIDataSourceComponent>();
        source->SetData(Reflection::Create(&data));
    }

    ~UIDataBindingTest()
    {
        GetEngineContext()->uiControlSystem->Reset();
    }

    void SetUp(const String& testName) override
    {
        text = MakeRef<UIStaticText>();
        screen->AddControl(text.Get());

        textField = MakeRef<UITextField>();
        screen->AddControl(textField.Get());

        list = MakeRef<UIControl>();
        screen->AddControl(list.Get());
    }

    void TearDown(const String& testName) override
    {
        screen->RemoveControl(text.Get());
        text = nullptr;

        screen->RemoveControl(textField.Get());
        textField = nullptr;

        screen->RemoveControl(list.Get());
        list = nullptr;
    }

    DAVA_TEST (ScopeTest)
    {
        UIDataBindingComponent* bindComp = text->GetOrCreateComponent<UIDataBindingComponent>();
        bindComp->SetUpdateMode(UIDataBindingComponent::MODE_READ);
        bindComp->SetControlFieldName("UITextComponent.text");
        bindComp->SetBindingExpression("a + b");

        UIDataSourceComponent* scope = text->GetOrCreateComponent<UIDataSourceComponent>();
        scope->SetSourceType(UIDataSourceComponent::FROM_EXPRESSION);
        scope->SetSource("map");

        UIDataBindingSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIDataBindingSystem>();
        sys->Process(0.0f);

        TEST_VERIFY(text->GetUtf8Text() == "47");
    }

    DAVA_TEST (BindingTest)
    {
        UIDataBindingComponent* bindComp = text->GetOrCreateComponent<UIDataBindingComponent>();
        bindComp->SetUpdateMode(UIDataBindingComponent::MODE_READ);
        bindComp->SetControlFieldName("UITextComponent.text");
        bindComp->SetBindingExpression("a + b");

        UIDataBindingSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIDataBindingSystem>();
        sys->Process(0.0f);

        TEST_VERIFY(text->GetUtf8Text() == "357");

        std::shared_ptr<FormulaContext> context = sys->GetFormulaContext(text.Get());
        TEST_VERIFY(context->FindReflection("a").IsValid());
    }

    DAVA_TEST (BindingListTest)
    {
        UIDataListComponent* listComp = list->GetOrCreateComponent<UIDataListComponent>();
        listComp->SetCellPackage("~res:/UI/UIDataBinindingCell.yaml");
        listComp->SetCellControlName("UIStaticText");
        listComp->SetDataContainer("items");

        UIDataBindingSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIDataBindingSystem>();
        sys->Process(0.0f);

        TEST_VERIFY(list->GetChildren().size() == data.items.size());
        auto it = list->GetChildren().begin();
        for (size_t i = 0; i < data.items.size(); i++)
        {
            TEST_VERIFY((*it)->GetComponent<UITextComponent>()->GetText() == data.items[i].name);
            ++it;
        }
    }

    DAVA_TEST (BindingChildFactoryTest)
    {
        UIDataChildFactoryComponent* factoryComp = list->GetOrCreateComponent<UIDataChildFactoryComponent>();
        factoryComp->SetPackageExpression("\"~res:/UI/UIDataBinindingCell.yaml\"");
        factoryComp->SetControlExpression("cellName");

        data.cellName = "UIStaticText_1";
        UIDataBindingSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIDataBindingSystem>();
        sys->SetIssueDelegate(&issueDelegate);
        sys->Process(0.0f);

        TEST_VERIFY(issueDelegate.totalIssuesCount == 0);
        sys->SetIssueDelegate(nullptr);

        TEST_VERIFY(list->GetChildren().size() == 1);
        auto it = list->GetChildren().begin();

        TEST_VERIFY((*it)->GetName() == FastName("UIStaticText_1"));
    }

    DAVA_TEST (BindingWriteTest)
    {
        UIDataBindingComponent* bindComp = textField->GetOrCreateComponent<UIDataBindingComponent>();
        bindComp->SetUpdateMode(UIDataBindingComponent::MODE_WRITE);
        bindComp->SetControlFieldName("text");
        bindComp->SetBindingExpression("str");

        textField->SetUtf8Text("fromControlToModel");

        UIDataBindingSystem* sys = GetEngineContext()->uiControlSystem->GetSystem<UIDataBindingSystem>();
        sys->Process(0.0f);

        UIDataBindingPostProcessingSystem* postSys = GetEngineContext()->uiControlSystem->GetSystem<UIDataBindingPostProcessingSystem>();
        postSys->Process(0.0f);

        TEST_VERIFY(data.str == "fromControlToModel");
    }
};
