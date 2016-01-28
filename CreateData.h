#pragma once
#include "afxcmn.h"

#include "Seeker.h"

// CCreateData dialog

class CCreateData : public CDialog
{
        DECLARE_DYNAMIC(CCreateData)

public:
        CCreateData(CWnd* pParent = NULL);   // standard constructor
        virtual ~CCreateData();

// Dialog Data
        enum { IDD = IDD_CREATE_DATA };

protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

        DECLARE_MESSAGE_MAP()
        CSpinButtonCtrl c_SpinRecords;
        void AddBlock(CStdioFile & f, int n, Seeker * seekinfo);
protected:
    afx_msg void OnBnClickedCreateData();
    virtual BOOL OnInitDialog();
protected:
    CProgressCtrl c_Progress;
};
