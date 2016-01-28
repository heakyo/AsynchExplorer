// AsynchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "AsynchDlg.h"
#include "regvars.h"
#include "CreateData.h"
#include "ErrorString.h"
#include "rand32.h"
#include "Seeker.h"
#include "Header.h"
#include "log.h"
#include "ToClip.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define REQUESTS_MULTIPLIER 100

static UINT UWM_LOG = ::RegisterWindowMessage(UWM_LOG_MSG);


/****************************************************************************
*                               UWM_THREAD_DONE
* Inputs:
*       WPARAM: ignored
*       LPARAM: ignored
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Notifies that the receiver thread has finished
****************************************************************************/

static UINT UWM_THREAD_DONE  = ::RegisterWindowMessage(_T("UWM_THREAD_DONE-{E4637467-B550-420d-BBF8-3C6F1E206226}"));


/****************************************************************************
*                                 UWM_CREATE_STEPIT
*                                UWM_RESPONSE_STEPIT
* Inputs:
*       WPARAM: ignored
*       LPARAM: ignored
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Steps the progress bar
****************************************************************************/

static UINT UWM_CREATE_STEPIT   = ::RegisterWindowMessage(_T("UWM_CREATE_STEPIT-{E4637467-B550-420d-BBF8-3C6F1E206226}"));
static UINT UWM_RESPONSE_STEPIT = ::RegisterWindowMessage(_T("UWM_RESPONSE_STEPIT-{E4637467-B550-420d-BBF8-3C6F1E206226}"));



class OVL : public OVERLAPPED {
    public:
       OVL(int size) { buffer = (char *)::VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE); ASSERT(buffer != NULL); }
       // Cannot put virtual methods in this class!!!
       /* non-virtual */ ~OVL() { VERIFY(::VirtualFree(buffer, 0, MEM_RELEASE)); }
       int sequence;
       void SetOffset(LONGLONG pos) { LARGE_INTEGER L; L.QuadPart = pos; Offset = L.LowPart; OffsetHigh = L.HighPart; }
       char * buffer;
       void SetStart() { QueryPerformanceCounter(&start); }
       void SetStop()  { QueryPerformanceCounter(&stop);  }
       double dt() { LONGLONG delta = stop.QuadPart - start.QuadPart;
                     LARGE_INTEGER freq;
                     QueryPerformanceFrequency(&freq);
                     double ticks = (double)delta;
                     double ticks_per_sec = (double)freq.QuadPart;
                     return ticks/ticks_per_sec; }
    protected:
       LARGE_INTEGER start;
       LARGE_INTEGER stop;
    };

// CAboutDlg dialog used for App About
class CAboutDlg : public CDialog
{
public:
        CAboutDlg();

// Dialog Data
        enum { IDD = IDD_ABOUTBOX };

        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
        DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CAsynchDlg dialog



CAsynchDlg::CAsynchDlg(CWnd* pParent /*=NULL*/)
        : CDialog(CAsynchDlg::IDD, pParent)
   {
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    initialized = FALSE;
    running = FALSE;
    seekinfo = NULL;
   }

void CAsynchDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
DDX_Control(pDX, IDC_FILENAME, c_Filename);
DDX_Control(pDX, IDC_PLOT, c_Plot);
DDX_Control(pDX, IDC_FILE_FLAG_NO_BUFFERING, c_FILE_FLAG_NO_BUFFERING);
DDX_Control(pDX, IDC_FILE_FLAG_WRITE_THROUGH, c_FILE_FLAG_WRITE_THROUGH);
DDX_Control(pDX, IDC_FILE_FLAG_RANDOM_ACCESS, c_FILE_FLAG_RANDOM_ACCESS);
DDX_Control(pDX, IDC_FILE_FLAG_SEQUENTIAL_SCAN, c_FILE_FLAG_SEQUENTIAL_SCAN);
DDX_Control(pDX, IDC_NREQUESTS, c_Requests);
DDX_Control(pDX, IDC_REQUESTS_CAPTION_1, x_Requests1);
DDX_Control(pDX, IDC_REQUESTS_CAPTION_2, x_Requests2);
DDX_Control(pDX, IDC_GENERATE, c_Generate);
DDX_Control(pDX, IDC_SPIN_REQUESTS, c_SpinRequests);
DDX_Control(pDX, IDC_LOG, c_Log);
DDX_Control(pDX, IDC_FRAME, c_Limit);
DDX_Control(pDX, IDC_RESPONSE_PROGRESS, c_ResponseProgress);
DDX_Control(pDX, IDC_CREATE_PROGRESS, c_CreateProgress);
DDX_Control(pDX, IDC_FILE_FLAG_SUGGESTION_NONE, c_None);
DDX_Control(pDX, IDC_SHOW_PROGRESS, c_ShowProgress);
DDX_Control(pDX, IDC_SCATTER, c_ScatterPlot);
DDX_Control(pDX, IDC_TEMPORAL, c_TimePlot);
DDX_Control(pDX, IDC_RUN, c_Run);
DDX_Control(pDX, IDC_X_CAPTION, c_X_Label);
DDX_Control(pDX, IDC_Y_CAPTION, c_Y_Label);
}

