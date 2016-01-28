/*****************************************************************************
*           Change Log
*  Date     | Change
*-----------+-----------------------------------------------------------------
* 25-Feb-99 | [1.379] Created
* 25-Feb-99 | REQ #010: [1.379] Delete after the startup boundary, not from 0
* 26-Feb-99 | REQ #015: [1.379] If deleting, make sure that the top index
*           | is properly maintained; otherwise the box will not allow
*           | manual scrolling while it is being added to once the limit is hit
*           | (a nasty interaction with DeleteString)
*  8-Feb-00 | REQ #154: Brought up to new TraceEvent features
*  8-Jan-01 | REQ #017: discovered item 17 has not been logged. Provided
*           | message to allow asynchronous update across threads
*****************************************************************************/
// TraceList.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TraceEvent.h"
#include "Trace.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static int indent1 = 0;
static int indent2 = 0;

/****************************************************************************
*                                 UWM_MESSAGE
* Inputs:
*       WPARAM: (WPARAM)(TraceEvent *)
*       LPARAM: ignored
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Adds the message to the trace window.
* Notes:
*       This is done in the thread context of the main GUI thread to avoid
*       deadlock with worker threads
****************************************************************************/

static const UINT UWM_MESSAGE = ::RegisterWindowMessage(_T("UWM_MESSAGE-{B7C2D240-D5E1-11d2-839F-D8C378000000}"));// REQ #017


/////////////////////////////////////////////////////////////////////////////
// CTraceList

CTraceList::CTraceList()
{
 boundary = 0;
 fixed = NULL; // no initial font
 height = 0;   // initial height is unknown
 modified = FALSE;
 toDisk = FALSE;
 maxwidth = 0;
 limit = 0x7FFFFFFF; // actually, this is ignored if 'limited' is FALSE
 limited = FALSE;
 deleted = 0;                                                       // REQ #010
}

CTraceList::~CTraceList()
{
}


BEGIN_MESSAGE_MAP(CTraceList, CListBox)
        //{{AFX_MSG_MAP(CTraceList)
        ON_REGISTERED_MESSAGE(UWM_MESSAGE, OnMessage)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTraceList message handlers

/****************************************************************************
*                            CTraceList::setLimit
* Inputs:
*       int limit: Amount of limit, or < 0 to disable limiting
* Result: void
*       
* Effect: 
*       Sets the limit value, or disables limiting
****************************************************************************/

void CTraceList::setLimit(int limit)
    {
     if(limit < 0)
        { /* disable */
         limited = FALSE;
         return;
        } /* disable */

     if(limit < MIN_LOG_ENTRIES) 
        limit = MIN_LOG_ENTRIES; 
     if(limit < boundary)
        limit = boundary + MIN_LOG_ENTRIES;
     this->limit = limit; 
     limited = TRUE;     
    }

/****************************************************************************
*                           CTraceList::MeasureItem
* Inputs:
*       LPMEASUREITEMSTRUCT mis:
* Result: void
*       
* Effect: 
*       Returns the height of the item in mis->height
****************************************************************************/

void CTraceList::MeasureItem(LPMEASUREITEMSTRUCT mis) 
{
 if(height == 0)
    { /* compute double-height */
     CClientDC dc(this);
     CFont * f = GetFont();
     dc.SelectObject(f);
     TEXTMETRIC tm;
     dc.GetTextMetrics(&tm);
     height = tm.tmHeight + tm.tmInternalLeading;
    } /* compute double-height */

 TraceEvent * e = (TraceEvent *)mis->itemData;
 mis->itemHeight = height * e->displayHeight();
        
}

/****************************************************************************
*                            CTraceList::isVisible
* Inputs:
*       int n: Position in list
* Result: BOOL
*       TRUE if visible
*       FALSE if hidden
* Notes: 
*       It is assumed that this is working on the last item in the list
*       so we are only concerned about it falling off the bottom
****************************************************************************/

