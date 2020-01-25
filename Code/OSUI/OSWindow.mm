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
@interface Window : NSWindow<NSApplicationDelegate, NSWindowDelegate>
    @property OSWindow * m_OSWindow;
@end

@implementation Window
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
    _m_OSWindow->OnMinimize();
}

// windowShouldClose
//------------------------------------------------------------------------------
- (BOOL)windowShouldClose:(id)sender {
    (void)sender;

    const bool closeHandled = _m_OSWindow->OnClose();
    return closeHandled ? NO : YES; // Swallow the close if handled by the callback
}
@end

// WindowOSX_Create
//------------------------------------------------------------------------------
void * WindowOSX_Create( OSWindow * owner, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    id applicationName = [[NSProcessInfo processInfo] processName];
    Window * window = [[Window alloc]
            initWithContentRect:NSMakeRect(0, 0, w, h)
            styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskMiniaturizable
            backing:NSBackingStoreBuffered
            defer:NO
            ];
    [window cascadeTopLeftFromPoint:NSMakePoint(x, y)];
    [window setTitle:applicationName];
    [window makeKeyAndOrderFront:nil];
    window.m_OSWindow = owner;
    
    FlippedView * flippedView = [[FlippedView alloc] init];
    [window setContentView:flippedView];

    NSApplication * application = [NSApplication sharedApplication];
    [application setDelegate:window];
    
    // NOTE: transfer ownership to C++
    return (__bridge_retained void *)window;
}

// WindowOSX_Destroy
//------------------------------------------------------------------------------
void WindowOSX_Destroy( OSWindow * owner )
{
    // NOTE: Transfer ownership back to ARC
    Window * window = (__bridge_transfer Window *)owner->GetHandle();
    (void)window;
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

    // Send a dummy event to force the event loop to stop
    [NSApp postEvent:[NSEvent otherEventWithType:NSEventTypeApplicationDefined
                              location:NSMakePoint( 0, 0 )
                              modifierFlags:0
                              timestamp:0
                              windowNumber:0
                              context:NULL
                              subtype:0
                              data1:0
                              data2:0]
           atStart:NO];
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
    Window * window = (__bridge Window *)owner->GetHandle();
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
