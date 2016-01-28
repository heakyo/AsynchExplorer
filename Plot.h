#pragma once

#include "InfoDisplay.h"
#include "TraceEvent.h"

class PlotData {
    public:
       BOOL m_FILE_FLAG_SEQUENTIAL_SCAN;
       BOOL m_FILE_FLAG_RANDOM_ACCESS;
       BOOL m_FILE_FLAG_WRITE_THROUGH;
       BOOL m_FILE_FLAG_NO_BUFFERING;
    public: // methods
       CArray<LONGLONG, LONGLONG&> SeekPositions;
       CArray<DWORD, DWORD&> Sequence;
       CArray<double, double> Times;
    public:
       void SetSamples(int n) { SeekPositions.SetSize(n); Sequence.SetSize(n); Times.SetSize(n); }
    public: // methods
       BOOL Write(CStdioFile & f);
       BOOL WriteCSV(CStdioFile & f);
};

// CPlot

class CPlot : public CStatic
{
        DECLARE_DYNAMIC(CPlot)

public:
        CPlot();
        virtual ~CPlot();
public: // data
        PlotData data;
        typedef enum { ScatterPlot, TimePlot } PlotType;
public: // methods
        void DoPlot();
        void SetDisplayMode(PlotType type);
        void ShowData(int show);
        void SetSamples(int n) { data.SetSamples(n); if(GetSafeHwnd() != NULL) Invalidate(); }
        void LogMessage(TraceEvent * e);
        int HitTest(CPoint pt);
protected:
        //======================================
        // Hit-test array, in client coordinates
        //======================================
        CArray<CRect, const CRect&> HitTestArray;

        void SetInfoWindow(CPoint point, const CString & stats, BOOL visible);
        PlotType CurrentMode;
        void DoAddressPlot(CDC & dc, CArray<LONGLONG, LONGLONG&> & positions);
        void DoScatterPlot(CDC & dc);
        void DoTimePlot(CDC & dc);
        CString units(double usec);

        CInfoDisplay * info;
        CString TimeStats;
        
        DECLARE_MESSAGE_MAP()
              
protected:
    afx_msg void OnPaint();
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
    afx_msg LRESULT OnMouseLeave(WPARAM, LPARAM);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};


