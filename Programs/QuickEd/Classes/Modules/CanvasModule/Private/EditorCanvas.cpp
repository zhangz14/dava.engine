#include "Modules/CanvasModule/EditorCanvas.h"
#include "Modules/CanvasModule/CanvasModuleData.h"
#include "Modules/CanvasModule/CanvasData.h"

#include "UI/Preview/Data/CentralWidgetData.h"
#include "Modules/UpdateViewsSystemModule/UpdateViewsSystem.h"

#include <TArc/Core/ContextAccessor.h>
#include <TArc/DataProcessing/DataContext.h>

#include <UI/UIControlSystem.h>
#include <UI/UIEvent.h>
#include <UI/UIControl.h>

EditorCanvas::EditorCanvas(DAVA::TArc::ContextAccessor* accessor)
    : BaseEditorSystem(accessor)
    , canvasDataAdapter(accessor)
{
    using namespace DAVA;
    using namespace DAVA::TArc;

    UpdateViewsSystem* updateSystem = GetEngineContext()->uiControlSystem->GetSystem<UpdateViewsSystem>();
    updateSystem->beforeRender.Connect(this, &EditorCanvas::UpdateMovableControlState);

    canvasDataAdapterWrapper = accessor->CreateWrapper([this](const DataContext*) { return Reflection::Create(&canvasDataAdapter); });
}

bool EditorCanvas::CanProcessInput(DAVA::UIEvent* currentInput) const
{
    using namespace DAVA;
    if (accessor->GetActiveContext() == nullptr)
    {
        return false;
    }
    if ((currentInput->device & eInputDevices::CLASS_POINTER) == eInputDevices::UNKNOWN)
    {
        return false;
    }
    if (currentInput->phase == UIEvent::Phase::WHEEL || currentInput->phase == UIEvent::Phase::GESTURE)
    {
        return true;
    }
    return (GetSystemsManager()->GetDragState() == EditorSystemsManager::DragScreen &&
            (currentInput->mouseButton == eMouseButtons::LEFT || currentInput->mouseButton == eMouseButtons::MIDDLE));
}

void EditorCanvas::ProcessInput(DAVA::UIEvent* currentInput)
{
    using namespace DAVA;

    if (currentInput->device == eInputDevices::TOUCH_PAD)
    {
        if (currentInput->phase == UIEvent::Phase::GESTURE)
        {
            const UIEvent::Gesture& gesture = currentInput->gesture;
            if (gesture.dx != 0.0f || gesture.dy != 0.0f)
            {
                Vector2 position = canvasDataAdapter.GetPosition();
                Vector2 newPosition = position - Vector2(gesture.dx, gesture.dy);
                canvasDataAdapter.SetPosition(newPosition);
            }
            else if (gesture.magnification != 0.0f)
            {
                float32 newScale = canvasDataAdapter.GetScale() + gesture.magnification;
                canvasDataAdapter.SetScale(newScale, currentInput->physPoint);
            }
        }
    }
    else if (currentInput->device == eInputDevices::MOUSE)
    {
        if (currentInput->phase == UIEvent::Phase::WHEEL)
        {
            if ((currentInput->modifiers & (eModifierKeys::CONTROL | eModifierKeys::COMMAND)) != eModifierKeys::NONE)
            {
                int32 ticksCount = static_cast<int32>(currentInput->wheelDelta.y);
                float newScale = GetScaleFromWheelEvent(ticksCount);
                canvasDataAdapter.SetScale(newScale, currentInput->physPoint);
            }
            else
            {
                Vector2 position = canvasDataAdapter.GetPosition();
                Vector2 additionalPos(currentInput->wheelDelta.x, currentInput->wheelDelta.y);

                additionalPos *= canvasDataAdapter.GetViewSize();
//custom delimiter to scroll widget by little chunks of visible area
#if defined(__DAVAENGINE_MACOS__)
                //on the OS X platform wheelDelta depend on scrolling speed
                static const float wheelDelta = 0.002f;
#elif defined(__DAVAENGINE_WIN32__)
                static const float wheelDelta = 0.1f;
#endif //platform
                Vector2 newPosition = position - additionalPos * wheelDelta;
                canvasDataAdapter.SetPosition(newPosition);
            }
        }
        else
        {
            Vector2 position = canvasDataAdapter.GetPosition();
            Vector2 delta = GetSystemsManager()->GetMouseDelta();
            canvasDataAdapter.SetPosition(position - delta);
        }
    }
}

