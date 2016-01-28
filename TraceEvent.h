/*****************************************************************************
*           Change Log
*  Date     | Change
*-----------+-----------------------------------------------------------------
* 26-Dec-99 | [1.387] Created
* 26-Dec-99 | [1.387] REQ #134: Minor changes from LINT run
*  8-Feb-00 | REQ #154: Brought up to new TraceEvent features
*****************************************************************************/

#pragma once
/****************************************************************************
*                           class TraceEvent
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
* Notes:
*       This constructor is used only by its subclasses.  This is a virtual
*       class and is never instantiated on its own
****************************************************************************/
/*lint -e1712 */
class TraceEvent : public CObject {
    DECLARE_DYNAMIC(TraceEvent)
protected:
    static UINT counter;
    TraceEvent(UINT id) { time = CTime::GetCurrentTime(); 
                          filterID = id; 
                          threadID = ::GetCurrentThreadId(); 
                          seq = counter++;
                        }
public:
    static const UINT None;
    virtual ~TraceEvent() { }
    virtual COLORREF textcolor() = 0;
    virtual CString show();
    virtual CString showID();
    virtual CString showfile();
    virtual CString showThread();
    virtual int displayHeight() { return 1; }                       // REQ #134
    static CString cvtext(BYTE ch); // useful for various subclasses
    static CString cvtext(const CString &s); // useful for various subclasses // REQ #134
protected:
    CTime time;
    UINT  filterID;
    DWORD threadID;
    UINT  seq;
};



/****************************************************************************
*                           class TraceConnect
* Inputs:
*       UINT id: TraceEvent::None
****************************************************************************/

class TraceConnect: public TraceEvent {
    DECLARE_DYNAMIC(TraceConnect)
public:
    TraceConnect(UINT id): TraceEvent(id) { }
    virtual ~TraceConnect() { }
    virtual COLORREF textcolor() { return RGB(255, 0, 0); }
    virtual CString show();
   };


/****************************************************************************
*                           class TraceComment
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
*       CString s: String to display
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
*       UINT u: IDS_ index of string to display
****************************************************************************/

class TraceComment: public TraceEvent {
    DECLARE_DYNAMIC(TraceComment)
public:
    TraceComment(UINT id, const CString &s) : TraceEvent(id) {comment = s;}// REQ #134
    TraceComment(UINT id, UINT u) : TraceEvent(id){ CString s; s.LoadString(u); comment = s; }
    virtual ~TraceComment() { }
    virtual COLORREF textcolor() { return RGB(0, 128, 0); }
    virtual CString show();
protected:
    CString comment;
   };

/****************************************************************************
*                          class TraceFormatComment
* Inputs:
*       UINT id: Line number
*       CString fmt: Formatting string
*       ...
****************************************************************************/

class TraceFormatComment : public TraceComment                      // REQ #154
   {                                                                // REQ #154
   DECLARE_DYNAMIC(TraceFormatComment)                              // REQ #154
    public:                                                         // REQ #154
      TraceFormatComment(UINT id, CString fmt, ...);                // REQ #154
      TraceFormatComment(UINT id, UINT fmt, ...);                   // REQ #154
   }; // class TraceFormatComment                                   // REQ #154

/****************************************************************************
*                           class TraceShowText
* Inputs:
*       UINT id: ID, or TraceEvent::None
*       LPCTSTR text: Text to show
****************************************************************************/
class TraceShowText: public TraceComment {
   DECLARE_DYNAMIC(TraceShowText)
    public:
      TraceShowText(UINT id, LPCTSTR text) : TraceComment(id, text) {}
      virtual ~TraceShowText() {}
      virtual COLORREF textcolor() { return RGB(0,0, 255); }
    protected:
};


/****************************************************************************
*                              class TraceSpeech
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
*       CString s: String to display
****************************************************************************/