BOOL CTraceList::isVisible(int n, BOOL fully)
    {
     CRect item;
     CRect r;
     GetItemRect(n, &item);
     GetClientRect(&r);

     if(!fully && item.top < r.bottom)
        return TRUE;
     if(fully && item.bottom < r.bottom)
        return TRUE;
     
     return FALSE;
    }

/****************************************************************************
*                            CTraceList::setToDisk
* Inputs:
*       BOOL v: Value to set
* Result: void
*       
* Effect: 
*       If not already set, requests a file to log to
* Notes:
*       Case    toDisk          v       Effect
*       I       FALSE           FALSE   ignored
*       II      TRUE            FALSE   toDisk = FALSE
*       III     TRUE            TRUE    ignored
*       IV      FALSE           TRUE    prompt for name
****************************************************************************/

void CTraceList::setToDisk(BOOL v)
    {
     if(!toDisk && !v)
        { /* I. */
         return; // ignore false-to-false
        } /* I. */

     if(toDisk)
        { /* II., III. */
         toDisk = v;
         return;
        } /* II., III. */

     // IV.
     doSave(FALSE); // force prompt for file name and save current state
     toDisk = TRUE;
    }

/****************************************************************************
*                            CTraceList::AddString
* Inputs:
*       TraceEvent * event: Event to add
* Result: int
*       Place where event was added
* Effect: 
*       If the last item in the box was visible before the add, and is not
*       visible after the add, scroll the box up
****************************************************************************/

int CTraceList::AddString(TraceEvent * event)
    {
     if(toDisk)
        appendToFile(event);

     // During shutdown transient, the window may actually be gone
     if(!::IsWindow(m_hWnd))
        { /* gone */
         delete event; // already logged to file if file logging on
         return LB_ERR;
        } /* gone */

     // If we've exceeded the number of lines of trace that are set,
     // delete enough leading lines to get below the limit

     BOOL visible = FALSE;
     int count = GetCount();
     if(count > 0)
        { /* nonempty */
         visible = isVisible(count - 1, FALSE); // any part showing?
        } /* nonempty */
     else
        visible = TRUE;

     int top = GetTopIndex(); // get current top of list

     BOOL first = TRUE;
     while(limited && count > limit - 1)
        { /* delete first */                                        // REQ #015
         if(first)                                                  // REQ #010
            DeleteString(boundary);                                 // REQ #010
         first = FALSE;                                             // REQ #010
         DeleteString(boundary);                                    // REQ #010
         deleted++;                                                 // REQ #010
         count = GetCount();                                        // REQ #010
         if(boundary < top)                                         // REQ #015
            SetTopIndex(top);                                       // REQ #015
         else                                                       // REQ #015
            SetTopIndex(top - 1);                                   // REQ #015
        } /* delete first */                                        // REQ #015
     
     if(!first)                                                     // REQ #010
        { /* deleted at least one */                                // REQ #010
         InsertString(boundary, new TraceFormatComment(TraceEvent::None, IDS_DELETED_ENTRIES, deleted + 1)); // REQ #010
                                                                    // REQ #154
        } /* deleted at least one */                                // REQ #010

     count = GetCount();
     // We want to auto-scroll the list box if the last line is showing,
     // even if partially

     CRect r;

     // Add the element to the list box

     modified = TRUE; 
     int n = CListBox::AddString((LPCTSTR) event);      

     // If the last line had been visible, and the just-added line is
     // not visible, scroll the list box up to make it visible.

     // Note: This depends on the box being LBS_OWNERDRAWFIXED.  The
     // computation for variable is much more complex!
     if(visible)
        { /* was visible */
         visible = isVisible(n, TRUE); // all of it showing?
         if(!visible)
            { /* dropped off */
             top = GetTopIndex();
             if(top + 1 >= GetCount())
                { /* can't scroll */
                } /* can't scroll */
             else
                { /* scroll it */
                 SetTopIndex(top + 1);
                } /* dropped off */
            } /* dropped off */
        } /* was visible */

     // Get the width and compute the scroll width
     // Optimization: if the current width is less than the current
     // maximum, we don't need to do anything

     CClientDC dc(this);
     CString s = event->show();
     UINT w = dc.GetTextExtent(s, s.GetLength()).cx;
     if(w > maxwidth)
        { /* recompute */
         maxwidth = w;
         SetHorizontalExtent((int)maxwidth);
        } /* recompute */

     // If this is an error notification, notify the parent to update
     // the controls
     if(event->IsKindOf(RUNTIME_CLASS(TraceError)))
        GetParent()->SendMessage(TLM_UPDATE_ERRORS);
     return n;
    }

