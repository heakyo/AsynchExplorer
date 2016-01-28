// Plot.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "Plot.h"
#include "SaveDC.h"
#include <math.h>
#include "log.h"

static UINT UWM_LOG = ::RegisterWindowMessage(UWM_LOG_MSG);

// CPlot

IMPLEMENT_DYNAMIC(CPlot, CStatic)
CPlot::CPlot()
{
 CurrentMode = ScatterPlot;
 info = NULL;
}

CPlot::~CPlot()
{
}


BEGIN_MESSAGE_MAP(CPlot, CStatic)
    ON_WM_PAINT()
    ON_WM_ERASEBKGND()
    ON_WM_MOUSEMOVE()
    ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
END_MESSAGE_MAP()



// CPlot message handlers


/****************************************************************************
*                                CPlot::DoPlot
* Result: void
*       
* Effect: 
*       Forces the replot
****************************************************************************/

void CPlot::DoPlot()
   { 
    Invalidate();
   }

/****************************************************************************
*                             CPlot::DoAddressPlot
* Inputs:
*       CDC & dc: DC to use
* Result: void
*       
* Effect: 
*       Plots the addresses for the seek, based on the current value
****************************************************************************/

void CPlot::DoAddressPlot(CDC & dc, CArray<LONGLONG, LONGLONG&> & positions)
    {
     CRect r;
     GetClientRect(&r);

     int n = (int)positions.GetSize();

     //****************************************************************
     // Plot the addresses
     //****************************************************************
     if(n > 0)
        { /* has positions */
         CSaveDC save(dc);
         CArray<CPoint, CPoint&>points;
         points.SetSize(n);

         int y = 0;
         for(int i = 0; i < n; i++)
            { /* plot seek */
             points[i] = CPoint(i, (int)positions[i]);
             y = max(y, (int)positions[i]);
            } /* plot seek */

         dc.SetMapMode(MM_ANISOTROPIC);
         y = max(y, 1);
         dc.SetWindowExt(n, y);
         dc.SetViewportOrg(0, r.Height());
         dc.SetViewportExt(r.Width(), -r.Height());
#define ADDR_COLOR RGB(155, 155, 255)
         CPen pen(PS_SOLID, 0,  ADDR_COLOR);
         dc.SelectObject(&pen);

         dc.Polyline(points.GetData(), n);
        } /* has positions */

    } // CPlot::DoAddressPlot

/****************************************************************************
*                             CPlot::DoScatterPlot
* Inputs:
*       CDC & dc: DC to use
* Result: void
*       
* Effect: 
*       Plots the scatter-plot of received time (x) vs. sent time (y)
****************************************************************************/