CanvasControls EditorCanvas::CreateCanvasControls()
{
    CanvasModuleData* canvasModuleData = accessor->GetGlobalContext()->GetData<CanvasModuleData>();
    return { { DISPLAY_CANVAS, canvasModuleData->canvas } };
}

void EditorCanvas::DeleteCanvasControls(const CanvasControls& canvasControls)
{
    UpdateViewsSystem* updateSystem = DAVA::GetEngineContext()->uiControlSystem->GetSystem<UpdateViewsSystem>();
    if (updateSystem != nullptr)
    {
        updateSystem->beforeRender.Disconnect(this);
    }
    CanvasModuleData* canvasModuleData = accessor->GetGlobalContext()->GetData<CanvasModuleData>();
    canvasModuleData->canvas = nullptr;
}

EditorSystemsManager::eDragState EditorCanvas::RequireNewState(DAVA::UIEvent* currentInput)
{
    using namespace DAVA;
    Function<void(UIEvent*, bool)> setMouseButtonOnInput = [this](const UIEvent* currentInput, bool value) {
        if (currentInput->mouseButton == eMouseButtons::MIDDLE)
        {
            isMouseMidButtonPressed = value;
        }
    };
    if (currentInput->device == eInputDevices::MOUSE)
    {
        if (currentInput->phase == UIEvent::Phase::BEGAN)
        {
            setMouseButtonOnInput(currentInput, true);
        }
        else if (currentInput->phase == UIEvent::Phase::ENDED)
        {
            setMouseButtonOnInput(currentInput, false);
        }
    }
    else if (currentInput->device == eInputDevices::KEYBOARD && currentInput->key == eInputElements::KB_SPACE)
    {
        //we cant use isKeyPressed here, because DAVA update keyboard state after sending Key_Up event
        //if we will press key on dock widget and hold it -> DAVA will receive only KEY_REPEAT event without KEY_DOWN
        if (currentInput->phase == UIEvent::Phase::KEY_DOWN || currentInput->phase == UIEvent::Phase::KEY_DOWN_REPEAT)
        {
            isSpacePressed = true;
        }
        else if (currentInput->phase == UIEvent::Phase::KEY_UP)
        {
            isSpacePressed = false;
        }
    }
    bool inDragScreenState = isMouseMidButtonPressed || isSpacePressed;
    return inDragScreenState ? EditorSystemsManager::DragScreen : EditorSystemsManager::NoDrag;
}

DAVA::float32 EditorCanvas::GetScaleFromWheelEvent(DAVA::int32 ticksCount) const
{
    using namespace DAVA;
    DAVA::TArc::DataContext* activeContext = accessor->GetActiveContext();
    DVASSERT(activeContext != nullptr);
    CanvasData* canvasData = activeContext->GetData<CanvasData>();
    if (ticksCount > 0)
    {
        return canvasData->GetNextScale(ticksCount);
    }
    else if (ticksCount < 0)
    {
        return canvasData->GetPreviousScale(ticksCount);
    }
    return canvasData->GetScale();
}

void EditorCanvas::OnMovableControlPositionChanged(const DAVA::Any& movableControlPosition)
{
    using namespace DAVA;
    CanvasModuleData* canvasModuleData = accessor->GetGlobalContext()->GetData<CanvasModuleData>();
    UIControl* canvas = canvasModuleData->canvas.Get();
    //right now we scale and move the same UIControl
    //because there is no reason to have another UIControl for moving only
    if (movableControlPosition.CanGet<Vector2>())
    {
        Vector2 position = movableControlPosition.Get<Vector2>();
        canvas->SetPosition(position);
    }
    else
    {
        canvas->SetPosition(Vector2(0.0f, 0.0f));
    }
}

void EditorCanvas::OnScaleChanged(const DAVA::Any& scaleValue)
{
    using namespace DAVA;
    CanvasModuleData* canvasModuleData = accessor->GetGlobalContext()->GetData<CanvasModuleData>();
    UIControl* canvas = canvasModuleData->canvas.Get();
    if (scaleValue.CanGet<float32>())
    {
        float32 scale = scaleValue.Get<float32>();
        canvas->SetScale(Vector2(scale, scale));
    }
    else
    {
        canvas->SetScale(Vector2(1.0f, 1.0f));
    }
}

void EditorCanvas::UpdateMovableControlState()
{
    OnMovableControlPositionChanged(canvasDataAdapterWrapper.GetFieldValue(CanvasDataAdapter::movableControlPositionPropertyName));
    OnScaleChanged(canvasDataAdapterWrapper.GetFieldValue(CanvasDataAdapter::scalePropertyName));
}
