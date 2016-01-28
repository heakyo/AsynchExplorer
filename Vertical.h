#pragma once


// CVertical

class CVertical : public CStatic
{
        DECLARE_DYNAMIC(CVertical)

public:
        CVertical();
        virtual ~CVertical();
        void SetWindowText(LPCTSTR s) { CStatic::SetWindowText(s); Invalidate(); }

protected:
        DECLARE_MESSAGE_MAP()
protected: // methods
        afx_msg void OnPaint();
        afx_msg LRESULT OnSetFont(WPARAM, LPARAM);
        afx_msg LRESULT OnGetFont(WPARAM, LPARAM);
        afx_msg LRESULT OnSetText(WPARAM, LPARAM);
        afx_msg LRESULT OnGetText(WPARAM, LPARAM);
        afx_msg LRESULT OnGetTextLength(WPARAM, LPARAM);
protected: // data
        CBrush brush;
        HFONT font;
        CString text;
public:
    afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};


