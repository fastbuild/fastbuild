// OSDropDown.mm
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// OSUI
#include <OSUI/OSDropDown.h>
#include <OSUI/OSWindow.h>

// System
#import <Cocoa/Cocoa.h>

// PopUpButton
//------------------------------------------------------------------------------
@interface PopUpButton : NSPopUpButton
{
    @public OSDropDown * m_Owner;
}
@end
@implementation PopUpButton
- (IBAction)SelectorCallback:(id)sender
{
    (void)sender;
    m_Owner->GetParentWindow()->OnDropDownSelectionChanged( m_Owner );
}
@end

// DropDownOSX_Create
//------------------------------------------------------------------------------
void * DropDownOSX_Create( OSDropDown * owner, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    // The height on OSX describes the height of the main widget, not the height
    // of the box that appears like on Windows
    h = 24; // TODO:C Is there a way to avoid this hard coded number?

    // Create drop down
    CGRect popUpFrame = CGRectMake(x, y, w, h);
    PopUpButton * popUp = [[PopUpButton alloc] initWithFrame:popUpFrame];
    popUp->m_Owner = owner;

    // Hookup callbacks
    [popUp setAction:@selector(SelectorCallback:)];
    [popUp setTarget:popUp];

    // Add to the window
    NSWindow * window = (__bridge NSWindow *)owner->GetParentWindow()->GetHandle();
    [window.contentView addSubview:popUp];

    return (__bridge void *)popUp;
}

// DropDownOSX_AddItem
//------------------------------------------------------------------------------
void DropDownOSX_AddItem( OSDropDown * owner, const char * itemText )
{
    PopUpButton * popUp = (__bridge PopUpButton *)owner->GetHandle();
    [popUp addItemWithTitle:[NSString stringWithUTF8String:itemText]];
}

// DropDownOSX_SetSelectedItem
//------------------------------------------------------------------------------
void DropDownOSX_SetSelectedItem( OSDropDown * owner, uint32_t index )
{
    PopUpButton * popUp = (__bridge PopUpButton *)owner->GetHandle();
    [popUp selectItemAtIndex:index];
}

// DropDownOSX_GetSelectedItem
//------------------------------------------------------------------------------
uint32_t DropDownOSX_GetSelectedItem( const OSDropDown * owner )
{
    PopUpButton * popUp = (__bridge PopUpButton *)owner->GetHandle();
    return popUp.indexOfSelectedItem;
}

// DropDownOSX_SetEnabled
//------------------------------------------------------------------------------
void DropDownOSX_SetEnabled( const OSDropDown * owner, bool enabled )
{
    PopUpButton * popUp = (__bridge PopUpButton *)owner->GetHandle();
    popUp.enabled = enabled ? YES : NO;
}

//------------------------------------------------------------------------------