BEGIN_MESSAGE_MAP(CAsynchDlg, CDialog)
        ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnToolTipNotify)
        ON_REGISTERED_MESSAGE(UWM_LOG, OnLog)
        ON_WM_SYSCOMMAND()
        ON_WM_PAINT()
        ON_WM_QUERYDRAGICON()
        ON_REGISTERED_MESSAGE(UWM_RESPONSE_STEPIT, OnResponseStepit)
        ON_REGISTERED_MESSAGE(UWM_CREATE_STEPIT, OnCreateStepit)
        ON_REGISTERED_MESSAGE(UWM_THREAD_DONE, OnThreadDone)
        ON_WM_CLOSE()
        ON_BN_CLICKED(IDC_CREATEDATA, OnBnClickedCreatedata)
        ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
        ON_BN_CLICKED(IDC_FILE_FLAG_NO_BUFFERING, OnBnClickedFileFlagNoBuffering)
        ON_BN_CLICKED(IDC_FILE_FLAG_RANDOM_ACCESS, OnBnClickedFileFlagRandomAccess)
        ON_BN_CLICKED(IDC_FILE_FLAG_SEQUENTIAL_SCAN, OnBnClickedFileFlagSequentialScan)
        ON_BN_CLICKED(IDC_FILE_FLAG_WRITE_THROUGH, OnBnClickedFileFlagWriteThrough)
        ON_EN_CHANGE(IDC_FILENAME, OnEnChangeFilename)
        ON_BN_CLICKED(IDC_GENERATE, OnBnClickedGenerate)
        ON_EN_CHANGE(IDC_NREQUESTS, OnEnChangeNrequests)
        ON_WM_SIZE()
        ON_WM_GETMINMAXINFO()
        ON_COMMAND(ID_HELP_ABOUT, OnHelpAbout)
        ON_COMMAND(ID_FILE_SAVE_PLOT, OnFileSavePlot)
        ON_COMMAND(ID_FILE_SAVE_PLOT_AS, OnFileSavePlotAs)
        ON_COMMAND(ID_FILE_EXIT, OnFileExit)
        ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
        ON_COMMAND(ID_EDIT_CLEARLOG, OnEditClearlog)
        ON_BN_CLICKED(IDC_SCATTER, OnBnClickedScatter)
        ON_BN_CLICKED(IDC_TEMPORAL, OnBnClickedTemporal)
        ON_BN_CLICKED(IDC_RUN, OnBnClickedRun)
        ON_WM_INITMENUPOPUP()
        ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
END_MESSAGE_MAP()


// CAsynchDlg message handlers

/****************************************************************************
*                          CAsynchDlg::OnInirDialog
* Result: BOOL
*       TRUE, always
* Effect: 
*       Runs the program
****************************************************************************/

