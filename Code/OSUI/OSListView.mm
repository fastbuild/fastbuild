// OSListView.mm
//------------------------------------------------------------------------------

// Includes
//------------------------------------------------------------------------------
// OSUI
#include <OSUI/OSListView.h>
#include <OSUI/OSWindow.h>

// Core
#include <Core/Containers/Array.h>
#include <Core/Process/Mutex.h>
#include <Core/Strings/AStackString.h>
#include <Core/Strings/AString.h>

// System
#import <Cocoa/Cocoa.h>

// TableColumn
//------------------------------------------------------------------------------
@interface TableColumn : NSTableColumn
{
    @public Array<AString> m_Data;
}
@end
@implementation TableColumn
@end

// TableView
//------------------------------------------------------------------------------
@interface TableView : NSTableView<NSTableViewDataSource, NSTableViewDelegate>
{
    @public Mutex m_Mutex;
    @public Array<TableColumn *> m_Columns;
}
- (void)RefreshUI;
@end
@implementation TableView

// RefreshUI
//------------------------------------------------------------------------------
- (void)RefreshUI
{
    [[NSOperationQueue mainQueue] addOperationWithBlock:^{
        [self reloadData];
    }];
}

// numberOfRowsInTableView
//------------------------------------------------------------------------------
- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView
{
    TableView * table = (TableView *)tableView;
    MutexHolder mh( table->m_Mutex );
    
    // Handle no columns being added yet
    if ( m_Columns.IsEmpty() )
    {
        return 0;
    }
    
    // Size of first column controls table size
    return m_Columns[ 0 ]->m_Data.GetSize();
}

// objectValueForTableColumn
//------------------------------------------------------------------------------
- (id)tableView:(NSTableView *)tableView objectValueForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)rowIndex
{
    TableView * table = (TableView *)tableView;
    TableColumn * column = (TableColumn *)tableColumn;

    MutexHolder mh( table->m_Mutex );
    
    // Handle empty sub-items
    if ( (size_t)rowIndex >= column->m_Data.GetSize() )
    {
        return @"";
    }
    return [NSString stringWithUTF8String:column->m_Data[ rowIndex ].Get()];
}
@end

// ListViewOSX_Create
//------------------------------------------------------------------------------
void * ListViewOSX_Create( OSListView * owner, int32_t x, int32_t y, uint32_t w, uint32_t h )
{
    NSWindow * window = (__bridge NSWindow *)owner->GetParentWindow()->GetHandle();

    // create a table view and a scroll view
    CGRect tableViewFrame = CGRectMake(x, y, w, h);
    NSScrollView * scrollView = [[NSScrollView alloc] initWithFrame:tableViewFrame];
    TableView * table = [[TableView alloc] initWithFrame:tableViewFrame];
    [table setDataSource:table];
    [table setDelegate:table];

    // Configure NSScrollView
    [scrollView setHasVerticalScroller:YES]; // Always visible vertical scroll bar
    [scrollView setDocumentView:table];

    // add to window
    [window.contentView addSubview:scrollView];
    
    return (__bridge void *)table;
}

// ListViewOSX_AddColumn
//------------------------------------------------------------------------------
void ListViewOSX_AddColumn( OSListView * owner, const char * columnHeading, uint32_t columnWidth )
{
    TableView * table = (__bridge TableView *)owner->GetHandle();
    
    // Add column
    TableColumn * column = [[TableColumn alloc] init];
    {
        MutexHolder mh( table->m_Mutex );
        table->m_Columns.Append( column );
    }
    [column setWidth:columnWidth];
    [column setTitle:[NSString stringWithUTF8String:columnHeading]];
    [table addTableColumn:column];

    [table RefreshUI];
}

// ListViewOSX_AddItem
//------------------------------------------------------------------------------
void ListViewOSX_AddItem( OSListView * owner, const char * itemText )
{
    TableView * table = (__bridge TableView *)owner->GetHandle();

    // Add item
    {
        MutexHolder mh( table->m_Mutex );
        table->m_Columns[ 0 ]->m_Data.EmplaceBack( itemText );
    }
    
    [table RefreshUI];
}

// ListViewOSX_SetItemText
//------------------------------------------------------------------------------
void ListViewOSX_SetItemText( OSListView * owner, uint32_t rowIndex, uint32_t columnIndex, const char * text )
{
    TableView * table = (__bridge TableView *)owner->GetHandle();
    
    {
        MutexHolder mh( table->m_Mutex );

        // Sanity checks
        ASSERT( table->m_Columns.IsEmpty() == false ); // Must have added columns
        ASSERT( rowIndex < table->m_Columns[ 0 ]->m_Data.GetSize() ); // Column 0 controls row count

        TableColumn * column = table->m_Columns[ columnIndex ];
        
        // Handle first time addition of sub item
        if ( column->m_Data.GetSize() < ( rowIndex + 1 ) )
        {
            column->m_Data.SetSize( rowIndex + 1 );
        }
        
        // Update text
        column->m_Data[ rowIndex ] = text;
    }
        
    [table RefreshUI];
}

//------------------------------------------------------------------------------
