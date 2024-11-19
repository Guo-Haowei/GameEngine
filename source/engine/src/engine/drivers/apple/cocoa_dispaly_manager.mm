#include "cocoa_display_manager.h"

#import <Cocoa/Cocoa.h>

namespace my {

class NSWindowWrapperImpl {
public:
    NSWindowWrapperImpl() {
        // Create the NSWindow instance
        @autoreleasepool {
            window_ = [[NSWindow alloc] initWithContentRect:NSMakeRect(100, 100, 800, 600)
                                                   styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskResizable | NSWindowStyleMaskClosable)
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
            [window_ setTitle:@"Wrapper Window"];
        }
    }

    ~NSWindowWrapperImpl() {
        @autoreleasepool {
            [window_ release];
        }
    }

    void show() {
        @autoreleasepool {
            [window_ makeKeyAndOrderFront:nil];
        }
    }

    void hide() {
        @autoreleasepool {
            [window_ orderOut:nil];
        }
    }

private:
    NSWindow* window_; // Pointer to the NSWindow
};

void CocoaDisplayManager::Finalize() {
}

bool CocoaDisplayManager::ShouldClose() {
    return true;
}

std::tuple<int, int> CocoaDisplayManager::GetWindowSize() {
    return std::make_tuple(0, 0);
}

std::tuple<int, int> CocoaDisplayManager::GetWindowPos() {
    return std::make_tuple(0, 0);
}

void CocoaDisplayManager::NewFrame() {

}

void CocoaDisplayManager::Present() {

}

bool CocoaDisplayManager::InitializeWindow() {
//    NSWindow *window = [[[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 640, 480)
//                                     styleMask:NSWindowStyleMaskTitled |
//                                               NSWindowStyleMaskResizable |
//                                               NSWindowStyleMaskClosable |
//                                               NSWindowStyleMaskMiniaturizable
//                                       backing:NSBackingStoreBuffered
//                                         defer:NO] autorelease];
//    [window setTitle:@"OpenGL Sky"];
//    [window cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
//    [window setMinSize:NSMakeSize(300, 200)];
//    [window setAcceptsMouseMovedEvents:YES];
//    [window makeKeyAndOrderFront:nil];
//    [window center];
    return false;
}

void CocoaDisplayManager::InitializeKeyMapping() {

}

}
