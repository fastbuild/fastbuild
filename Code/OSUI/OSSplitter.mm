// OSSplitter.mm
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// OSUI
#include <OSUI/OSSplitter.h>
#include <OSUI/OSWindow.h>

// System
#import <Cocoa/Cocoa.h>

// SplitterOSX_Create
//------------------------------------------------------------------------------
void * SplitterOSX_Create( OSSplitter * owner, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    NSWindow * window = (__bridge NSWindow *)owner->GetParentWindow()->GetHandle();
   
    // Label
    CGRect boxFrame = CGRectMake(x, y, w, h);
    NSBox * box = [[NSBox alloc] initWithFrame:boxFrame];
    [window.contentView addSubview:box];

    return (__bridge void *)box;
}

//------------------------------------------------------------------------------
