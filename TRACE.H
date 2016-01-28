// TraceList.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTraceList window
#define MIN_LOG_ENTRIES 50

class CTraceList : public CListBox
{
// Construction
public:
        CTraceList();

// Attributes
public:
        void AddMessage(TraceEvent * event);
        int AddString(TraceEvent * event); 
        int InsertString(int index, TraceEvent * event) 
                { modified = TRUE; return CListBox::InsertString(index, (LPCTSTR)event); }
        int DeleteString(int n) { modified = TRUE; return CListBox::DeleteString(n); }
        void ResetContent() { modified = TRUE; 
                              maxwidth = 0; 
                              CListBox::ResetContent(); }
        void SetModified(BOOL mode = TRUE) { modified = mode; }
        BOOL GetModified() { return modified; }
        void setBoundary(int n) { boundary = n;
                                  if(limit < boundary + MIN_LOG_ENTRIES)
                                        limit = boundary + MIN_LOG_ENTRIES; }
// Operations
public:
        void doSave(BOOL mode);
        BOOL toNext();
        BOOL toPrev();
        void toTop();
        void toEnd();
        BOOL isNext();
        BOOL isPrev();
        BOOL isTop();
        BOOL isEnd();
// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CTraceList)
        public:
        virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
        virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
        virtual void DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct);
        //}}AFX_VIRTUAL

// Implementation
public:
        virtual ~CTraceList();
        CFont * fixed;
        int height;
        CString getFileName() { return SaveFileName; }
        void setLimit(int limit);
        void setToDisk(BOOL v);
        // Generated message map functions
protected:
        DWORD deleted;                                              // REQ #010
        int boundary;
        int findNext(int i);
        int findPrev(int i);
        UINT maxwidth;
        void appendToFile(TraceEvent * event);
        BOOL toDisk;
        BOOL isVisible(int n, BOOL fully);
        BOOL modified; 
        CString SaveFileName;
        int limit;
        BOOL limited;

        //{{AFX_MSG(CTraceList)
        afx_msg LRESULT OnMessage(WPARAM, LPARAM);
        //}}AFX_MSG

        DECLARE_MESSAGE_MAP()
};

#define TLM_UPDATE_ERRORS (WM_USER + 200)

/////////////////////////////////////////////////////////////////////////////
