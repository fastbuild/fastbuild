// WorkerWindow
//------------------------------------------------------------------------------
#pragma once
#ifndef FBUILD_FBUILDWORKER_WORKERWINDOW_H
#define FBUILD_FBUILDWORKER_WORKERWINDOW_H

// Includes
//------------------------------------------------------------------------------
#include "Core/Containers/Singleton.h"
#include "Core/Process/Thread.h"
#include "Core/Strings/AString.h"

// system
#if defined( __WINDOWS__ )
    #include <windows.h>
    #include <shellapi.h>
#endif

// Forward Declaration
//------------------------------------------------------------------------------

// WorkerWindow
//------------------------------------------------------------------------------
class WorkerWindow : public Singleton< WorkerWindow >
{
public:
	explicit WorkerWindow( void * hInstance );
	~WorkerWindow();

	void SetStatus( const char * statusText );
	void SetWorkerState( size_t index, const AString & hostName, const AString & status );

	#if defined( __WINDOWS__ )
		HWND GetWindowHandle() const { return m_WindowHandle; }
		HMENU GetMenu() const { return m_Menu; }
	#endif

	bool WantToQuit() const { return ( m_UIState == WANT_TO_QUIT ); }
	void SetWantToQuit() { m_UIState = WANT_TO_QUIT; }
	void SetAllowQuit() { m_UIState = ALLOWED_TO_QUIT; }

	#if defined( __WINDOWS__ )
		inline HWND GetModeComboBoxHandle() const { return m_ModeComboBox; } // TODO:C make this private
	#endif

	void ShowBalloonTip( const char * msg );

private:
	static uint32_t UIUpdateThreadWrapper( void * );
	void UIUpdateThread();

	// UI created/updated in another thread to ensure responsiveness at all times
	enum UIThreadState
	{
		NOT_READY,		// UI is not yet created
		UPDATING,		// UI is created and is being updated
		WANT_TO_QUIT,	// UI wants to close down
		ALLOWED_TO_QUIT	// main thread has authorized UI shut down
	};
	volatile UIThreadState	m_UIState;
	//volatile bool			m_UIThreadQuitNotification;	// does UI want to exit (app closed etc)
	//volatile bool			m_UIThreadExitAllowed;		// main thread acknowledged exit and it's safe to shutdown thread
	Thread::ThreadHandle	m_UIThreadHandle;

	// properties of the window
	#if defined( __WINDOWS__ )
		HWND			m_WindowHandle;		// the main window
		HMENU			m_Menu;				// right-click context menu
		NOTIFYICONDATA	m_NotifyIconData;	// tray icon
		HINSTANCE		m_HInstance;		// main application instance to tie window to
	#endif

	// sub items of the window
	#if defined( __WINDOWS__ )
		HWND			m_ThreadListView;	// list showing worker thread status
		HWND			m_ModeComboBox;		// drop down showing operation mode
		HWND			m_ModeComboLabel;	// label for drop down
		HWND			m_ResourcesComboBox;// drop down showing CPU usage limits
		HWND			m_ResourcesComboLabel; // label for drop down
		HWND			m_Splitter;
		HFONT			m_Font;
	#endif

	AString			m_HostName;
};

//------------------------------------------------------------------------------
#endif // FBUILD_FBUILDWORKER_WORKERWINDOW_H
