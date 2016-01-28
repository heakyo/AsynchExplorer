// CreateData.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "CreateData.h"
#include "ErrorString.h"
#include "Seeker.h"
#include "RegVars.h"

#define NUM_RECORDS_PER_BLOCK 1000

// CCreateData dialog

IMPLEMENT_DYNAMIC(CCreateData, CDialog)
CCreateData::CCreateData(CWnd* pParent /*=NULL*/)
        : CDialog(CCreateData::IDD, pParent)
   {
   }

CCreateData::~CCreateData()
   {
   }

void CCreateData::DoDataExchange(CDataExchange* pDX)
   {
   CDialog::DoDataExchange(pDX);
   DDX_Control(pDX, IDC_SPIN_RECORDS, c_SpinRecords);
   DDX_Control(pDX, IDC_PROGRESS, c_Progress);
   }


BEGIN_MESSAGE_MAP(CCreateData, CDialog)
    ON_BN_CLICKED(IDC_CREATE_DATA, OnBnClickedCreateData)
END_MESSAGE_MAP()


// CCreateData message handlers

void CCreateData::OnBnClickedCreateData()
    {
     CString filter;
     filter.LoadString(IDS_DATABASE_FILTER);
     CFileDialog dlg(FALSE, _T(".dat"), NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);
     
     if(dlg.DoModal() != IDOK)
        return; // not found

     CString filename = dlg.GetPathName();
     filename.Trim();
     if(filename.IsEmpty())
        return;

     CStdioFile f;

     if(!f.Open(filename, CFile::modeWrite | CFile::modeCreate))
        { /* failed to open */
         DWORD err = ::GetLastError();
         CString msg = ErrorString(err);
         msg += _T("\r\n");
         msg += filename;
         AfxMessageBox(msg, MB_ICONERROR | MB_OK);
         return;
        } /* failed to open */

     int n = c_SpinRecords.GetPos32();

     Seeker seekinfo(f.GetFilePath());

     c_Progress.SetRange32(0, n);
     c_Progress.SetPos(0);
     c_Progress.ShowWindow(SW_SHOW);
     c_Progress.SetStep(1);

     CString length;
     length.Format(_T("%06d"), n * NUM_RECORDS_PER_BLOCK);
     length += _T("\r\n");
     f.WriteString(length);   // first 6 bytes are length, plus CRLF

     for(int i = 0; i < n; i++)
        { /* create file */
         AddBlock(f, i, &seekinfo);
         c_Progress.StepIt();
         c_Progress.UpdateData();
        } /* create file */
     c_Progress.StepIt();
     c_Progress.ShowWindow(SW_HIDE);
     f.Close();
     RegistryString database(IDS_REGISTRY_DATABASE);
     database.value = filename;
     database.store();
    }

/****************************************************************************
*                          CCreateData::OnInitDialog
* Result: BOOL
*       TRUE, always
* Effect: 
*       Initializes the dialog
****************************************************************************/

BOOL CCreateData::OnInitDialog()
    {
     CDialog::OnInitDialog();

     c_SpinRecords.SetRange32(1, 100);
     c_SpinRecords.SetPos(10);
     c_Progress.ShowWindow(SW_HIDE);

     return TRUE;  // return TRUE unless you set the focus to a control
                   // EXCEPTION: OCX Property Pages should return FALSE
    }

/****************************************************************************
*                            CCreateData::AddBlock
* Inputs:
*       CStdioFile & f: File
*       int n: Block number
* Result: void
*       
* Effect: 
*       Writes a block of the file.  Each block has NUM_RECORDS_PER_BLOCK records
****************************************************************************/

void CCreateData::AddBlock(CStdioFile & f, int n, Seeker * seekinfo)
    {
     // Each record is of the form
     // nnnnnnnn nnnnnnnn ... nnnnnnn
     for(int i = 0; i < NUM_RECORDS_PER_BLOCK; i++)
        { /* write block */
         int recno = n * NUM_RECORDS_PER_BLOCK + i;
         CString s;
         s.Format(_T("%08d "), recno);

         CString data;
         while((DWORD)data.GetLength() < seekinfo->GetBytesPerRecord())
            { /* make block */
             data += s;
            } /* make block */
         f.WriteString(data.Left(seekinfo->GetBytesPerRecord()  - 1));
         f.WriteString(_T("*"));
        } /* write block */
    } // CCreateData::AddBlock
