// AsynchDlg.h : header file
//

#pragma once
#include "afxwin.h"

#include "Plot.h"
#include "afxcmn.h"
#include "TraceEvent.h"
#include "trace.h"
#include "Seeker.h"
#include "Vertical.h"

// CAsynchDlg dialog
class CAsynchDlg : public CDialog
{
// Construction
public:
        CAsynchDlg(CWnd* pParent = NULL);       // standard constructor

// Dialog Data
        enum { IDD = IDD_ASYNCH_DIALOG };

        protected:
        virtual void DoDataExchange(CDataExchange* pDX);        // DDX/DDV support


// Implementation
protected:
        HICON m_hIcon;

        // Generated message map functions
        virtual BOOL OnInitDialog();
        afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
        afx_msg void OnPaint();
        afx_msg HCURSOR OnQueryDragIcon();
        DECLARE_MESSAGE_MAP()

protected: // methods
           void updateControls();
           BOOL DoFileSave(const CString & filename);
           void updateDisplay();
           BOOL OpenDatabase();

protected: // generated methods
    afx_msg LRESULT OnCreateStepit(WPARAM, LPARAM);
    afx_msg LRESULT OnResponseStepit(WPARAM, LPARAM);
    afx_msg LRESULT OnThreadDone(WPARAM, LPARAM);
    afx_msg LRESULT OnLog(WPARAM, LPARAM);
    afx_msg void OnClose();
    virtual void OnCancel();
    virtual void OnOK();
    afx_msg void OnBnClickedCreatedata();
    afx_msg void OnBnClickedBrowse();
    afx_msg void OnBnClickedFileFlagNoBuffering();
    afx_msg void OnBnClickedFileFlagRandomAccess();
    afx_msg void OnBnClickedFileFlagSequentialScan();
    afx_msg void OnBnClickedFileFlagWriteThrough();
    afx_msg void OnEnChangeFilename();
    afx_msg void OnBnClickedGenerate();
    afx_msg void OnEnChangeNrequests();
    afx_msg void OnSize(UINT nType, int cx, int cy);
    afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    afx_msg void OnHelpAbout();
    afx_msg void OnFileSavePlot();
    afx_msg void OnFileSavePlotAs();
    afx_msg void OnFileExit();
    afx_msg void OnEditClear();
    afx_msg void OnEditClearlog();
    afx_msg void OnBnClickedScatter();
    afx_msg void OnBnClickedTemporal();
    afx_msg void OnBnClickedRun();
    afx_msg void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    afx_msg BOOL OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult);

protected: // Control variables
    CEdit c_Filename;
    CPlot c_Plot;
    CButton c_FILE_FLAG_NO_BUFFERING;
    CButton c_FILE_FLAG_WRITE_THROUGH;
    CButton c_FILE_FLAG_RANDOM_ACCESS;
    CButton c_FILE_FLAG_SEQUENTIAL_SCAN;
    CEdit c_Requests;
    CStatic x_Requests1;
    CStatic x_Requests2;
    CButton c_Generate;
    CSpinButtonCtrl c_SpinRequests;
    CTraceList c_Log;
    CStatic c_Limit;
    CProgressCtrl c_ResponseProgress;
    CProgressCtrl c_CreateProgress;
    CButton c_None;
    CButton c_ShowProgress;
    CButton c_ScatterPlot;
    CButton c_TimePlot;
    CButton c_Run;
    CStatic c_X_Label;
    CVertical c_Y_Label;

protected: // variables
   BOOL running;
   CString PlotFilename;
   BOOL initialized;
   DWORD HeaderSize;
   Seeker * seekinfo;
   DWORD RecordSize; // size of database record
   BOOL ShowProgress;
   CArray<long, long>seeks;
   
protected: // thread interface
   static UINT thread(LPVOID p);
   void RunThread();

   static UINT iocomp(LPVOID p);
   void GetNotifications();
   
protected: // Shared with thread
   HANDLE port;  // I/O Completion Port
   volatile long requests; // desired number of requests
   HANDLE file;  // handle to database file
public:
    afx_msg void OnEditCopy();
};