void CPlot::DoScatterPlot(CDC & dc) 
    { 
     CRect r;
     GetClientRect(&r);

     int n = (int)data.Sequence.GetSize();

#define SCATTER_COLOR RGB(0,0,0)
     CPen pen(PS_SOLID, 0, SCATTER_COLOR);
     CBrush br(SCATTER_COLOR);

     //****************************************************************
     // Plots the arrival-time (x) against the send time (y)
     //****************************************************************

     if(n > 0)
        { /* plot sequence */
         CSaveDC save(dc);
         CArray<CPoint, CPoint&>points;
         points.SetSize(n);

         dc.SelectObject(&pen);
         dc.SelectObject(&br);

         //****************************************************************
         // Plot the "synchronous" line of 1:1 mapping from arrival to
         // departure
         //
         // Everything above this line arrived early
         // Everything below this line arrived late
         //****************************************************************

         { /* synch line */
          CFont pf; // declare before CSaveDC
          CSaveDC save(dc);
          CArray<CPoint, CPoint&>points;
          points.SetSize(n);

          dc.SetMapMode(MM_ANISOTROPIC);
          dc.SetWindowExt(n, n);
          dc.SetViewportOrg(0, r.Height());
          dc.SetViewportExt(r.Width(), -r.Height());

         //****************************************************************
         // Having set the coordinate transforms, compute the correct
         // font size
         //****************************************************************

          CFont * f = GetFont();

          ASSERT(f != NULL);
          if(f == NULL)
             return;

          LOGFONT lf;
          f->GetLogFont(&lf);

          VERIFY(pf.CreatePointFont(120, lf.lfFaceName, &dc));

#ifdef _DEBUG // see the properties of the actual font
          LOGFONT plf;
          pf.GetLogFont(&plf);
#endif
          dc.SelectObject(&pf);

          for(int i = 0; i < n; i++)
             { /* plot synch line */
              points[i] = CPoint(i, i);
             } /* plot synch line */

#define SYNCH_COLOR RGB(255, 0, 0)
          CPen pen(PS_SOLID, 0, SYNCH_COLOR);
          dc.SelectObject(&pen);
          dc.Polyline(points.GetData(), n);

          dc.SetTextColor(SYNCH_COLOR);
          dc.SetBkMode(TRANSPARENT);

          CPoint pt;
          CString s;

          s.LoadString(IDS_EARLY);
          dc.SetTextAlign(TA_TOP | TA_LEFT);
          pt = CPoint(0, 0);
          dc.DPtoLP(&pt);
          dc.TextOut(pt.x, pt.y, s);

          s.LoadString(IDS_LATE);
          dc.SetTextAlign(TA_BOTTOM | TA_RIGHT);
          pt = CPoint(r.right, r.bottom);
          dc.DPtoLP(&pt);
          dc.TextOut(pt.x, pt.y, s);
         } /* synch line */

         //****************************************************************
         // Now plot the data of actual arrival time to send time
         //****************************************************************

         //================================================================
         // There is a graphics trick here
         // We first compute the points in virtual space
         // then render them in device space
         //================================================================
         CArray<int, int>polycounts;

         HitTestArray.SetSize(n); // set the collection of rectangles

         { /* plot scatter */
          CSaveDC save(dc);
          
#define NPOINTS 5
          points.SetSize(NPOINTS * n); 
           
          { /* plot in logical space */
           dc.SetMapMode(MM_ANISOTROPIC);
           dc.SetWindowExt(n * (NPOINTS - 1), n);
           dc.SetViewportOrg(0, r.Height());
           dc.SetViewportExt(r.Width(), -r.Height());

           polycounts.SetSize(n);

           for(int i = 0; i < n; i++)
              { /* scan points */
               //================================================================
               //
               //   +-------+   
               //   |       |
               //   |   *   |   <= i, data.Sequence[i]
               //   |       |
               //   +-------+
               //================================================================
               // Now, the trick here is to make square blocks no matter what the
               // aspect ratio of the display is.  So we first figure out which is the
               // smaller dimension

               int k = NPOINTS * i;
               points[k]      = CPoint( (i * 4), data.Sequence[i]);
               points[k + 1]  = CPoint( (i * 4), data.Sequence[i]);
               points[k + 2]  = CPoint( (i * 4), data.Sequence[i]);
               points[k + 3]  = CPoint( (i * 4), data.Sequence[i]);
               points[k + 4]  = points[k];
               polycounts[i] = NPOINTS; 
#if defined(_DEBUG) && 0
               CPoint pt = points[k];
               dc.LPtoDP(&pt);
               LogMessage(new TraceFormatComment(TraceEvent::None, _T("LPdoDP:[%d] (%d, %d) => (%d, %d)"), i, points[k].x, points[k].y, pt.x, pt.y));
#endif
               dc.LPtoDP(&points[k]);
               dc.LPtoDP(&points[k+1]);
               dc.LPtoDP(&points[k+2]);
               dc.LPtoDP(&points[k+3]);
               dc.LPtoDP(&points[k+4]);
              } /* scan points */

          } /* plot in logical space */

          for(int i = 0; i < n; i++)
             { /* adjust widths */
              // Another trick here: remember that the graphcs all draw to
              // limit - 1, so to get square items, you need to make the
              // boundaries one larger
              int k = i * NPOINTS;
               //================================================================
               // k,k+4      k+1
               //   +-------+   
               //   |       |
               //   |   *   |   <= i, data.Sequence[i]
               //   |       |
               //   +-------+
               //   k+3     k+2
               //================================================================
#define PT_WIDTH 2
              points[k].x -= PT_WIDTH;
              points[k].y -= PT_WIDTH;
              points[k+1].x += PT_WIDTH + 1;
              points[k+1].y = points[k].y;
              points[k+2].x = points[k+1].x;
              points[k+2].y += PT_WIDTH + 1;
              points[k+3].x = points[k].x;
              points[k+3].y = points[k+2].y;
              points[k+4] = points[k];
              //================================================================
              // To support mouse hit testing, create an array of CRects
              // that delimite each of the rectangles
              //================================================================
              HitTestArray[i] = CRect(points[k].x, points[k].y, points[k+2].x, points[k+2].y);
              HitTestArray[i].NormalizeRect();   
#ifdef _DEBUG
              LogMessage(new TraceFormatComment(TraceEvent::None,
                                                _T("HitTest [%d] (%d, %d, %d, %d)"), 
                                                i,
                                                HitTestArray[i].left,
                                                HitTestArray[i].top,
                                                HitTestArray[i].right,
                                                HitTestArray[i].bottom));
#endif
             } /* adjust widths */
         } /* plot scatter */


         //================================================================
         // Avoid this situation by setting WINDING mode
         //
         //     +------------+
         //     |############|
         //     |############|
         //     |######+-----+-------+
         //     |######|     |#######|
         //     |######|     |#######|
         //     |######|     |#######|
         //     |######|     |#######|
         //     +------+-----+#######|
         //            |#############|
         //            |#############|
         //            +-------------+
         dc.SetPolyFillMode(WINDING);
         dc.PolyPolygon(points.GetData(), polycounts.GetData(), n);
        } /* plot sequence */
    } // CPlot::DoScatterPlot

