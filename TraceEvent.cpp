/*****************************************************************************
*           Change Log
*  Date     | Change
*-----------+-----------------------------------------------------------------
* 26-Dec-99 | [1.387] Created
* 26-Dec-99 | [1.387] REQ #134: Minor fixes from LINT run
* 24-Jan-00 | REQ #140: try for string with same value as error code
* 24-Jan-00 | REQ #140: try for string with same value as error code
* 28-Feb-06 | REQ #141: Use FormatV
*****************************************************************************/
#include "stdafx.h"
#include "TraceEvent.h"
#include "resource.h"

IMPLEMENT_DYNAMIC(TraceEvent, CObject)
IMPLEMENT_DYNAMIC(TraceShowText, TraceEvent)
IMPLEMENT_DYNAMIC(TraceConnect, TraceEvent)
IMPLEMENT_DYNAMIC(TraceComment, TraceEvent)
IMPLEMENT_DYNAMIC(TraceFormatComment, TraceComment)                 // REQ #154
IMPLEMENT_DYNAMIC(TraceError, TraceEvent)
IMPLEMENT_DYNAMIC(TraceFormatError, TraceError)                     // REQ #154
IMPLEMENT_DYNAMIC(TraceFormatMessage, TraceError)                   // REQ #154
IMPLEMENT_DYNAMIC(TraceLock, TraceEvent)
IMPLEMENT_DYNAMIC(TraceSpeech, TraceComment)
IMPLEMENT_DYNAMIC(TraceFormatInternet, TraceFormatComment)          // REQ #138
IMPLEMENT_DYNAMIC(TraceFormatMIDI, TraceFormatComment)
const UINT TraceEvent::None = (UINT)-1;
UINT TraceEvent::counter = 0;

/****************************************************************************
*                            TraceEvent::showfile
* Result: CString
*       Nicely formatted string for writing to file
* Effect: 
*       Prepares a line for file output; uses fixed-width fields for the prefix
* Notes:
*       Cleverly uses the virtual show() method to get the rest of the string
****************************************************************************/

CString TraceEvent::showfile()
    {
     CString t;
     if(filterID != None)
        t.Format(_T("%03d  "), filterID);
     else
        t = _T("     ");

     CString s;
     s.Format(_T("%03d| "), seq);
     return s + t + showThread() + show();
     
    }

/****************************************************************************
*                              TraceEvent::show
* Result: CString
*       
* Effect: 
*       Formats the base string for the trace event
****************************************************************************/

CString TraceEvent::show()
    {
     CString s = time.Format(_T("%H:%M:%S  "));
     return s;
    }

/****************************************************************************
*                             TraceEvent::showID
* Result: CString
*       The trace ID, or an empty string
* Effect: 
*       Formats the trace ID.  Connectionless events have no id
****************************************************************************/

CString TraceEvent::showID()
    {
     CString t;
     if(filterID != None)
        t.Format(_T("%03d"), filterID);
     else
        t = _T("");

     return t;
    }

/****************************************************************************
*                       CString TraceEvent::showThread
* Result: CString
*       Thread ID, formatted
****************************************************************************/

CString TraceEvent::showThread()
    {
     CString s;
     s.Format(_T("%08x "), threadID);
     return s;
    }

/****************************************************************************
*                             TraceEvent::cvtext
* Inputs:
*       CString s: String to convert
* Result: CString
*       String formatted for printout
****************************************************************************/

CString TraceEvent::cvtext(const CString &s) // REQ #134
    {
     CString str;
     for(int i = 0; i < s.GetLength(); i++)
        { /* convert to printable */
         str += cvtext((BYTE)s[i]);                                 // REQ #134
        } /* convert to printable */
     return str;     
    }

/****************************************************************************
*                            TraceEvent::cvtext
* Inputs:
*       BYTE ch: Character to display (8-bit)
* Result: CString
*       Displayable string
****************************************************************************/

