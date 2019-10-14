// OSLabel.mm
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// OSUI
#include <OSUI/OSLabel.h>
#include <OSUI/OSWindow.h>

// System
#import <Cocoa/Cocoa.h>

// LabelOSX_Create
//------------------------------------------------------------------------------
void * LabelOSX_Create( OSLabel * owner, int32_t x, int32_t y, uint32_t w, uint32_t h, const char * labelText )
{
    NSWindow * window = (__bridge NSWindow *)owner->GetParentWindow()->GetHandle();
   
    // Label
    CGRect textFrame = CGRectMake(x, y, w, h);
    NSTextField * textField = [[NSTextField alloc] initWithFrame:textFrame];
    [textField setStringValue:[NSString stringWithUTF8String:labelText]];
    [textField setBezeled:NO];
    [textField setDrawsBackground:NO];
    [textField setEditable:NO];
    [textField setSelectable:NO];    
    [window.contentView addSubview:textField];

    return (__bridge void *)textField;
}

//------------------------------------------------------------------------------