#define SCALING 1000
#define MICROSECONDS 1000000

/****************************************************************************
*                                CPlot::units
* Inputs:
*       double t: Time, expressed in microseconds
* Result: CString
*       String of the form
*               nnn us
*               nnn ms
*               nnn s
****************************************************************************/

CString CPlot::units(double usec)
    {
     static const LPCTSTR scales[] = { _T("us"), _T("ms"), _T("s"), NULL };
     CString result;
     int time = (int)usec;

     for(int i = 0; scales[i] != NULL; i++)
        { /* compute */
         result.Format(_T("%d%s"), time, scales[i]);
         if(time < 1000)
            break;
         time /= 1000;
        } /* compute */
     return result;
    } // CPlot::units

/****************************************************************************
*                               CPlot::DoTimePlot
* Inputs:
*       CDC & dc: DC to use
* Result: void
*       
* Effect: 
*       Does a time plot
****************************************************************************/

void CPlot::DoTimePlot(CDC & dc)
    {
     CRect r;
     GetClientRect(&r);
     CArray<LONGLONG, LONGLONG&>positions;

     int n = (int)data.Sequence.GetSize();

     //****************************************************************
     // Fill the array in based on the sending time (Sequence array)
     //****************************************************************

     CArray<double, double> sorted;

     sorted.SetSize(n);
     positions.SetSize(n);

     //----------------------------------------------------------------
     //                          _            _
     //          _n_            |   _n_        |2
     //         \               |  \           |
     //       n  >      x[i]² - |   >     x[i] |
     //         /___            |  /___        |
     //         i = 1           |_ i = 1      _|
     //  S² = -------------------------------------------
     //                  n (n - 1)
     //----------------------------------------------------------------

     double sumx2 = 0.0;
     double sumx = 0.0;

     for(int i = 0; i < n; i++)
        { /* compute mean, sd, sorted list */
         sumx2 += data.Times[i] * data.Times[i];
         sumx  += data.Times[i];
         sorted[data.Sequence[i]] = data.Times[i];
         positions[data.Sequence[i]] = data.SeekPositions[i];
        } /* compute mean, sd, sorted list */

     double sq = 0.0;
     double mean = 0.0;

     if(n == 1)
        mean = data.Times[0];  // this should never happen

     if(n > 1)
        { /* mean & sd */
         sq = (n * sumx2 - (sumx * sumx)) / (n * (n - 1));
         mean = sumx / (double)n;
        } /* mean & sd */

     double sd = sqrt(sq);

     //****************************************************************
     // Compute the maximum range
     //****************************************************************

     double maxTime = 0.0;
     double minTime = 1.0E50;

     for(int i = 0; i < n; i++)
        { /* compute max */
         maxTime = max(maxTime, data.Times[i]);
         minTime = min(minTime, data.Times[i]);
        } /* compute max */

     double maxlog = log10(maxTime * MICROSECONDS);
     maxlog = ceil(maxlog);

     int maxY = (int)maxlog * SCALING; // scaled integer value for maximum rate
     if(maxY == 0)
         return; // not interesting, no data

     //****************************************************************
     // Now draw the points
     //****************************************************************

     CFont pf; // must declare before CSaveDC
     
     CSaveDC save(dc);

     // Set up the coordinates
     dc.SetMapMode(MM_ANISOTROPIC);
     dc.SetWindowExt(n, (int)maxY);
     dc.SetViewportOrg(0, r.Height());
     dc.SetViewportExt(r.Width(), -r.Height());
     
     //****************************************************************
     // Having set the coordinate transforms, compute the correct
     // font size
     //****************************************************************

     CFont * f = GetFont();

     ASSERT(f != NULL);

     LOGFONT lf;
     f->GetLogFont(&lf);

     VERIFY(pf.CreatePointFont(90, lf.lfFaceName, &dc));

     dc.SelectObject(&pf);

     //****************************************************************
     // Draw the mean and standard deviation in the background
     //****************************************************************
     { /* sd */
      double low  = log10( (mean - sd) * MICROSECONDS);
      double high = log10( (mean + sd) * MICROSECONDS);
      CRect r(0, (int)(low * SCALING), n, (int)(high * SCALING));
      dc.FillSolidRect(&r, RGB(255, 255, 0));

      CPen meanpen(PS_DASH, 0, RGB(0, 128, 0));
      // Note that the CPen must *precede* the CSaveDC

      CSaveDC save(dc);
      dc.SelectObject(&meanpen);
      double m = log10( mean * MICROSECONDS );
      dc.MoveTo(0, (int)(m * SCALING));
      dc.LineTo(n, (int)(m * SCALING));
     } /* sd */

     DoAddressPlot(dc, positions);

     //****************************************************************
     // Draw the horizontal gridlines
     //****************************************************************
     { /* gridlines */
      CSaveDC save(dc);
      dc.SetTextAlign(TA_BOTTOM);
      dc.SetBkMode(TRANSPARENT);
      dc.SetTextColor(RGB(255, 0, 0));
      
      for(int i = 0; i < maxlog; i++)
         { /* y-lines */
          dc.MoveTo(0, i * SCALING);
          dc.LineTo(n, i * SCALING);
          double val = pow(10.0, (double)i);
          CString s;
          s = units(val);
          dc.TextOut(0, i * SCALING, s);
         } /* y-lines */
     } /* gridlines */

     CArray<CPoint, CPoint>points;
     points.SetSize(n);

     for(int i = 0; i < n; i++)
        { /* display times */
         double L = log10(sorted[i] * MICROSECONDS);
         int y = (int) (L * SCALING);
         points[i] = CPoint(i, y);
        } /* display times */

     dc.Polyline(points.GetData(), n);

     CString fmt;
     fmt.LoadString(IDS_TIMESTATS);
     // Max time %s\nMin time %s\nAverage time %s ±%s
     CString maxt = units(maxTime * MICROSECONDS);
     CString mint = units(minTime * MICROSECONDS);
     CString meant = units(mean * MICROSECONDS);
     CString sdt = units(sd * MICROSECONDS);
     
     TimeStats.Format(fmt, maxt, mint, meant, sdt);
    } // CPlot::DoTimePlot

