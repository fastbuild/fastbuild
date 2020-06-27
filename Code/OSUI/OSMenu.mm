// OSMenu.mm
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// OSUI
#include <OSUI/OSMenu.h>
#include <OSUI/OSWindow.h>

// System
#import <Cocoa/Cocoa.h>

// MenuItem
//------------------------------------------------------------------------------
@interface MenuItem : NSMenuItem
{
    @public OSWindow * m_Window;
}
@end
@implementation MenuItem
- (IBAction)SelectorCallback:(id)sender
{
    (void)sender;
    m_Window->OnTrayIconMenuItemSelected( 0 ); // TODO:B Fix assumption that menus are used by TrayIcon only
}
@end

// MenuOSX_Create
//------------------------------------------------------------------------------
void * MenuOSX_Create( OSWindow * parentWindow )
{
    (void)parentWindow;
    NSMenu * menu = [[NSMenu alloc] init];

    // NOTE: transfer ownership to C++
    return (__bridge_retained void *)menu;
}

// MenuOSX_Destroy
//------------------------------------------------------------------------------
void MenuOSX_Destroy( OSMenu * owner )
{
    // NOTE: Transfer ownership back to ARC
    NSMenu * menu = (__bridge_transfer NSMenu *)owner->GetHandle();
    (void)menu;
}

// MenuOSX_AddItem
//------------------------------------------------------------------------------
void MenuOSX_AddItem( OSMenu * owner, const char * text )
{
    NSMenu * menu = (__bridge NSMenu *)owner->GetHandle();

    // Create menu item
    MenuItem * menuItem = [[MenuItem alloc] initWithTitle:[NSString stringWithUTF8String:text]
                                            action:@selector(SelectorCallback:)
                                            keyEquivalent:@""];
    menuItem->m_Window = owner->GetParentWindow();
    [menuItem setTarget:menuItem];

    // Add to menu
    [menu addItem:menuItem];
}

//------------------------------------------------------------------------------
