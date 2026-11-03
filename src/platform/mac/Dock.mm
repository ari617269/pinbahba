#include "platform/Dock.h"

#import <AppKit/AppKit.h>

namespace Platform {

void setDockIconHidden(bool hidden) {
    [NSApp setActivationPolicy:hidden ? NSApplicationActivationPolicyAccessory
                                      : NSApplicationActivationPolicyRegular];
}

}  // namespace Platform