/****************************************************************************
*                               CPlot::OnPaint
* Result: void
*       
* Effect: 
*       Plots the data
* Notes:
*       Plots the sequence of completions
*       Plots the record offset
****************************************************************************/

void CPlot::OnPaint() 
    {
     CPaintDC dc(this); // device context for painting  

     //========================================
     // Because static controls use CS_OWNDC
     // we need to set an explicit clipping
     // region to prevent spillover
     //========================================
     CRect r;
     GetClientRect(&r);
     CRgn rgn;
     rgn.CreateRectRgn(r.left, r.top, r.right, r.bottom);

     CSaveDC save(dc);

     dc.SelectClipRgn(&rgn);

     switch(CurrentMode)
        { /* plot mode */
         case ScatterPlot:
            DoAddressPlot(dc, data.SeekPositions);
            DoScatterPlot(dc);
            break;
         case TimePlot:
            DoTimePlot(dc);
            break;
        } /* plot mode */

     // Do not call CStatic::OnPaint() for painting messages
    }

/****************************************************************************
*                                PlotData::Write
* Inputs:
*       CStdioFile & f: File to write to
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Writes the plot data
* Notes:
*      <XML ...>
*      <PLOT:Plot>
*         <PLOT:Parameters>
*            <PLOT:Filename "name"/>
*            <PLOT:FILE_FLAG_NO_BUFFERING "0"/>
*            <PLOT:FILE_FLAG_WRITE_THROUGH "0"/>
*            <PLOT:FILE_FLAG_RANDOM_ACCESS "0"/>
*            <PLOT:FILE_FLAG_SEQUENTIAL_SCAN "0"/>
*            <PLOT:NREQUESTS "100"/>
*         </PLOT:Parameters>
*         <PLOT:DATA>
*            <PLOT:SEEK>
*               <PLOT:ADDRESS "12345"/>
*               <PLOT:ADDRESS "45678"/>
*            </PLOT:SEEK>
*         </PLOT:DATA>
*      </PLOT:Plot>
****************************************************************************/