/****************************************************************************
*                            CTraceList::DrawItem
* Inputs:
*       LPDRAWITEMSTRUCT dis:
* Result: void
*       
* Effect: 
*       Draws the item
*
****************************************************************************/

void CTraceList::DrawItem(LPDRAWITEMSTRUCT dis) 
{
 CDC * dc = CDC::FromHandle(dis->hDC);
 
 if(height == 0)
    { /* compute line height */
     TEXTMETRIC tm;
     dc->GetTextMetrics(&tm);
     height = tm.tmHeight + tm.tmInternalLeading;
    } /* compute line height */

 if(indent1 == 0)
    { /* compute indent */
     static const CString prefix1 = _T("000  ");
     indent1 = dc->GetTextExtent(prefix1, prefix1.GetLength()).cx;
     static const CString prefix2 = _T("00000000  ");
     indent2 = dc->GetTextExtent(prefix2, prefix2.GetLength()).cx;
    } /* compute indent */

 if(dis->itemID == (UINT)-1)
    { /* no items */
     CBrush bg(::GetSysColor(COLOR_WINDOW));
     dc->FillRect(&dis->rcItem, &bg);
     if(dis->itemState & ODS_FOCUS)
        { /* selected */
         dc->DrawFocusRect(&dis->rcItem);
        } /* selected */
     return;
    } /* no items */

 TraceEvent * e = (TraceEvent *)dis->itemData;

// if(dis->itemState & ODA_FOCUS)
//    { /* focus only */
//     dc->DrawFocusRect(&dis->rcItem);
//     return;
//    } /* focus only */

 int saved = dc->SaveDC();

 COLORREF txcolor;
 COLORREF bkcolor;

 if(dis->itemState & ODS_SELECTED)
    { /* selected */
     if(::GetFocus() == m_hWnd)
        { /* has focus */
         bkcolor = ::GetSysColor(COLOR_HIGHLIGHT);
         txcolor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
        } /* has focus */
     else
        { /* no focus */
         bkcolor = ::GetSysColor(COLOR_BTNFACE);
         txcolor = ::GetSysColor(COLOR_BTNTEXT);
        } /* no focus */
    } /* selected */
 else
    { /* not selected */
     txcolor = e->textcolor();
     bkcolor = ::GetSysColor(COLOR_WINDOW);
    } /* not selected */

 {
  CBrush bg(bkcolor);
  dc->FillRect(&dis->rcItem, &bg);
 }

 dc->SetTextColor(txcolor);
 dc->SetBkMode(TRANSPARENT);

 CString id = e->showID();

 dc->TextOut(dis->rcItem.left, dis->rcItem.top, id);

 CString s = e->showThread();

 dc->TextOut(dis->rcItem.left + indent1, dis->rcItem.top, s);

 s = e->show();

 dc->TextOut(dis->rcItem.left + indent1 + indent2, dis->rcItem.top, s);

 if(dis->itemState & ODS_FOCUS)
    dc->DrawFocusRect(&dis->rcItem);

 dc->RestoreDC(saved);
}

/****************************************************************************
*                           CTraceList::DeleteItem
* Inputs:
*       LPDELETEITEMSTRUCT dis
* Result: void
*       
* Effect: 
*       Deletes the item attached to the element
****************************************************************************/

void CTraceList::DeleteItem(LPDELETEITEMSTRUCT dis) 
{
 TraceEvent * e = (TraceEvent *)dis->itemData;
 delete e;

 CListBox::DeleteItem(dis);
}

