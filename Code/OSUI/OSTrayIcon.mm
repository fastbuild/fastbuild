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
void * TrayIconOSX_Create( void * iconData, size_t iconDataSize )
{
    // Create NSImage from data
    NSImage * statusImage = [[NSImage alloc] initWithData:[NSData dataWithBytes:iconData length:iconDataSize]];

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
