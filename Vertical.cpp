// Vertical.cpp : implementation file
//

#include "stdafx.h"
#include "Vertical.h"
#include "SaveDC.h"
#include ".\vertical.h"


// CVertical

IMPLEMENT_DYNAMIC(CVertical, CStatic)
CVertical::CVertical()
{
 brush.CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
 font = NULL;
}

CVertical::~CVertical()
{
}


BEGIN_MESSAGE_MAP(CVertical, CStatic)
//    ON_WM_CTLCOLOR_REFLECT()
ON_WM_PAINT()
ON_MESSAGE(WM_SETFONT, OnSetFont)
ON_MESSAGE(WM_GETFONT, OnGetFont)
ON_MESSAGE(WM_SETTEXT, OnSetText)
ON_MESSAGE(WM_GETTEXT, OnGetText)
ON_MESSAGE(WM_GETTEXTLENGTH, OnGetTextLength)
ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CVertical message handlers


//HBRUSH CVertical::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
//    {
//     pDC->SetTextAlign(TA_BOTTOM);
//     return (HBRUSH)brush;
//    }

void CVertical::OnPaint()
    { 
     CPaintDC dc(this); // device context for painting

     CRect r;
     GetClientRect(&r);

     CRgn rgn;
     rgn.CreateRectRgn(r.left, r.top, r.right, r.bottom);
     
     CFont * f = GetFont();
     if(f == NULL) 
        { /* font missing */
         f = GetParent()->GetFont();
         if(f == NULL)
            return; // nothing to draw, no font has been set
         else
            SetFont(f);
        } /* font missing */

     LOGFONT lf;
     f->GetLogFont(&lf);
     lf.lfEscapement = 900;
     lf.lfOrientation = 900;


     CFont yfont;
     VERIFY(yfont.CreateFontIndirect(&lf));

     CString s;
     GetWindowText(s);

     { /* display data */
      CSaveDC save(dc);

      dc.SetTextAlign(TA_BOTTOM); 
      dc.SetBkMode(TRANSPARENT);

      dc.SelectClipRgn(&rgn);
      dc.SelectObject(&yfont);

      int y = r.bottom;
      y -= 4 * ::GetSystemMetrics(SM_CXBORDER);
    
      dc.TextOut(r.right, y, s);
     } /* display data */


    // Do not call CStatic::OnPaint() for painting messages
    }

/****************************************************************************
*                            CVertical::OnSetFont
* Inputs:
*       WPARAM: (WPARAM)(HFONT)
*       LPARAM: (LPARAM)MAKELONG(BOOL, ?): Redraw request
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Sets the font
****************************************************************************/

LRESULT CVertical::OnSetFont(WPARAM wParam, LPARAM lParam)
    {
     font = (HFONT)wParam;
     if(LOWORD(lParam))
        Invalidate();
     return 0;
    } // CVertical::OnSetFont

/****************************************************************************
*                            CVertical::OnGetFont
* Inputs:
*       WPARAM: unused
*       LPARAM: unused
* Result: LRESULT
*       (LRESULT)(HANDLE) handle of a font
****************************************************************************/

LRESULT CVertical::OnGetFont(WPARAM, LPARAM)
    {
     return (LRESULT)font;
    } // CVertical::OnGetFont

/****************************************************************************
*                            CVertical::OnSetText
* Inputs:
*       WPARAM: ignored
*       LPARAM: (LPARAM)(LPCTSTR)
* Result: LRESULT
*       TRUE if successful (always)
* Effect: 
*       Sets the font
****************************************************************************/

LRESULT CVertical::OnSetText(WPARAM, LPARAM lParam)
    {
     text = (LPCTSTR)lParam;
     return TRUE;
    } // CVertical::OnSetText

/****************************************************************************
*                            CVertical::OnGetText
* Inputs:
*       WPARAM: length of buffer in TCHARs
*       LPARAM: (LPARAM)(LPTSTR)
* Result: LRESULT
*       Actual number of characters copied
* Effect: 
*       Copies the text out to the specified buffer
****************************************************************************/

LRESULT CVertical::OnGetText(WPARAM wParam, LPARAM lParam)
    {
     WPARAM n = text.GetLength();
     if(n >= wParam)
        n = wParam;
     
     LPTSTR end;
     StringCchCopyEx((LPTSTR)lParam, wParam, text, &end, NULL, 0);
     LRESULT len = end - (LPTSTR)lParam;
     return len;
    } // CVertical::OnGetText

/****************************************************************************
*                         CVertical::OnGetTextLength
* Inputs:
*       WPARAM: unused
*       LPARAM: unused
* Result: LRESULT
*       Length of characters in string
****************************************************************************/

LRESULT CVertical::OnGetTextLength(WPARAM, LPARAM)
    {
     return (LRESULT)text.GetLength();
    } // CVertical::OnGetTextLength

/****************************************************************************
*                           CVertical::OnEraseBkgnd
* Inputs:
*       CDC * pDC: DC
* Result: BOOL
*       TRUE, always
* Effect: 
*       Erases the background
****************************************************************************/

BOOL CVertical::OnEraseBkgnd(CDC* pDC)
    {
     CRect r;
     GetClientRect(&r);
     pDC->FillSolidRect(&r, ::GetSysColor(COLOR_3DFACE));
     return TRUE;
    }