/****************************************************************************
*                              TraceList::doSave
* Inputs:
*       BOOL mode: TRUE for Save, FALSE for Save As
* Result: void
*       
* Effect: 
*       Does a save
****************************************************************************/

void CTraceList::doSave(BOOL mode)
    {
     BOOL prompt = FALSE;

     if(mode)
        { /* Save */
         if(SaveFileName.GetLength() == 0)
            { /* no file name set */
             prompt = TRUE;
            } /* no file name set */
        } /* Save */
     else
        { /* SaveAs */
         prompt = TRUE;
        } /* SaveAs */

     CString name;

     if(prompt)
        { /* get file name */
         CString filter;
         filter.LoadString(IDS_LOG_FILTER);
         CFileDialog dlg(FALSE, _T(".log"), NULL,
                                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
                                filter);

         CString s;
         s.LoadString(mode ? IDS_FILE_SAVE : IDS_FILE_SAVE_AS);
         dlg.m_ofn.lpstrTitle = s;

         switch(dlg.DoModal())
            { /* domodal */
             case 0: // should never happen
             case IDCANCEL:
                     return;
            } /* domodal */

         // We get here, it must be IDOK

         name = dlg.GetPathName();
        } /* get file name */
     else
        { /* already has name */
         name = SaveFileName;
        } /* already has name */

     TRY
        {
         CStdioFile f(name, CFile::modeCreate | CFile::modeWrite | CFile::shareExclusive); //lint -e655
         for(int i = 0; i < GetCount(); i++)
            { /* write each */
             TraceEvent * e = (TraceEvent *)GetItemDataPtr(i);
             f.WriteString(e->showfile());

             f.WriteString(_T("\r\n"));
            } /* write each */

         f.Close();

         SaveFileName = name;

         SetModified(FALSE);
        }
     CATCH(CFileException, e)
        { /* error */
         AfxMessageBox(IDS_FILE_OPEN_FAILED, MB_ICONERROR | MB_OK);
         // TODO: Make this report a meaningful error (it's 5am and I want
         // to finish this before I leave for a trip later today!)
         return; // return without doing anything further
        } /* error */
     END_CATCH
     
    }

/****************************************************************************
*                          CTraceList::appendToFile
* Inputs:
*       TraceEvent * event:
* Result: void
*       
* Effect: 
*       Logs the event to the current file
****************************************************************************/

void CTraceList::appendToFile(TraceEvent * event)
    {
     if(SaveFileName.GetLength() == 0)
        return; // shouldn't be here

     TRY {
          CStdioFile f(SaveFileName, CFile::modeReadWrite | CFile::modeCreate |
                        CFile::modeNoTruncate | CFile::shareExclusive); //lint -e655


          f.SeekToEnd();
          f.WriteString(event->showfile());
          f.WriteString(_T("\r\n"));
          f.Close();
         }
      CATCH(CFileException, e) 
         { 
          // Do nothing.
         }
      END_CATCH
    }

/****************************************************************************
*                              CTraceList::toTop
* Result: void
*       
* Effect: 
*       Positions the list box to the top
****************************************************************************/

void CTraceList::toTop()
    {
     if(GetCount() == 0)
        return;

     SetTopIndex(0);
     SetCurSel(0);
    }

/****************************************************************************
*                              CTraceList::toEnd
* Result: void
*       
* Effect: 
*       Moves to the end of the box
****************************************************************************/

void CTraceList::toEnd()
    {
     int n = GetCount();
     if(n == 0)
        return;
     SetTopIndex(n - 1);
     SetCurSel(n - 1);
    }

/****************************************************************************
*                              CTraceList::isTop
* Result: BOOL
*       TRUE if toTop has meaning
****************************************************************************/

BOOL CTraceList::isTop()
    {
     if(GetCount() == 0)
        return FALSE;
     int n = GetTopIndex();
     return (n > 0);
    }

/****************************************************************************
*                              CTraceList::isEnd
* Result: BOOL
*       TRUE if toEnd has meaning
****************************************************************************/

