#import <borealis/core/logger.hpp>
#import <borealis/platforms/desktop/desktop_platform.hpp>
#import <GameController/GameController.h>
#import <UIKit/UIKit.h>

namespace brls {

ThemeVariant ios_theme() {
    if (UIScreen.mainScreen.traitCollection.userInterfaceStyle == UIUserInterfaceStyleDark)
        return ThemeVariant::DARK;
    else
        return ThemeVariant::LIGHT;
}

bool darwin_runloop(const std::function<bool()>& runLoopImpl) {
    @autoreleasepool {
        return runLoopImpl();
    }
}

uint8_t ios_battery_status() {
#if defined(IOS)
    UIDevice.currentDevice.batteryMonitoringEnabled = true;
    return UIDevice.currentDevice.batteryState;
#else
    return 0;
#endif
}

float ios_battery() {
#if defined(IOS)
    return UIDevice.currentDevice.batteryLevel;
#else
    return 0;
#endif
}
};


#if defined(IOS)
void ios_connectVirtualController() {
    if (@available(iOS 15.0, *)) {
        auto config = [[GCVirtualControllerConfiguration alloc] init];
        config.elements = [NSSet setWithObjects: 
//                           GCInputDirectionPad,
                           GCInputLeftThumbstick,
                           GCInputRightThumbstick,
                           GCInputLeftTrigger,
                           GCInputRightTrigger,
                           GCInputButtonA,
                           GCInputButtonB,
                           GCInputButtonX,
                           GCInputButtonY,
                           nil];
        auto controller = [[GCVirtualController alloc] initWithConfiguration: config];
        [controller connectWithReplyHandler: NULL];
    } else { }
}

void ios_disconnectVirtualController() {

}
#endif