BOOL CAsynchDlg::OnInitDialog()
{
        CDialog::OnInitDialog();

        // Add "About..." menu item to system menu.

        // IDM_ABOUTBOX must be in the system command range.
        ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
        ASSERT(IDM_ABOUTBOX < 0xF000);

        CMenu* pSysMenu = GetSystemMenu(FALSE);
        if (pSysMenu != NULL)
        {
                CString strAboutMenu;
                strAboutMenu.LoadString(IDS_ABOUTBOX);
                if (!strAboutMenu.IsEmpty())
                {
                        pSysMenu->AppendMenu(MF_SEPARATOR);
                        pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
                }
        }

        // Set the icon for this dialog.  The framework does this automatically
        //  when the application's main window is not a dialog
        SetIcon(m_hIcon, TRUE);                 // Set big icon
        SetIcon(m_hIcon, FALSE);                // Set small icon

        // TODO: Add extra initialization here
        
        RegistryString database(IDS_REGISTRY_DATABASE);
        VERIFY(database.load());
        c_Filename.SetWindowText(database.value);
        c_SpinRequests.SetRange(1, 100);
        RegistryInt samples(IDS_REGISTRY_SAMPLES);
        samples.load(10);
        c_SpinRequests.SetPos(samples.value);

        CFont * f = c_Generate.GetFont();
        c_Plot.SetFont(f);

        CheckRadioButton(IDC_FILE_FLAG_SUGGESTION_NONE, IDC_FILE_FLAG_RANDOM_ACCESS, IDC_FILE_FLAG_SUGGESTION_NONE);
        CheckRadioButton(IDC_SCATTER, IDC_TEMPORAL, IDC_SCATTER);


        RegistryInt ffnb(IDS_REGISTRY_FILE_FLAG_NO_BUFFERING);
        ffnb.load(0);
        c_FILE_FLAG_NO_BUFFERING.SetCheck(ffnb.value ? BST_CHECKED : BST_UNCHECKED);

        RegistryInt ffwt(IDS_REGISTRY_FILE_FLAG_WRITE_THROUGH);
        ffwt.load(0);
        c_FILE_FLAG_WRITE_THROUGH.SetCheck(ffwt.value ? BST_CHECKED : BST_UNCHECKED);


        initialized = TRUE;
        updateControls();
        updateDisplay();
        
        EnableToolTips();
        
        return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAsynchDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
        if ((nID & 0xFFF0) == IDM_ABOUTBOX)
        {
                CAboutDlg dlgAbout;
                dlgAbout.DoModal();
        }
        else
        {
                CDialog::OnSysCommand(nID, lParam);
        }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CAsynchDlg::OnPaint() 
{
        if (IsIconic())
        {
                CPaintDC dc(this); // device context for painting

                SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

                // Center icon in client rectangle
                int cxIcon = GetSystemMetrics(SM_CXICON);
                int cyIcon = GetSystemMetrics(SM_CYICON);
                CRect rect;
                GetClientRect(&rect);
                int x = (rect.Width() - cxIcon + 1) / 2;
                int y = (rect.Height() - cyIcon + 1) / 2;

                // Draw the icon
                dc.DrawIcon(x, y, m_hIcon);
        }
        else
        {
                CDialog::OnPaint();
        }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CAsynchDlg::OnQueryDragIcon()
{
        return static_cast<HCURSOR>(m_hIcon);
}

/****************************************************************************
*                             CAsynchDlg::OnClose
* Result: void
*       
* Effect: 
*       Forces the dialog to close by calling OnOK
****************************************************************************/

void CAsynchDlg::OnClose()
    {
     CDialog::OnOK();
    }

/****************************************************************************
*                            CAsynchDlg::OnCancel
* Result: void
*       
* Effect: 
*       Does nothing.  Avoids having the application close on <Escape>
****************************************************************************/

void CAsynchDlg::OnCancel()
    {
    }

/****************************************************************************
*                              CAsynchDlg::OnOK
* Result: void
*       
* Effect: 
*       Does nothing.  Avoids having the application close on <Enter>
****************************************************************************/

void CAsynchDlg::OnOK()
    {
    }

/****************************************************************************
*                      CAsynchDlg::OnBnClickedCreatedata
* Result: void
*       
* Effect: 
*       Creates a data file
****************************************************************************/

void CAsynchDlg::OnBnClickedCreatedata()
    {
     CCreateData dlg;
     dlg.DoModal();
     RegistryString database(IDS_REGISTRY_DATABASE);
     VERIFY(database.load());
     c_Filename.SetWindowText(database.value);
    }

/****************************************************************************
*                        CAsynchDlg::ONBnClickedBrowse
* Result: void
*       
* Effect: 
*       Finds the database file
****************************************************************************/

void CAsynchDlg::OnBnClickedBrowse()
   { 
    CString s;
    c_Filename.GetWindowText(s);
    
    CString filter;
    filter.LoadString(IDS_DATABASE_FILTER);

    CFileDialog dlg(TRUE, _T(".dat"), s, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);
                    
    if(dlg.DoModal() != IDOK)
       return;  // rejected

    s = dlg.GetPathName();

    s.Trim();

    if(s.IsEmpty())
       return; // ignore blank filename
    
    c_Filename.SetWindowText(s);
    RegistryString database(IDS_REGISTRY_DATABASE);
    database.value = s;
    VERIFY(database.store());
   }

/****************************************************************************
*                 CAsynchDlg::OnBnClickedFileFlagNoBuffering
* Result: void
*       
* Effect: 
*       Updates the controls
****************************************************************************/

void CAsynchDlg::OnBnClickedFileFlagNoBuffering()
   {
    RegistryInt ffnb(IDS_REGISTRY_FILE_FLAG_NO_BUFFERING);
    ffnb.value = c_FILE_FLAG_NO_BUFFERING.GetCheck() == BST_CHECKED ? TRUE : FALSE;
    ffnb.store();
    updateControls();
   }

/****************************************************************************
*                 CAsynchDlg::OnBnClickedFileFlagRandomAccess
* Result: void
*       
* Effect: 
*       Updates the controls
****************************************************************************/

void CAsynchDlg::OnBnClickedFileFlagRandomAccess()
   {
    updateControls();
   }

/****************************************************************************
*                CAsynchDlg::OnBnClickedFileFlagSequentialScan
* Result: void
*       
* Effect: 
*       ¶
****************************************************************************/

void CAsynchDlg::OnBnClickedFileFlagSequentialScan()
   {
    updateControls();
   }

/****************************************************************************
*                 CAsynchDlg::OnBnClickedFileFlagWriteThrough
* Result: void
*       
* Effect: 
*       Updates the controls
****************************************************************************/

void CAsynchDlg::OnBnClickedFileFlagWriteThrough()
   {
    RegistryInt ffwt(IDS_REGISTRY_FILE_FLAG_WRITE_THROUGH);
    ffwt.value = c_FILE_FLAG_WRITE_THROUGH.GetCheck() == BST_CHECKED ? TRUE : FALSE;
    ffwt.store();
    updateControls();
   }

/****************************************************************************
*                       CAsynchDlg::OnEnChangeFilename
* Result: void
*       
* Effect: 
*       Updates the controls
****************************************************************************/

void CAsynchDlg::OnEnChangeFilename()
   {
    updateControls();
   }

/****************************************************************************
*                            CAsynchDlg::RunThread
* Result: void
*       
* Effect: 
*       Initiates a sequence of operations
****************************************************************************/

void CAsynchDlg::RunThread()
   {
    int delta = 0;

    //================================================================
    // Having computed all the seek addresses, now issue a sequence
    // of ReadFile requests based on those random addresses
    //================================================================

    c_Log.AddMessage(new TraceComment(TraceEvent::None, IDS_STARTING_REQUESTS));

    for(int i = 0; i < requests; i++)
       { /* generate requests */
        OVL * ovl = new OVL(seekinfo->GetBytesPerRecord());
        ovl->sequence = i;
        ovl->hEvent = NULL;

        long pos = seeks[i];

        DWORD bytesRead;

        ovl->SetOffset(seekinfo->GetSeekOffset(pos));

        ovl->SetStart();
        BOOL b = ::ReadFile(file, ovl->buffer, seekinfo->GetBytesPerRecord(), &bytesRead, ovl);

        if(!b)
            { /* read failed */
             DWORD err = ::GetLastError();
             if(err != ERROR_IO_PENDING)
                { /* real error */            
                 c_Log.AddMessage(new TraceFormatError(TraceEvent::None, IDS_QUEUED_READ_FAILED, i, seeks[i]));
                 c_Log.AddMessage(new TraceFormatMessage(TraceEvent::None, err));
                 delete ovl; // since no completion, need to delete here
                 delta++;
                } /* real error */
            } /* read failed */

        if(ShowProgress)
           PostMessage(UWM_CREATE_STEPIT);

       } /* generate requests */

    //================================================================
    // Compute the actual number of requests, in case there were
    // any failures
    //================================================================
    
    InterlockedExchangeAdd(&requests, - delta);

    //================================================================
    // Post the completion messages
    //================================================================
    
    c_Log.AddMessage(new TraceFormatComment(TraceEvent::None, IDS_ENDING_REQUESTS, requests));
    if(delta > 0)
      c_Log.AddMessage(new TraceFormatError(TraceEvent::None, IDS_TOTAL_QUEUED_FAILURSE, delta));
    delete seekinfo;
    seekinfo = NULL;
   } // CAsynchDlg::RunThread

/****************************************************************************
*                             CAsynchDlg::thread
* Inputs:
*       LPVOID p: (LPVOID)(CAsynchDlg *)this
* Result: UINT
*       Logically void, 0, always
* Effect: 
*       Initiates the seek operations (calls RunThread to work in C++ space)
****************************************************************************/

/* static */ UINT CAsynchDlg::thread(LPVOID p)
    {
     ((CAsynchDlg *)p)->RunThread();
     return 0;
    } // CAsynchDlg::thread

/****************************************************************************
*                        CAsynchDlg::GetNotifications
* Result: void
*       
* Effect: 
*       Handles I/O completion requests.  This function is run in the
*       context of the (one-and-only) receiving thread.
*       Note that this code would require tweaking to run in multiple
*       sever threads because it assumes that it has sole control of the
*       file handle
****************************************************************************/

void CAsynchDlg::GetNotifications()
    {

     // Note that we do not start the generator thread until the consumer
     // thread is actually running
     AfxBeginThread(thread, this);
     
     for(int i = 0; i < requests; i++)
        { /* read requests */
         DWORD bytesRead;
         ULONG_PTR key;
         OVL * ovl;
         
         BOOL b = ::GetQueuedCompletionStatus(port, &bytesRead, &key, (LPOVERLAPPED*)&ovl, INFINITE);
         if(!b)
            { /* error */
             DWORD err = ::GetLastError();
             c_Log.AddMessage(new TraceError(TraceEvent::None, IDS_COMPLETION_ERROR));
             c_Log.AddMessage(new TraceFormatMessage(TraceEvent::None, err));
             continue;
            } /* error */
         else
            { /* success */
             ovl->SetStop();
             c_Plot.data.Times[i] = ovl->dt();

             LARGE_INTEGER L;
             L.LowPart = ovl->Offset;
             L.HighPart = ovl->OffsetHigh;
             
             c_Plot.data.SeekPositions[i] = L.QuadPart;

             c_Plot.data.Sequence[i] = ovl->sequence;
             if(ShowProgress)
                PostMessage(UWM_RESPONSE_STEPIT);

             delete ovl;
            } /* success */
        } /* read requests */

     ::CloseHandle(file);
     file = NULL;
     c_Log.AddMessage(new TraceComment(TraceEvent::None, IDS_FILE_CLOSED));

     ::CloseHandle(port);
     port = NULL;
     PostMessage(UWM_THREAD_DONE);
    } // CAsynchDlg::GetNotifications

/****************************************************************************
*                             CAsynchDlg::iocomp
* Inputs:
*       LPVOID p: (LPVOID)(CAsynDlg *)this
* Result: UINT
*       Logically void, 0, always
* Effect: 
*       Invokes the I/O Completion thread in C++ space
****************************************************************************/

/* static */ UINT CAsynchDlg::iocomp(LPVOID p)
    {
     ((CAsynchDlg *)p)->GetNotifications();
     return 0;
    } // CAsynchDlg::iocomp

/****************************************************************************
*                          CAsynchDlg::OpenDatabase
* Result: BOOL
*       TRUE if successful, FALSE if error
* Effect: 
*       Opens the file and initializes the Seeker structure
****************************************************************************/

BOOL CAsynchDlg::OpenDatabase()
    {
     CString filename;
     c_Filename.GetWindowText(filename);
     filename.Trim();
     if(filename.IsEmpty())
        { /* error */
         return FALSE; // shouldn't be here, control should have been disabled
        } /* error */

     //================================================================
     // Compute the CreateFile flags based on the switch settings
     //================================================================
     DWORD flags = FILE_FLAG_OVERLAPPED;

#define SetFlag(flag) flags |= (c_##flag.GetCheck() == BST_CHECKED) ? flag : 0
     SetFlag(FILE_FLAG_NO_BUFFERING);
     SetFlag(FILE_FLAG_RANDOM_ACCESS);
     SetFlag(FILE_FLAG_SEQUENTIAL_SCAN);
     SetFlag(FILE_FLAG_WRITE_THROUGH);

    //================================================================
    // Open the file
    //================================================================
     file = ::CreateFile(filename,
                         GENERIC_READ,  // desired access
                         0,             // exclusive access
                         NULL,          // security
                         OPEN_EXISTING, // use existing
                         flags,         // 
                         NULL);         // template

     if(file == INVALID_HANDLE_VALUE)
        { /* failed */
         DWORD err = ::GetLastError();
         c_Log.AddMessage(new TraceError(TraceEvent::None, filename));
         c_Log.AddMessage(new TraceFormatMessage(TraceEvent::None, err));
         ::CloseHandle(file);
         file = NULL;
         return FALSE;
        } /* failed */

    //================================================================
    // Create a Seeker object that provides the appropriate
    // conversion functions for this particular file, based on its
    // file size
    //================================================================

     seekinfo = new Seeker(filename);

     ASSERT(seekinfo->GetBytesPerSector() >= sizeof(Header)); // this should always be true
     if(seekinfo->GetBytesPerSector() < sizeof(Header))
        { /* wrong size */
         ::CloseHandle(file);
         file = NULL;
         return FALSE;
        } /* wrong size */

    //================================================================
    // It was simpler to use VirtualAlloc instead of a complicated
    // algorithm that computed a modulo-sector-size memory address
    // This technique is suggested in the Microsoft documentation to
    // deal with this problem
    //================================================================
     Header * hdr = (Header *)::VirtualAlloc(NULL, seekinfo->GetBytesPerSector(), MEM_COMMIT, PAGE_READWRITE);

     if(hdr == NULL)
        { /* allocation failed */
         DWORD err = ::GetLastError();
         c_Log.AddMessage(new TraceError(TraceEvent::None, IDS_HEADER_ALLOC_FAILED));
         c_Log.AddMessage(new TraceFormatMessage(TraceEvent::None, err));
         ::CloseHandle(file);
         file = NULL;
         return FALSE;
        } /* allocation failed */

    //================================================================
    // Read the file header.  It says in the header how many records
    // are in the file.  From this we can compute the permitted
    // range of seek addresses
    //================================================================

     DWORD bytesRead;

     OVL ovl(seekinfo->GetBytesPerSector());
     ovl.SetOffset(0);
     ovl.hEvent = NULL;

     BOOL b = ::ReadFile(file, hdr, seekinfo->GetBytesPerSector(), &bytesRead, &ovl);
     if(!b)
        { /* wait for result */
         DWORD err = ::GetLastError();
         if(err != ERROR_IO_PENDING)
            { /* some other error */
             c_Log.AddMessage(new TraceError(TraceEvent::None, IDS_HEADER_READ_FAILED));
             c_Log.AddMessage(new TraceFormatMessage(TraceEvent::None, err));
             ::CloseHandle(file);
             file = NULL;
             VERIFY(::VirtualFree(hdr, 0, MEM_RELEASE));
             return FALSE;
            } /* some other error */

        // Because we want to do pseudo-sycnhronous I/O here, we just
        // wait for the GetOverlappedResult

         b = ::GetOverlappedResult(file, &ovl, &bytesRead, TRUE);

         if(!b)
            { /* header read failed */
             DWORD err = ::GetLastError();
             c_Log.AddMessage(new TraceFormatMessage(TraceEvent::None, err));
             ::CloseHandle(file);
             file = NULL;
             VERIFY(::VirtualFree(hdr, 0, MEM_RELEASE));
             return FALSE;
            } /* header read failed */
        } /* wait for result */

     if(bytesRead != seekinfo->GetBytesPerSector())
        { /* bad header */
         c_Log.AddMessage(new TraceFormatError(TraceEvent::None, IDS_HEADER_WRONG_SIZE, bytesRead, seekinfo->GetBytesPerSector()));
         ::CloseHandle(file);
         file = NULL;
         VERIFY(::VirtualFree(hdr, 0, MEM_RELEASE));
         return FALSE;
        } /* bad header */

     long nrecs = atoi(hdr->reccount);
     seekinfo->SetRecordCount(nrecs);

     VERIFY(::VirtualFree(hdr, 0, MEM_RELEASE));

     return TRUE;
    } // CAsynchDlg::OpenDatabase

/****************************************************************************
*                          CAsynchDlg::OnBnClickedGenerate
* Result: void
*       
* Effect: 
*       Runs a sample using the current settings
****************************************************************************/

void CAsynchDlg::OnBnClickedGenerate()
   {
    if(!OpenDatabase())
       return;

    // seekinfo is now defined
    
    requests = c_SpinRequests.GetPos32() * REQUESTS_MULTIPLIER;

    seeks.SetSize(requests);

    c_Log.AddMessage(new TraceComment(TraceEvent::None, IDS_STARTING_GENERATION));

    // This seems odd, but the randomness of the numbers appears to suffer if we
    // don't fill them all in at once

    for(int i = 0; i < requests; i++)
       { /* fill in positions */
        long pos;

        int count = 0;
        while(TRUE)   
           { /* generate random number */
            DWORD rnd = (DWORD)Rand32::rand32();
            pos = rnd % seekinfo->GetRecordCount();

            //================================================================
            // The problem with the modulo computation is that it can
            // return long sequences of zeroes because the values can
            // be multiples of nrecs.  
            //================================================================
            if(pos == 0)
               { /* retry */
                // If we have a bug that keeps returning a pos of 0, give up and use it
                if(count++ > 100)
                   { /* failed */
                    ASSERT(FALSE);
                    break;
                   } /* failed */
                continue;
               } /* retry */
            break;
           } /* generate random number */

        //********************************
        // Force every 10th hit to be from
        // the same address as has been
        // previously used
        // The idea is to see if file
        // system cache hits have any
        // impact
        //********************************
        if(i > 0 && i % 10 == 0)
           { /* reuse address */
            pos = seeks[i / 10];
           } /* reuse address */
        //********************************

        seeks[i] = pos;
       } /* fill in positions */

    ::CloseHandle(file);
    file = NULL;

    c_Log.AddMessage(new TraceFormatComment(TraceEvent::None, IDS_ENDING_GENERATION, requests));

    updateControls();
   }

/****************************************************************************
*                       CAsynchDlg::OnEnChangeNrequests
* Result: void
*       
* Effect: 
*       Handles changes in the number of requests
****************************************************************************/

void CAsynchDlg::OnEnChangeNrequests()
   {
    if(!initialized)
        return; // not ready to handle these yet!
    // We've changed the number of elements in the test
    // clear all the values
    delete seekinfo;
    seekinfo = NULL;
    seeks.SetSize(0); // no more seek values
    c_Plot.SetSamples(0);

    RegistryInt samples(IDS_REGISTRY_SAMPLES);
    samples.value = c_SpinRequests.GetPos32();
    samples.store();
    
    updateControls();
   }

/****************************************************************************
*                         CAsynchDlg::updateControls
* Result: void
*       
* Effect: 
*       Updates the controls
****************************************************************************/

void CAsynchDlg::updateControls()
    {
     if(!initialized)
        return;
     
     //-----------------------
     // Requests, SpinRequests
     //-----------------------
     x_Requests1.EnableWindow(!running);
     x_Requests2.EnableWindow(!running);
     c_Requests.EnableWindow(!running);
     c_SpinRequests.EnableWindow(!running);
     
     //----------------
     // FILE_FLAG_...
     //----------------
     c_None.EnableWindow(!running);
     c_FILE_FLAG_RANDOM_ACCESS.EnableWindow(!running);
     c_FILE_FLAG_SEQUENTIAL_SCAN.EnableWindow(!running);
     c_FILE_FLAG_WRITE_THROUGH.EnableWindow(!running);
     c_FILE_FLAG_NO_BUFFERING.EnableWindow(!running);

     //----------------
     // GENERATE/RUN
     //----------------
     CString s;
     c_Filename.GetWindowText(s);
     s.Trim();
     c_Generate.EnableWindow(!running && !s.IsEmpty());

     c_Run.EnableWindow(!running && !s.IsEmpty() && seeks.GetSize() > 0);

     //---------------------------
     // Show Progress Bars option
     //---------------------------
     c_ShowProgress.EnableWindow(!running);

     //----------------
     // Progress
     //----------------
     c_CreateProgress.ShowWindow(running ? SW_SHOW : SW_HIDE);
     c_ResponseProgress.ShowWindow(running ? SW_SHOW : SW_HIDE);

     
    } // CAsynchDlg::updateControls

/****************************************************************************
*                             CAsynchDlg::OnSize
* Inputs:
*       UINT nType: Type of dialog
*       int cx: new width
*       int cy: new height
* Result: void
*       
* Effect: 
*       Resizes the plot area
****************************************************************************/

void CAsynchDlg::OnSize(UINT nType, int cx, int cy)
    {
     CDialog::OnSize(nType, cx, cy); 

     //****************************************************************
     // Resize Plot window
     //****************************************************************
     if(c_Plot.GetSafeHwnd() != NULL)
        { /* do resize */
         CRect r;
         c_Plot.GetWindowRect(&r);
         ScreenToClient(&r);

         c_Plot.SetWindowPos(NULL,
                             0,0, // no reposition
                             cx - r.left, cy - r.top,  // new size
                             SWP_NOZORDER | SWP_NOMOVE);
         c_Plot.Invalidate();
        } /* do resize */

     //****************************************************************
     // Resize Y-axis window
     //****************************************************************
     if(c_Y_Label.GetSafeHwnd() != NULL)
        { /* resize Y-label */
         CRect r;
         c_Y_Label.GetWindowRect(&r);
         ScreenToClient(&r);
         c_Y_Label.SetWindowPos(NULL,
                                0, 0, // no reposition
                                r.Width(),
                                cy - r.top,
                                SWP_NOZORDER | SWP_NOMOVE);
         c_Y_Label.Invalidate();
        } /* resize Y-label */

     //****************************************************************
     // Resize log window
     //****************************************************************
     if(c_Log.GetSafeHwnd() != NULL)
        { /* resize log */
         CRect r;
         c_Log.GetWindowRect(&r);
         ScreenToClient(&r);
         c_Log.SetWindowPos(NULL,
                            0,0, // no reposition
                            cx, r.Height(), // new width, same height
                            SWP_NOZORDER | SWP_NOMOVE);
        } /* resize log */
    }

/****************************************************************************
*                         CAsynchDlg::OnGetMinMaxInfo
* Inputs:
*       MINMAXINFO * MMI:
* Result: void
*       
* Effect: 
*       Limits the resize so we don't hide controls
* Notes:
*       This is based on the c_Limit control, an invisible frame, which
*       is laid out to determine the minimum sizes
****************************************************************************/

void CAsynchDlg::OnGetMinMaxInfo(MINMAXINFO * MMI)
    {
     CRect r;
     if(c_Limit.GetSafeHwnd() != NULL)
        { /* limit it */
         c_Limit.GetWindowRect(&r);
         ScreenToClient(&r);

         CalcWindowRect(&r);
         MMI->ptMinTrackSize = CPoint(r.Width(), r.Height());
        } /* limit it */

     CDialog::OnGetMinMaxInfo(MMI);
    }

/****************************************************************************
*                           CAsynchDlg::OnHelpAbout
* Result: void
*       
* Effect: 
*       Pops up the about dialog
****************************************************************************/

void CAsynchDlg::OnHelpAbout()
    {
     CAboutDlg dlg;
     dlg.DoModal();
    }

/****************************************************************************
*                         CAsynchDlg::OnFileSavePlot
* Result: void
*       
* Effect: 
*       Saves the plot data for later display and analysis
****************************************************************************/

void CAsynchDlg::OnFileSavePlot()
    {
     if(PlotFilename.IsEmpty())
        { /* save as */
         OnFileSavePlotAs();
        } /* save as */
     else
        { /* save existing */
         DoFileSave(PlotFilename);
        } /* save existing */
    }

/****************************************************************************
*                        CAsynchDlg::OnFileSavePlotAs
* Result: void
*       
* Effect: 
*       Prompts for a filename and saves the file
****************************************************************************/

void CAsynchDlg::OnFileSavePlotAs()
    {
     CString filter;
     filter.LoadString(IDS_PLOT_FILTER);
     
     CFileDialog dlg(FALSE, _T(".plt"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);
     if(dlg.DoModal() != IDOK)
        return;

     CString s = dlg.GetPathName();
     if(DoFileSave(s))
        { /* save successful */
         PlotFilename = s;
        } /* save successful */
    }

/****************************************************************************
*                           CAsynchDlg::OnFileExit
* Result: void
*       
* Effect: 
*       Exits the app
****************************************************************************/

void CAsynchDlg::OnFileExit()
    {
     CDialog::OnOK();
    }

/****************************************************************************
*                           CAsynchDlg::DoFileSave
* Inputs:
*       const CString & filename: String reference
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Saves the plot information (in XML format)
****************************************************************************/

BOOL CAsynchDlg::DoFileSave(const CString & filename)
    {
     CStdioFile f;
     if(!f.Open(filename, CFile::modeWrite | CFile::modeCreate))
        { /* failed */
         DWORD err = ::GetLastError();
         CString msg = ErrorString(err);
         msg += _T("\r\n");
         msg += filename;
         AfxMessageBox(msg, MB_OK | MB_ICONERROR);
         return FALSE;
        } /* failed */
     TCHAR ext[MAX_PATH];
     _tsplitpath(filename, NULL, NULL, NULL, ext);
     if(_tcsicmp(_T(".csv"), ext) == 0)
        { /* save csv */
         c_Plot.data.WriteCSV(f);
        } /* save csv */
     else
        { /* save xml */
         c_Plot.data.Write(f);
        } /* save xml */
     f.Close();
     return TRUE;
    } // CAsynchDlg::DoFileSave

/****************************************************************************
*                        CAsynchDlg::OnResponseStepit
* Inputs:
*       WPARAM: ignored
*       LPARAM: ignored
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       ¶
****************************************************************************/

LRESULT CAsynchDlg::OnResponseStepit(WPARAM, LPARAM)
    {
     c_ResponseProgress.StepIt();
     return 0;
    } // CAsynchDlg::OnResponseStepit

/****************************************************************************
*                         CAsynchDlg::OnCreateStepit
* Inputs:
*       WPARAM: ignored
*       LPARAM: ignored
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Steps the creation progress bar
****************************************************************************/

LRESULT CAsynchDlg::OnCreateStepit(WPARAM, LPARAM)
    {
     c_CreateProgress.StepIt();
     return 0;
    } // CAsynchDlg::OnCreateStepit

/****************************************************************************
*                          CAsynchDlg::OnThreadDone
* Inputs:
*       WPARAM: ignored
*       LPARAM: ignored
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Handles the thread termination
****************************************************************************/

LRESULT CAsynchDlg::OnThreadDone(WPARAM, LPARAM)
    {
     running = FALSE;
     updateControls();
     c_Plot.DoPlot();
     return 0;
    } // CAsynchDlg::OnThreadDone

/****************************************************************************
*                           CAsynchDlg::OnEditClear
* Result: void
*       
* Effect: 
*       Clears the information from the display
****************************************************************************/

void CAsynchDlg::OnEditClear()
    {
     c_Plot.SetSamples(0);
    }

/****************************************************************************
*                         CAsynchDlg::OnEditClearlog
* Result: void
*       
* Effect: 
*       Clears the log
****************************************************************************/

void CAsynchDlg::OnEditClearlog()
    {
     c_Log.ResetContent();
    }

/****************************************************************************
*                       CAsynchDlg::OnBnClickedScatter
*                    void CAsynchDlg::OnBnClickedTemporal()
* Result: void
*       
* Effect: 
*       Changes the format of the display
****************************************************************************/

void CAsynchDlg::OnBnClickedScatter()
    {
     updateDisplay();
    }

void CAsynchDlg::OnBnClickedTemporal()
    {
     updateDisplay();
    }

/****************************************************************************
*                          CAsynchDlg::updateDisplay
* Result: void
*       
* Effect: 
*       Updates the display
****************************************************************************/

void CAsynchDlg::updateDisplay()
    {
     CString label;
     if(c_ScatterPlot.GetCheck() == BST_CHECKED)
        { /* scatter labels */
         c_Plot.SetDisplayMode(CPlot::ScatterPlot);

         label.LoadString(IDS_X_LABEL_SCATTER);
         c_X_Label.SetWindowText(label);
         label.LoadString(IDS_Y_LABEL_SCATTER);
         c_Y_Label.SetWindowText(label); 
        } /* scatter labels */
     else
     if(c_TimePlot.GetCheck() == BST_CHECKED)
        { /* time plot */
         c_Plot.SetDisplayMode(CPlot::TimePlot);

         label.LoadString(IDS_X_LABEL_TIME);
         c_X_Label.SetWindowText(label);
         label.LoadString(IDS_Y_LABEL_TIME);
         c_Y_Label.SetWindowText(label);
        } /* time plot */
    } // CAsynchDlg::updateDisplay

/****************************************************************************
*                              CAsynchDlg::OnLog
* Inputs:
*       WPARAM: (WPARAM)(TraceEvent *) string to log
*       LPARAM: ignored
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Logs the message
****************************************************************************/

LRESULT CAsynchDlg::OnLog(WPARAM wParam, LPARAM)
    {
     c_Log.AddMessage((TraceEvent *)wParam);
     return 0;
    } // CAsynchDlg::OnLog

/****************************************************************************
*                         CAsynchDlg::OnBnClickedRun
* Result: void
*       
* Effect: 
*       Runs an experiment on the current set of values
****************************************************************************/

void CAsynchDlg::OnBnClickedRun()
    {
     if(!OpenDatabase())
        return;

    //================================================================
    // We have a successful read of the header.  Create the
    // I/O Completion Port that goes with it
    //================================================================
     port = ::CreateIoCompletionPort(file, NULL, 0, 0);
     if(port == NULL)
        { /* failed */
         DWORD err = ::GetLastError();
         c_Log.AddMessage(new TraceError(TraceEvent::None, IDS_COMPLETION_FAILED));
         ::CloseHandle(file);
         file = NULL;
         return;
        } /* failed */

     c_Log.AddMessage(new TraceFormatComment(TraceEvent::None, IDS_NRECS, seekinfo->GetRecordCount()));
     running = TRUE;

     c_CreateProgress.SetRange32(0, requests);
     c_CreateProgress.SetPos(0);
     c_CreateProgress.SetStep(1);

     c_ResponseProgress.SetRange32(0, requests);
     c_ResponseProgress.SetPos(0);
     c_ResponseProgress.SetStep(1);

     c_Plot.SetSamples(requests);

     ShowProgress = c_ShowProgress.GetCheck() == BST_CHECKED;

    //================================================================
    // Now launch the sender and receiver threads.  Note the order,
    // that we launch the receiver thread first.  This might
    // well avoid any startup transients.
    // (need to think about this...)
    //================================================================

     AfxBeginThread(iocomp, this);

     updateControls();
    }

/****************************************************************************
*                         CAsynchDlg::OnInitMenuPopup
* Inputs:
*       CMenu * pPopupMenu
*       UINT nIndex: ignored
*       BOOL bSysMenu: TRUE if system menu
* Result: void
*       
* Effect: 
*       Handles enabling of items
****************************************************************************/

void CAsynchDlg::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
   {
    CDialog::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);
    
    if(!bSysMenu)
       { /* main menu */
        CMenu * menu = GetMenu();
        menu->EnableMenuItem(ID_FILE_EXIT, running ? MF_GRAYED : MF_ENABLED);
       } /* main menu */
    else
       { /* system menu */
        pPopupMenu->EnableMenuItem(SC_CLOSE, running ? MF_GRAYED : MF_ENABLED);
       } /* system menu */
    // TODO: Add your message handler code here
   }

/****************************************************************************
*                         CAsynchDlg::OnToolTipNotify
* Inputs:
*       UINT id: ID of control
*       NMHDR * pNMHDR: notification header
*       LRESULT * pResult: Set to 0, always
* Result: BOO
*       TRUE if message was handled
* Effect: 
*       Provides text to tooltips
****************************************************************************/

BOOL CAsynchDlg::OnToolTipNotify(UINT id, NMHDR * pNMHDR, LRESULT * pResult)
    {
     TOOLTIPTEXT * pTTT = (TOOLTIPTEXT *)pNMHDR;
     UINT_PTR nID = pNMHDR->idFrom;

     CString text;
     
     if (pNMHDR->code == TTN_NEEDTEXT && (pTTT->uFlags & TTF_IDISHWND))
        {
         // idFrom is actually the HWND of the tool
         nID = ::GetDlgCtrlID((HWND)nID);
        }

     switch(nID)
        { /* decode text */
         case 0:
            *pResult = 0;
            return FALSE;
         default:
            pTTT->lpszText = MAKEINTRESOURCE(nID);
            pTTT->hinst = AfxGetInstanceHandle();
            break;
        } /* decode text */

     *pResult = 0;

     return TRUE;
    } // CAsynchDlg::OnToolTipNotify

/****************************************************************************
*                           CAsynchDlg::OnEditCopy
* Result: void
*       
* Effect: 
*       Copies the plot to the clipboard
****************************************************************************/

void CAsynchDlg::OnEditCopy()
    {
     toClipboard(&c_Plot, TRUE);
    }
