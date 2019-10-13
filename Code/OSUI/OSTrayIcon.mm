// OSTrayIcon.mm
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// OSUI
#include <OSUI/OSMenu.h>
#include <OSUI/OSTrayIcon.h>

// System
#import <Cocoa/Cocoa.h>

// TrayIconOSX_Create
//------------------------------------------------------------------------------
void * TrayIconOSX_Create()
{
    // Create an icon image - TODO:OSX Use an actual image
    NSSize size;
    size.width = 16;
    size.height = 16;
    NSColor * color = [NSColor colorWithCalibratedRed:0.0f green:0.0f blue:1.0f alpha:1.0];
    NSImage * statusImage = [[NSImage alloc] initWithSize:size];
    [statusImage lockFocus];
    [color drawSwatchInRect:NSMakeRect(0, 0, size.width, size.height)];
    [statusImage unlockFocus];

    // Add status item to global status bar
    NSStatusItem * statusItem = [[NSStatusBar systemStatusBar] statusItemWithLength:NSSquareStatusItemLength];
    [statusItem.button setImage:statusImage];

    return (__bridge void *)statusItem;
}

// TrayIconOSX_SetMenu
//------------------------------------------------------------------------------
void TrayIconOSX_SetMenu( OSTrayIcon * owner, OSMenu * menu )
{
    NSStatusItem * statusItem = (__bridge NSStatusItem *)owner->GetHandle();
    NSMenu * nsMenu = (__bridge NSMenu *)menu->GetHandle();
    [statusItem setMenu:nsMenu];
}

//------------------------------------------------------------------------------
