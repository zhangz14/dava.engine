#pragma once

#include "Base/BaseTypes.h"

#if defined(__DAVAENGINE_IPHONE__)

#include "Engine/iOS/UIApplicationDelegateListener.h"
namespace DAVA
{
class LocalNotificationController;

namespace Private
{
struct NativeDelegate : public UIApplicationDelegateListener
{
    NativeDelegate(LocalNotificationController& controller);
    virtual ~NativeDelegate();
    void didFinishLaunchingWithOptions(UIApplication* application, NSDictionary* launchOptions) override;
    void applicationDidBecomeActive() override;
    void didReceiveLocalNotification(UILocalNotification* notification) override;
private:
    LocalNotificationController& localNotificationController;
};
} // namespace Private
} // namespace DAVA
#endif // __DAVAENGINE_MACOS__
