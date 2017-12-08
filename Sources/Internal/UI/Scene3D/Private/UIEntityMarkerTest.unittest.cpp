#include "UnitTests/UnitTests.h"

#include "Base/BaseTypes.h"
#include "Base/RefPtr.h"
#include "Base/ScopedPtr.h"
#include "Engine/Engine.h"
#include "Render/Highlevel/Camera.h"
#include "Scene3D/Components/CameraComponent.h"
#include "Scene3D/Components/ScreenPositionComponent.h"
#include "Scene3D/Scene.h"
#include "Scene3D/Systems/ScreenPositionSystem.h"
#include "UI/Scene3D/UIEntityMarkerComponent.h"
#include "UI/Scene3D/UIEntityMarkerSystem.h"
#include "UI/UI3DView.h"
#include "UI/UIControl.h"
#include "UI/UIControlSystem.h"
#include "UI/UIScreen.h"

DAVA_TESTCLASS (UIEntityMarkerTest)
{
    BEGIN_FILES_COVERED_BY_TESTS()
    FIND_FILES_IN_TARGET(DavaFramework)
    DECLARE_COVERED_FILES("UIEntityMarkerComponent.cpp")
    DECLARE_COVERED_FILES("UIEntityMarkerSystem.cpp")
    END_FILES_COVERED_BY_TESTS();

    DAVA::RefPtr<DAVA::Scene> scene;
    DAVA::RefPtr<DAVA::Camera> camera;
    DAVA::RefPtr<DAVA::UI3DView> ui3dView;
    DAVA::RefPtr<DAVA::UIControl> c1;
    DAVA::RefPtr<DAVA::UIControl> c2;
    DAVA::RefPtr<DAVA::UIControl> markers;

    DAVA::RefPtr<DAVA::Entity> CreateEntity(const DAVA::FastName& name, const DAVA::Vector3& position)
    {
        using namespace DAVA;
        RefPtr<Entity> entity(new Entity());
        entity->SetName(name);

        Matrix4 m;
        m.BuildTranslation(position);
        entity->SetLocalTransform(m);

        entity->AddComponent(new ScreenPositionComponent());

        return entity;
    }

    DAVA::RefPtr<DAVA::UIControl> CreateControl(DAVA::Entity * entity)
    {
        using namespace DAVA;
        RefPtr<UIControl> control(new UIControl());
        control->SetName(entity->GetFullName());
        UIEntityMarkerComponent* emc = control->GetOrCreateComponent<UIEntityMarkerComponent>();
        emc->SetTargetEntity(entity);
        return control;
    }

    void ModifyComponentForEach(const DAVA::Vector<DAVA::RefPtr<DAVA::UIControl>>& ctrls, const DAVA::Function<void(DAVA::UIEntityMarkerComponent * emc)>& fn)
    {
        using namespace DAVA;
        for (auto& ctrl : ctrls)
        {
            UIEntityMarkerComponent* emc = ctrl->GetOrCreateComponent<UIEntityMarkerComponent>();
            fn(emc);
        }
    }

    void SetUp(const DAVA::String& testName) override
    {
        using namespace DAVA;

        scene = new Scene();

        RefPtr<Entity> e1 = CreateEntity(FastName("ENTITY_ONE"), Vector3(-1.f, -1.f, -1.f));
        scene->AddNode(e1.Get());
        RefPtr<Entity> e2 = CreateEntity(FastName("ENTITY_TWO"), Vector3(1.f, 1.f, 1.f));
        scene->AddNode(e2.Get());

        camera = new Camera();
        camera->SetupPerspective(70.f, 1.0, 0.5f, 2500.f);
        camera->SetLeft(Vector3(1, 0, 0));
        camera->SetUp(Vector3(0, 0, 1.f));
        camera->SetTarget(Vector3(0, 0, 0));
        camera->SetPosition(Vector3(0, -2, 0));

        RefPtr<Entity> cameraEntity(new Entity());
        cameraEntity->AddComponent(new CameraComponent(camera.Get()));
        scene->AddNode(cameraEntity.Get());
        scene->AddCamera(camera.Get());
        scene->SetCurrentCamera(camera.Get());

        ui3dView = new UI3DView(Rect(0, 0, 500, 500));
        ui3dView->SetScene(scene.Get());

        c1 = CreateControl(e1.Get());
        c2 = CreateControl(e2.Get());
        markers = new UIControl();
        markers->AddControl(c1.Get());
        markers->AddControl(c2.Get());

        RefPtr<UIScreen> screen(new UIScreen());
        screen->AddControl(ui3dView.Get());
        screen->AddControl(markers.Get());
        DAVA::GetEngineContext()->uiControlSystem->SetScreen(screen.Get());
    }

    void TearDown(const DAVA::String& testName) override
    {
        c1 = nullptr;
        c2 = nullptr;
        markers = nullptr;
        ui3dView = nullptr;
        camera = nullptr;
        scene = nullptr;
    }

    void SystemsUpdate(DAVA::float32 delta)
    {
        DAVA::GetEngineContext()->uiControlSystem->UpdateWithCustomTime(delta);
    }

    DAVA_TEST (ComponentTest)
    {
        using namespace DAVA;
        RefPtr<UIControl> c3 = c1->SafeClone();
        TEST_VERIFY(c3->GetComponent<UIEntityMarkerComponent>());

        RefPtr<UIControl> c4(new UIControl());
        c1->AddControl(c4.Get());

        c4->GetOrCreateComponent<UIEntityMarkerComponent>();
        TEST_VERIFY(c4->GetComponentCount<UIEntityMarkerComponent>() == 1);

        c4->RemoveComponent<UIEntityMarkerComponent>();
        TEST_VERIFY(c4->GetComponentCount<UIEntityMarkerComponent>() == 0);
    }

    DAVA_TEST (PositionTest)
    {
        using namespace DAVA;

        const struct
        {
            RefPtr<UIControl> c;
            Vector2 data;
            float32 eps;
        } testData[] = {
            { c1, { -107.f, 607.f }, 0.1f },
            { c2, { 369.f, 131.f }, 0.1f }
        };

        ModifyComponentForEach({ c1, c2 }, [](UIEntityMarkerComponent* emc) {
            emc->SetSyncPositionEnabled(true);
        });
        SystemsUpdate(0.f);

        for (const auto& test : testData)
        {
            TEST_VERIFY(test.c->GetComponent<UIEntityMarkerComponent>()->IsSyncPositionEnabled());
            Vector2 pos = test.c->GetPosition();
            TEST_VERIFY(FLOAT_EQUAL_EPS(pos.x, test.data.x, test.eps));
            TEST_VERIFY(FLOAT_EQUAL_EPS(pos.y, test.data.y, test.eps));
        }
    }

    DAVA_TEST (ScaleTest)
    {
        using namespace DAVA;

        const struct
        {
            Vector3 camPos;
            struct
            {
                RefPtr<UIControl> c;
                Vector2 value;
                float32 eps;
            } data[2];
        } testData[] = {
            { { 0.f, -2.f, 0.f }, { { c1, { .57f, .57f }, .01f }, { c2, { .3f, .3f }, .01f } } },
            { { -1.f, -1.f, -1.f }, { { c1, { 2.f, 2.f }, .01f }, { c2, { .28f, .28f }, .01f } } },
            { { 0.f, 10.f, 0.f }, { { c1, { .1f, .1f }, .01f }, { c2, { .1f, .1f }, .01f } } }
        };

        ModifyComponentForEach({ c1, c2 }, [](UIEntityMarkerComponent* emc) {
            emc->SetSyncScaleEnabled(true);
        });
        for (const auto& test : testData)
        {
            camera->SetPosition(test.camPos);
            SystemsUpdate(0.f);

            for (const auto& d : test.data)
            {
                TEST_VERIFY(d.c->GetComponent<UIEntityMarkerComponent>()->IsSyncScaleEnabled());
                Vector2 scale = d.c->GetScale();
                TEST_VERIFY(FLOAT_EQUAL_EPS(scale.x, d.value.x, d.eps));
                TEST_VERIFY(FLOAT_EQUAL_EPS(scale.y, d.value.y, d.eps));
            }
        }
    }

    DAVA_TEST (OrderTest)
    {
        using namespace DAVA;

        const struct
        {
            Vector3 camPos;
            List<RefPtr<UIControl>> order; // Last element in list will be top on screen
        } testData[] = {
            { { 0.f, -2.f, 0.f }, { c2, c1 } },
            { { 0.f, 2.f, 0.f }, { c1, c2 } }
        };

        ModifyComponentForEach({ c1, c2 }, [](UIEntityMarkerComponent* emc) {
            emc->SetSyncOrderEnabled(true);
            emc->SetOrderMode(UIEntityMarkerComponent::OrderMode::NearFront);
        });
        for (const auto& test : testData)
        {
            camera->SetPosition(test.camPos);
            SystemsUpdate(0.f);
            TEST_VERIFY(std::equal(test.order.begin(), test.order.end(), markers->GetChildren().begin()));
        }

        ModifyComponentForEach({ c1, c2 }, [](UIEntityMarkerComponent* emc) {
            emc->SetOrderMode(UIEntityMarkerComponent::OrderMode::NearBack);
        });
        for (const auto& test : testData)
        {
            camera->SetPosition(test.camPos);
            SystemsUpdate(0.f);
            TEST_VERIFY(std::equal(test.order.begin(), test.order.end(), markers->GetChildren().rbegin()));
        }
    }

    DAVA_TEST (VisibilityTest)
    {
        using namespace DAVA;

        const struct
        {
            Vector3 camPos;
            Map<RefPtr<UIControl>, bool> data;
        } testData[] = {
            { { 0.f, -2.f, 0.f }, { { c1, true }, { c2, true } } },
            { { 0.f, -.5f, 0.f }, { { c1, false }, { c2, true } } },
            { { 0.f, .5f, 0.f }, { { c1, true }, { c2, false } } }
        };

        ModifyComponentForEach({ c1, c2 }, [](UIEntityMarkerComponent* emc) {
            emc->SetSyncVisibilityEnabled(true);
        });
        for (const auto& test : testData)
        {
            camera->SetPosition(test.camPos);
            SystemsUpdate(0.f);
            for (auto& pair : test.data)
            {
                TEST_VERIFY(pair.first->GetVisibilityFlag() == pair.second);
            }
        }
    }

    DAVA_TEST (CustomStrategyTest)
    {
        using namespace DAVA;
        ModifyComponentForEach({ c1, c2 }, [](UIEntityMarkerComponent* emc) {
            emc->SetUseCustomStrategy(true);
            emc->SetCustomStrategy([](UIControl* c, UIEntityMarkerComponent* emc, ScreenPositionComponent* spc) {
                TEST_VERIFY(c != nullptr);
                TEST_VERIFY(emc != nullptr);
                TEST_VERIFY(spc != nullptr);
            });
        });
        SystemsUpdate(0.f);
    }
};
