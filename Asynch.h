// Asynch.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
        #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"           // main symbols


// CAsynchApp:
// See Asynch.cpp for the implementation of this class
//

class CAsynchApp : public CWinApp
{
public:
        CAsynchApp();

// Overrides
        public:
        virtual BOOL InitInstance();

// Implementation

        DECLARE_MESSAGE_MAP()
        virtual BOOL ProcessMessageFilter(int code, LPMSG lpMsg);
 protected:
    HACCEL accelerator;
};

extern CAsynchApp theApp;
