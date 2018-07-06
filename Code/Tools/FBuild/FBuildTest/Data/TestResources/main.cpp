
#include "windows.h"
#include "resource.h"

// WinMain
//------------------------------------------------------------------------------
int __stdcall WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
{
    // Show the dialog
    HINSTANCE hInst = (HINSTANCE)GetModuleHandle( nullptr );
    if ( CreateDialog( hInst, MAKEINTRESOURCE( IDD_DIALOG1 ), nullptr, nullptr) )
    {
        return 1; // everything is ok - test will check for this
    }

    return 0; // failed to find resource - test will fail
}