BOOL CTraceList::isEnd()
    {
     int count = GetCount();

     if(count == 0)
        return FALSE;

     // if(isVisible(count - 1, TRUE))
     //    return FALSE;

     return TRUE;     
    }

/****************************************************************************
*                            CTraceList::findNext
* Inputs:
*       int i: Starting position
* Result: int
*       Next error position, or LB_ERR if not found
****************************************************************************/

int CTraceList::findNext(int i)
    {
     int count = GetCount();

     for(++i ; i < count; i++)
        { /* scan forward */
         TraceEvent * e = (TraceEvent *)GetItemDataPtr(i);
         if(e->IsKindOf(RUNTIME_CLASS(TraceError)))
            { /* found it */
             return i;
            } /* found it */
        } /* scan forward */
     return LB_ERR;
    }

/****************************************************************************
*                            CTraceList::findPrev
* Inputs:
*       int i: Starting position
* Result: int
*       Previous error position, or LB_ERR if not found
****************************************************************************/

int CTraceList::findPrev(int i)
    {
     int count = GetCount();
     if(count == 0)
        return LB_ERR;

     for(--i ; i >= 0; i--)
        { /* scan backward */
         TraceEvent * e = (TraceEvent *)GetItemDataPtr(i);
         if(e->IsKindOf(RUNTIME_CLASS(TraceError)))
            { /* found it */
             return i;
            } /* found it */
        } /* scan backward */
     return LB_ERR;
    }

/****************************************************************************
*                             CTraceList::toNext
* Result: BOOL

*       FALSE if we didn't move
* Effect: 
*       Sets the selection to the next error
****************************************************************************/

BOOL CTraceList::toNext()
    {
     int start = GetCurSel();

     int i = findNext(start);
     if(i == LB_ERR)
        return FALSE;

     if(i > 3)
        SetTopIndex(i - 3);
     else
        SetTopIndex(i);

     SetCurSel(i);
     return TRUE;
    }

/****************************************************************************
*                             CTraceList::toPrev
* Result: BOOL
*       TRUE if we moved
*       FALSE if we didn't move
* Effect: 
*       Sets the selection to the previous error
****************************************************************************/

BOOL CTraceList::toPrev()
    {
     int start = GetCurSel();

     int i = findPrev(start);
     if(i == LB_ERR)
        return FALSE;

     if(i > 3)
        SetTopIndex(i - 3);
     else
        SetTopIndex(i);

     SetCurSel(i);
     return TRUE;
    }

/****************************************************************************
*                             CTraceList::isPrev
* Result: BOOL
*       TRUE if there is a previous error
****************************************************************************/

BOOL CTraceList::isPrev()
    {
     return findPrev(GetCurSel()) != LB_ERR;
    }

/****************************************************************************
*                             CTraceList::isNext
* Result: BOOL
*       TRUE if there is a next error
****************************************************************************/

BOOL CTraceList::isNext()
    {
     return findNext(GetCurSel()) != LB_ERR;
    }

/****************************************************************************
*                            CTraceList::OnMessage
* Inputs:
*       WPARAM: (WPARAM)(TraceEvent *)
* Result: LRESULT
*       Logically void, 0, always
* Effect: 
*       Adds the message to the list
****************************************************************************/

LRESULT CTraceList::OnMessage(WPARAM wParam, LPARAM)                // REQ #017
    {                                                               // REQ #017
     AddString((TraceEvent *)wParam);                               // REQ #017
     return 0; // logically void                                    // REQ #017
    } // CTraceList::OnMessage                                      // REQ #017

/****************************************************************************
*                           CTraceList::AddMessage
* Inputs:
*       TraceEvent * event: Event to add
* Result: void
*       
* Effect: 
*       Posts a message to the window
****************************************************************************/

void CTraceList::AddMessage(TraceEvent * event)
    {
     PostMessage(UWM_MESSAGE, (WPARAM)event);
    } // CTraceList::AddMessage
