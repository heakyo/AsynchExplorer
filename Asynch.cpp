// Asynch.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "Asynch.h"
#include "AsynchDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAsynchApp

BEGIN_MESSAGE_MAP(CAsynchApp, CWinApp)
        ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()


// CAsynchApp construction

CAsynchApp::CAsynchApp()
{
        // TODO: add construction code here,
        // Place all significant initialization in InitInstance
}


// The one and only CAsynchApp object

CAsynchApp theApp;


// CAsynchApp initialization

BOOL CAsynchApp::InitInstance()
{
        // InitCommonControls() is required on Windows XP if an application
        // manifest specifies use of ComCtl32.dll version 6 or later to enable
        // visual styles.  Otherwise, any window creation will fail.
        InitCommonControls();

        CWinApp::InitInstance();

        AfxEnableControlContainer();

        // Standard initialization
        // If you are not using these features and wish to reduce the size
        // of your final executable, you should remove from the following
        // the specific initialization routines you do not need
        // Change the registry key under which our settings are stored
        // TODO: You should modify this string to be something appropriate
        // such as the name of your company or organization
        SetRegistryKey(_T("Local AppWizard-Generated Applications"));

        accelerator = ::LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR1));

        CAsynchDlg dlg;
        m_pMainWnd = &dlg;
        INT_PTR nResponse = dlg.DoModal();
        if (nResponse == IDOK)
        {
                // TODO: Place code here to handle when the dialog is
                //  dismissed with OK
        }
        else if (nResponse == IDCANCEL)
        {
                // TODO: Place code here to handle when the dialog is
                //  dismissed with Cancel
        }

        // Since the dialog has been closed, return FALSE so that we exit the
        //  application, rather than start the application's message pump.
        return FALSE;
}

BOOL CAsynchApp::ProcessMessageFilter(int code, LPMSG lpMsg)
    {
     if(code < 0)
         CWinApp::ProcessMessageFilter(code, lpMsg);
     if(m_pMainWnd != NULL && accelerator != NULL)
         if(::TranslateAccelerator(m_pMainWnd->m_hWnd, accelerator, lpMsg))
             return TRUE;
     return CWinApp::ProcessMessageFilter(code, lpMsg);
    }