CString TraceEvent::cvtext(BYTE ch)
    {
     if(ch >= ' ' && ch < 0177)
        { /* simple char */
         return CString((char)ch); // NYI: handle Unicode
        } /* simple char */

     CString s;
     switch(ch)
        { /* ch */
         case '\a':
                 return CString(_T("\\a"));
         case '\b':
                 return CString(_T("\\b"));
         case '\f':
                 return CString(_T("\\f"));
         case '\n':
                 return CString(_T("\\n"));
         case '\r':
                 return CString(_T("\\r"));
         case '\t':
                 return CString(_T("\\t"));
         case '\v':
                 return CString(_T("\\v"));
         case '\\':
                 return CString(_T("\\\\"));
         case '\"':
                 return CString(_T("\\\""));
         default:
                 s.Format(_T("\\x%02x"), ch);
                 return s;
        } /* ch */
    }

/****************************************************************************
*                             TraceComment::show
* Result: CString
*       Display string
****************************************************************************/

CString TraceComment::show()
    {
     return TraceEvent::show() + comment;
    }

/****************************************************************************
*                             TraceError::show
* Result: CString
*       Display string
****************************************************************************/

CString TraceError::show()
    {
     return TraceEvent::show() + Error;
    }

/****************************************************************************
*                     TraceFormatMessage::TraceFormatMessage
* Inputs:
*       UINT id:
*       DWORD error:
* Effect: 
*       Constructor
****************************************************************************/

TraceFormatMessage::TraceFormatMessage(UINT id, DWORD err) : TraceError(id)// REQ #154
    {
     LPTSTR s;
     if(::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        err,
                        0,
                        (LPTSTR)&s,
                        0,
                        NULL) == 0)
        { /* failed */
         // See if it is a known error code
         CString t;
         t.LoadString(err);                                         // REQ #140
         if(t.GetLength() == 0 || t[0] != _T('Ë'))                  // REQ #140
            { /* other error */                                     // REQ #139
             CString fmt;                                           // REQ #139
             fmt.LoadString(IDS_UNKNOWN_ERROR);                     // REQ #139
             t.Format(fmt, err, err);                               // REQ #139
            } /* other error */                                     // REQ #140
         else                                                       // REQ #140
            if(t.GetLength() > 0 && t[0] == _T('Ë'))                // REQ #140
               { /* drop prefix */                                  // REQ #140
                t = t.Mid(1);                                       // REQ #140
               } /* drop prefix */                                  // REQ #140
         Error = t;
        } /* failed */
     else
        { /* success */
         LPTSTR p = _tcschr(s, _T('\r'));
         if(p != NULL)
            { /* lose CRLF */
             *p = _T('\0');
            } /* lose CRLF */
         Error = s;
         ::LocalFree(s);
        } /* success */
    }

/****************************************************************************
*                    TraceFormatComment::TraceFormatComment
* Inputs:
*       UINT id: ID for line
*       CString fmt: Formatting string
*       ...: Arguments for formatting
* Effect: 
*       Constructor
****************************************************************************/
                                                                    // REQ #154
TraceFormatComment::TraceFormatComment(UINT id, CString fmt, ...)   // REQ #154
           : TraceComment(id, CString(""))                          // REQ #154
    {                                                               // REQ #154
     va_list args;                                                  // REQ #154
     va_start(args, fmt);                                           // REQ #154
     comment.FormatV(fmt, args);                                    // REQ #141
     va_end(args);                                                  // REQ #154
    } // TraceFormatComment::TraceFormatComment                     // REQ #154

/****************************************************************************
*                   TraceFormatComment::TraceFormatComment
* Inputs:
*       UINT id: Line number
*       UINT fmt: ID of formatting string
*       ...: Arguments
* Effect: 
*       Creates a trace comment entry, formatted
****************************************************************************/