class TraceSpeech : public TraceComment {
      DECLARE_DYNAMIC(TraceSpeech)
          TraceSpeech(UINT id, const CString &s) : TraceComment(id, s) { } // REQ #134
      TraceSpeech(UINT id, UINT u) : TraceComment(id, u) { }
      virtual ~TraceSpeech() { }
      virtual COLORREF textcolor() { return RGB(255, 128, 64); }
    }; // class TraceSpeech


/****************************************************************************
*                           class TraceError
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
*       CString s: String to display
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
*       UINT u: IDS_ token index of string to display
* Notes:
*       The first form is not used directly, only by subclasses
****************************************************************************/

class TraceError: public TraceEvent {
    DECLARE_DYNAMIC(TraceError)
public:
    TraceError(UINT id, const CString &s) : TraceEvent(id) {Error = s; } // REQ #134
    TraceError(UINT id, UINT u) : TraceEvent(id){ CString s; s.LoadString(u); Error = s; }
    virtual ~TraceError() { }
    virtual COLORREF textcolor() { return RGB(192, 0, 0); }
    virtual CString show();
protected:
    TraceError(UINT id) : TraceEvent(id) {}
    CString Error;
   };


/****************************************************************************
*                           class TraceFormatMessage
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
*       DWORD error: Error code from GetLastError()
****************************************************************************/

class TraceFormatMessage: public TraceError {
  DECLARE_DYNAMIC(TraceFormatMessage)
  public:
    TraceFormatMessage(UINT id, DWORD error);
    virtual ~TraceFormatMessage() { }
                                          };

/****************************************************************************
*                               class TraceLock
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
*       UINT u: IDS_ value of string to display
* Inputs:
*       UINT id: numeric ID, or TraceEvent::None
*       CString s: String value to display
****************************************************************************/

class TraceLock: public TraceEvent {
    DECLARE_DYNAMIC(TraceLock)
    public:
        TraceLock(UINT id, UINT u) : TraceEvent(id){ CString s; s.LoadString(u); display = s; }
        TraceLock(UINT id, const CString &s) : TraceEvent(id) { display = s; } // REQ #134
        virtual ~TraceLock() { }
        virtual COLORREF textcolor() { return RGB(255, 0, 0); }
        virtual CString show();
    protected:
        CString display;
                                   };

/****************************************************************************
*                            class TraceFormatError
* Inputs:
*       UINT id: Line number
*       CString fmt: Formatting string
*       ...
****************************************************************************/

class TraceFormatError : public TraceError                          // REQ #154
   {                                                                // REQ #154
   DECLARE_DYNAMIC(TraceFormatError)                                // REQ #154
    public:                                                         // REQ #154
      TraceFormatError(UINT id, CString fmt, ...);                  // REQ #154
      TraceFormatError(UINT id, UINT fmt, ...);                     // REQ #154
   }; // class TraceFormatError                                     // REQ #154

/****************************************************************************
*                          class TraceFormatInternet
* Inputs:
*       UINT id: Line number
*       CString fmt: Formatting string
*       ...
****************************************************************************/

class TraceFormatInternet : public TraceFormatComment               // REQ #138
   {                                                                // REQ #138
    DECLARE_DYNAMIC(TraceFormatInternet)                            // REQ #138
    public:                                                         // REQ #138
      TraceFormatInternet(UINT id, CString fmt, ...);               // REQ #138
      TraceFormatInternet(UINT id, UINT fmt, ...);                  // REQ #138
      virtual COLORREF textcolor() { return RGB(255, 0, 128); }     // REQ #138
   }; // class TraceFormatInternet                                  // REQ #138

/****************************************************************************
*                               class TraceFormatMIDI
****************************************************************************/

class TraceFormatMIDI : public TraceFormatComment
   {
    DECLARE_DYNAMIC(TraceFormatMIDI)                            
    public:                                                  
      TraceFormatMIDI(UINT id, CString fmt, ...);            
      TraceFormatMIDI(UINT id, UINT fmt, ...);           
      virtual COLORREF textcolor() { return RGB(0, 0, 255); }
   }; // class TraceFormatMIDI
