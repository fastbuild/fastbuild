// OSWindow.mm
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
#include "OSWindow.h"

// Core
#include "OSUI/OSWindow.h"

// System
#import <Cocoa/Cocoa.h>

// FlippedView
//------------------------------------------------------------------------------
// Make the coordinate system of our Window origin be top-left
@interface FlippedView: NSView
- (BOOL) isFlipped;
@end

@implementation FlippedView
- (BOOL) isFlipped
{
    return YES;
}
@end

// AppDelegate
//------------------------------------------------------------------------------
@interface AppDelegate : NSObject<NSApplicationDelegate, NSWindowDelegate>
    @property OSWindow * mOSWindow;
@end

@implementation AppDelegate
// applicationDidFinishLaunching
//------------------------------------------------------------------------------
- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    (void)notification;

    // Install custom quit event handler
    NSAppleEventManager* appleEventManager = [NSAppleEventManager sharedAppleEventManager];
    [appleEventManager setEventHandler:self andSelector:@selector(handleQuitEvent:withReplyEvent:) forEventClass:kCoreEventClass andEventID:kAEQuitApplication];
}

- (void)windowWillMiniaturize:(NSNotification *)notification
{
    (void)notification;
    _mOSWindow->OnMinimize();
}

// windowShouldClose
//------------------------------------------------------------------------------
- (BOOL)windowShouldClose:(id)sender {
    (void)sender;

    const bool closeHandled = _mOSWindow->OnClose();
    return closeHandled ? NO : YES; // Swallow the close if handled by the callback
}
@end

// WindowOSX_Create
//------------------------------------------------------------------------------
void * WindowOSX_Create( OSWindow * owner, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    AppDelegate * delegate = [[AppDelegate alloc] init];
    delegate.mOSWindow = owner;

    NSApplication * application = [NSApplication sharedApplication];
    [application setDelegate:delegate];

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    id applicationName = [[NSProcessInfo processInfo] processName];
    NSWindow * window = [[NSWindow alloc]
            initWithContentRect:NSMakeRect(0, 0, w, h)
            styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable
            backing:NSBackingStoreBuffered
            defer:NO
            ];
    [window setDelegate:delegate];
    [window cascadeTopLeftFromPoint:NSMakePoint(x, y)];
    [window setTitle:applicationName];
    [window makeKeyAndOrderFront:nil];
    
    FlippedView * flippedView = [[FlippedView alloc] init];
    [window setContentView:flippedView];

    return (__bridge void *)window;
}

// WindowOSX_MessageLoop
//------------------------------------------------------------------------------
void WindowOSX_MessageLoop()
{
    [NSApp activateIgnoringOtherApps:YES];
    [NSApp run];      
}

// WindowOSX_StopMessageLoop
//------------------------------------------------------------------------------
void WindowOSX_StopMessageLoop()
{
    [NSApp stop:nil];
}

// WindowOSX_SetMinimized
//------------------------------------------------------------------------------
void WindowOSX_SetMinimized( bool minimized )
{
    // TODO:C This assumes the app has one main window
    if (minimized)
    {
        [NSApp hide:nil];
    }
    else
    {
        [NSApp unhide:nil];
    }
}

// WindowOSX_SetTitle
//------------------------------------------------------------------------------
void WindowOSX_SetTitle( OSWindow * owner, const char * title )
{
    NSWindow * window = (__bridge NSWindow *)owner->GetHandle();
    NSString * string = [NSString stringWithUTF8String:title];
    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
        [window setTitle:string];
    }];
}

// WindowOSX_GetPrimaryScreenWidth
//------------------------------------------------------------------------------
uint32_t WindowOSX_GetPrimaryScreenWidth()
{
    NSRect screenRect;
    NSArray * screenArray = [NSScreen screens];
    NSScreen * screen = [screenArray objectAtIndex: 0];
    screenRect = [screen visibleFrame];
    return screenRect.size.width;
}

//------------------------------------------------------------------------------