TraceFormatComment::TraceFormatComment(UINT id, UINT fmt, ...)      // REQ #154
             : TraceComment(id, CString(""))                        // REQ #154
    {                                                               // REQ #154
     va_list args;                                                  // REQ #154
     va_start(args, fmt);                                           // REQ #154
     CString fmtstr;                                                // REQ #154
     fmtstr.LoadString(fmt);                                        // REQ #154
     comment.FormatV(fmtstr, args);                                 // REQ #141
     va_end(args);                                                  // REQ #154
    } // TraceFormatComment::TraceFormatComment                     // REQ #154

/****************************************************************************
*                      TraceFormatError::TraceFormatError
* Inputs:
*       UINT id: ID for line
*       CString fmt: Formatting string
*       ...: Arguments for formatting
* Effect: 
*       Constructor
****************************************************************************/

TraceFormatError::TraceFormatError(UINT id, CString fmt, ...)       // REQ #154
        : TraceError(id, CString(""))                               // REQ #154
    {                                                               // REQ #154
     va_list args;                                                  // REQ #154
     va_start(args, fmt);                                           // REQ #154
     Error.FormatV(fmt, args);                                      // REQ #141
     va_end(args);                                                  // REQ #154
    } // TraceFormatError::TraceFormatError                         // REQ #154

/****************************************************************************
*                   TraceFormatError::TraceFormatError
* Inputs:
*       UINT id: Line number
*       UINT fmt: ID of formatting string
*       ...: Arguments
* Effect: 
*       Creates a trace Error entry, formatted
****************************************************************************/

TraceFormatError::TraceFormatError(UINT id, UINT fmt, ...)          // REQ #154
          : TraceError(id, CString(""))                             // REQ #154
    {                                                               // REQ #154
     va_list args;                                                  // REQ #154
     va_start(args, fmt);                                           // REQ #154
     CString fmtstr;                                                // REQ #154
     fmtstr.LoadString(fmt);                                        // REQ #154
     Error.FormatV(fmtstr, args);                                   // REQ #141
     va_end(args);                                                  // REQ #154
    } // TraceFormatError::TraceFormatError                         // REQ #154

/****************************************************************************
*                   TraceFormatInternet::TraceFormatInternet
* Inputs:
*       UINT id: Line number
*       UINT fmt: ID of formatting string
*       ...: Arguments
* Effect: 
*       Creates a trace comment entry, formatted
****************************************************************************/

TraceFormatInternet::TraceFormatInternet(UINT id, UINT fmt, ...)    // REQ #138
    : TraceFormatComment(id, CString(""))                           // REQ #138
    {                                                               // REQ #138
     va_list args;                                                  // REQ #138
     va_start(args, fmt);                                           // REQ #138
     CString fmtstr;                                                // REQ #138
     fmtstr.LoadString(fmt);                                        // REQ #138
     comment.FormatV(fmtstr, args);                                 // REQ #138
     va_end(args);                                                  // REQ #138
    } // TraceFormatInternet::TraceFormatInternet                   // REQ #138

/****************************************************************************
*                   TraceFormatMIDI::TraceFormatMIDI
* Inputs:
*       UINT id: Line number
*       UINT fmt: ID of formatting string
*       ...: Arguments
* Effect: 
*       Creates a trace comment entry, formatted
****************************************************************************/

TraceFormatMIDI::TraceFormatMIDI(UINT id, UINT fmt, ...)            // REQ #138
     : TraceFormatComment(id, CString(""))                          // REQ #138
    {                                                               // REQ #138
     va_list args;                                                  // REQ #138
     va_start(args, fmt);                                           // REQ #138
     CString fmtstr;                                                // REQ #138
     fmtstr.LoadString(fmt);                                        // REQ #138
     comment.FormatV(fmtstr, args);                                 // REQ #141
     va_end(args);                                                  // REQ #138
    } // TraceFormatMIDI::TraceFormatMIDI                           // REQ #138