BOOL PlotData::Write(CStdioFile & f) 
    {
     f.WriteString(_T("<?XML version \"1.0\"?>\n"));
     f.WriteString(_T("<PLOT:Plot>\n")); 
        f.WriteString(_T("    <PLOT:Parameters>\n"));
        f.WriteString(_T("    </PLOT:Parameters>\n")); 
        f.WriteString(_T("    <PLOT:Data>\n"));
            f.WriteString(_T("        <PLOT:Seek>"));
            for(int i = 0; i < SeekPositions.GetSize(); i++)
               { /* seek positions */
                f.WriteString(_T("\n            "));
                CString s;
                s.Format(_T("<PLOT:Address i \"%d\" offset \"%7d\"/>"), i, SeekPositions[i]);
                f.WriteString(s);
               } /* seek positions */
            f.WriteString(_T("\n        </PLOT:Seek>\n"));
            f.WriteString(_T("\n        <PLOT:Sequence\n"));
            for(int i = 0; i < Sequence.GetSize(); i++)
               { /* sequence positions */
                f.WriteString(_T("\n            "));
                CString s;
                s.Format(_T("<PLOT:Seq i \"%d\" src \"%d\"/>"), i, Sequence[i]);
                f.WriteString(s);
               } /* sequence positions */
            f.WriteString(_T("\n        </PLOT:Sequence\n"));

            f.WriteString(_T("\n        <PLOT:Times\n"));
            for(int i = 0; i < Times.GetSize(); i++)
               { /* times */
                f.WriteString(_T("\n            "));
                CString s;
                s.Format(_T("<PLOT:Time received \"%d\" sent \"%d\" time \"%9.6f\" record \"%d\""), i, Sequence[i], Times[i], SeekPositions[i]);
                f.WriteString(s);
               } /* times */
            f.WriteString(_T("\n        </PLOT:Times\n"));
        f.WriteString(_T("    </PLOT:Data>\n"));
     f.WriteString(_T("</PLOT:Plot>\n"));
     return TRUE;
    } // PlotData::Write

/****************************************************************************
*                             CPlot::OnEraseBkgnd
* Inputs:
*       CDC * pDC: DC
* Result: BOOL
*       TRUE, always
* Effect: 
*       Erases the plot background
****************************************************************************/

BOOL CPlot::OnEraseBkgnd(CDC* pDC)
    {
     CRect r;
     GetClientRect(&r);

     pDC->FillSolidRect(&r, ::GetSysColor(COLOR_3DFACE));
     return TRUE;
    }

/****************************************************************************
*                            CPlot::SetDisplayMode
* Inputs:
*       PlotType type: Type of plot to use
* Result: void
*       
* Effect: 
*       Sets the display mode
****************************************************************************/

void CPlot::SetDisplayMode(PlotType type)
    {
     if(CurrentMode == type)
        return; // already established
     CurrentMode = type;
     Invalidate();
    } // CPlot::SetDisplayMode

/****************************************************************************
*                               PlotData::WriteCSV
* Inputs:
*       CStdioFile & file: File to write to
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Writes the file as csv with header
****************************************************************************/

BOOL PlotData::WriteCSV(CStdioFile & file)
    { 
     file.WriteString(_T("Received,    Sent,     Time,  Record\n"));
     //  |Received,    Sent,      Time,  Record
     //  |  dddddd,  dddddd,  1000.000, nnnnnnn

     for(int i = 0; i < Times.GetSize(); i++)
        { /* times */
         CString s;
         s.Format(_T("  %6d, %6d, %9.3f, %8d\n"), i, Sequence[i], Times[i] * 1000.0, SeekPositions[i]);
         file.WriteString(s);
        } /* times */
     return TRUE;
    } // PlotData::WriteCSV

/****************************************************************************
*                             CPlot::OnMouseMove
* Inputs:
*       UINT nFlags: ignored
*       CPoint point: Used to pop up info box
* Result: void
*       
* Effect: 
*       Pops up an info box appropriate for the display
****************************************************************************/

void CPlot::OnMouseMove(UINT nFlags, CPoint point)
    {

     switch(CurrentMode)
        { /* mode */
         case TimePlot:
            // Max time %s\n
            // Min time %s\n
            // Average time %s ±%s
            if(TimeStats.IsEmpty())
               return;
            
            SetInfoWindow(point, TimeStats, TRUE);
            break;
         case ScatterPlot:
            { /* scatter */
             CString ScatterStats;
             // Seek record %d 
             // Issued at T=%d
             // Received at T=%d
             
             int hit = HitTest(point);
             if(hit >= 0) 
                { /* hit */
                 CString fmt;
                 fmt.LoadString(IDS_SCATTER_STATS);
                 ScatterStats.Format(fmt, (int)data.SeekPositions[hit],
                                     data.Sequence[hit],
                                     hit);
                } /* hit */
             else
                { /* no hit */
                } /* no hit */
             SetInfoWindow(point, ScatterStats, hit >= 0);
            } /* scatter */
            break;
        } /* mode */

    }

/****************************************************************************
*                             CPlot::OnMouseLeave
* Inputs:
*       WPARAM:
*       LPARAM:
* Result: LRESULT
*       Logically void, 0, alwyas
* Effect: 
*       Destroys the popup window
****************************************************************************/

LRESULT CPlot::OnMouseLeave(WPARAM, LPARAM)
    { 
     if(info != NULL)
        { /* get rid of it */
         info->DestroyWindow();
#if defined(_DEBUG)
         LogMessage(new TraceFormatComment(TraceEvent::None, _T("WM_MOUSELEAVE")));
#endif
        } /* get rid of it */
     info = NULL;
     return 0;
    } // CPlot::OnMouseLeave

/****************************************************************************
*                              CPlot::LogMessage
* Inputs:
*       TraceEvent * e: Trace event
* Result: void
*       
* Effect: 
*       Asks the parent to log the message
****************************************************************************/

void CPlot::LogMessage(TraceEvent * e)
    {
     GetParent()->PostMessage(UWM_LOG, (WPARAM)e);
    } // CPlot::LogMessage

/****************************************************************************
*                               CPlot::HitTest
* Inputs:
*       CPoint pt: Point in client coordinates
* Result: int
*       Index of hit for subsequent operations
*       -1 if not found
****************************************************************************/

int CPlot::HitTest(CPoint pt) 
    {
     for(int i = 0; i < HitTestArray.GetSize(); i++)
        { /* scan values */
         if(HitTestArray[i].PtInRect(pt))
            return i;
        } /* scan values */
     return -1;
    } // CPlot::HitTest

/****************************************************************************
*                            CPlot::SetInfoWindow
* Inputs:
*       CPoint point: Point in client coordinates
*       const CString & stats: String to display
* Result: void
*       
* Effect: 
*       Creates/updates the info window
****************************************************************************/

void CPlot::SetInfoWindow(CPoint point, const CString & stats, BOOL visible)
    {
     CPoint target = point;
     ClientToScreen(&target);
     target.y += ::GetSystemMetrics(SM_CYCURSOR); // height of cursor

     if(info != NULL)
        { /* move window */
         CString old;
         info->GetWindowText(old);
         if(old != stats)
             info->SetWindowText(stats);

         info->SetWindowPos(NULL, target.x, target.y,
                            0, 0,
                            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
         info->ShowWindow(visible ? SW_SHOWNA : SW_HIDE);

#if defined(_DEBUG)
         LogMessage(new TraceFormatComment(TraceEvent::None, _T("WM_MOUSEMOVE client(x %d, y %d), screen (x %d, y %d) moved"), point.x, point.y, target.x, target.y));
#endif
        } /* move window */
     else
        { /* create window */
#if defined(_DEBUG)
         LogMessage(new TraceFormatComment(TraceEvent::None, _T("WM_MOUSEMOVE(x %d, y %d) created"), target.x, target.y));
#endif
         info = new CInfoDisplay;
         if(!info->Create(target.x, target.y, stats, this))
            { /* failed */
             delete info;
             return;
            } /* failed */
         info->SetWindowPos(NULL, target.x, target.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
         info->ShowWindow(visible ? SW_SHOWNA : SW_HIDE);

         TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
         VERIFY(TrackMouseEvent(&tme));
        } /* create window */
    } // CPlot::SetInfoWindow
