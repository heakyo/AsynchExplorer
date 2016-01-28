/*****************************************************************************
*           Change Log
*  Date     | Change
*-----------+-----------------------------------------------------------------
* 22-Mar-96 | Created
* 24-Nov-98 | REG #001: added DeleteKey/DeleteValue routines
*  3-Jun-99 | REG #002: Overload DWordArray items on id for name
* 24-Jan-00 | REG #003: Fixed string store to use length+1
* 20-Feb-01 | REG #004: Allow \ in the first position for absolute path
* 24-Mar-04 | REG #005: Added registry font support (from a newer version)
* 26-Mar-04 | REG #006: Improved versions of EnumRegistryKeys and 
*           | EnumRegistryValues installed. Various other upgrades copied over
* 10-Apr-04 | REG #007: Handle absolute path creation correctly
* 22-Apr-04 | REG #008: Fixed problem with failure to NUL-terminate incoming string
*  3-May-04 | REG #009: Added Window Placement calls
*  3-May-04 | REG #010: Added FindKey and CreateKey routines
*  3-May-04 | REG #011: Allow programs to obtain the base keypath
*  3-May-04 | REG #012: Use RegOpenKeyEx, added rights to various calls
* 28-Mar-05 | REG #013: SetLastError(ERROR_SUCCESS) when TRUE is ambiguous
* 21-May-05 | REG #014: Zero logfont out before trying to load
* 21-May-05 | REG #015: If required pointer parameter is NULL, return FALSE
* 21-Jun-05 | REG #016: Save font length properly in Unicode mode
*****************************************************************************/
// Copyright © 2005, Joseph M. Newcomer, All Rights Reserved
// You are free to redistribute this providing all copyrights are left intact
// You must record any changes you make to this source by putting // and your
// initials at the end of each line you change. You may not remove any lines
// from this source; instead, put // in front of them and your initials at the end.
// You may not charge any fee for the redistribution, nor incorporate this code in
// any commercial product. This is freeware, and must remain so.
// You are free to use this in any product, commercial or non-commercial, but you may
// not charge for this code in any way.
//
#include "stdafx.h"
#include "registry.h"
#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>
#include "resource.h"

//#ifdef _AFXDLL
//static UINT IDS_PROGRAM_ROOT;
//void SetRegistryRoot(UINT n) { IDS_PROGRAM_ROOT = n; }
//#endif


/****************************************************************************
*                               BuildKeyNameString
* Inputs:
*       const CString & path: Path name
*                       Starts with "\" if absolutepath
* Result: CString
*       The actual path to use for the open
*               If an absolute path was specified, the initial \ is stripped
*               If a relative path was specified, the standard prefix is added
*       The result string will NOT end with a "\" character
****************************************************************************/

static CString BuildKeyNameString(const CString & path)             // REG #007
    {                                                               // REG #007
     CString fullpath;                                              // REG #007
     if(path.IsEmpty() || path[0] != _T('\\'))                      // REG #004
        { /* load default root */                                   // REG #004
         // If you get an "undefined symbol" for this name,         // REG #001
         // you must add a String resource IDS_PROGRAM_ROOT         // REG #001
         // which is of the form                                    // REG #001
         // Software\yourcompanyname\yourprogramname                // REG #001
         //                                                         // REG #001
         fullpath.LoadString(IDS_PROGRAM_ROOT);// See above comment // REG #001
         if(fullpath.Right(1) != _T("\\"))                          // REG #001
            fullpath += _T("\\");                                   // REG #001
        } /* load default root */                                   // REG #004

     fullpath += path;                                              // REG #001
     if(fullpath.Left(1) == _T("\\"))                               // REG #007
        fullpath = fullpath.Mid(1);                                 // REG #007
     if(fullpath.Right(1) == _T("\\"))                              // REG #007
        fullpath = fullpath.Left(fullpath.GetLength() - 1);         // REG #007
     return fullpath;                                               // REG #007
    } // BuildKeyNameString                                         // REG #007

/****************************************************************************
*                                   CreateKeyPathIfNecessary
* Inputs:
*       HKEY root: Root of key
*       const CString & path: Path of the form
*                       a\b\c\d...
*       DWORD access: Desired access                                // REG #012
*       HKEY * key: Result of opening key
* Result: LONG
*       The result of ::RegOpenKeyEx
* Effect: 
*       If the path cannot be opened, tries to back off creating the key
*       one level at a time
****************************************************************************/

static LONG CreateKeyPathIfNecessary(HKEY root, const CString & path, DWORD access, HKEY * key)// REG #012
    {
     LONG result = ::RegOpenKeyEx(root, path, 0, access, key);      // REG #012
     if(result == ERROR_SUCCESS)
        return result;

     // We have a path of the form a\b\c\d  
     // But apparently a/b/c doesn't exist

     int i = path.ReverseFind(_T('\\'));
     if(i == -1)
        { /* failed */
         ::SetLastError(result);
         return result;  // well, we lose
        } /* failed */

     CString s;
     s = path.Left(i);

     HKEY newkey;
     result = CreateKeyPathIfNecessary(root, s, access | KEY_CREATE_SUB_KEY, &newkey); // REG #012
     if(result != ERROR_SUCCESS)
        { /* failed */
         ::SetLastError(result);                                    // REG #013
         return result;
        } /* failed */

     // OK, we now have created a\b\c
     CString v;
     v = path.Right(path.GetLength() - i - 1);
     DWORD disp;

     result = ::RegCreateKeyEx(newkey, 
                                v,
                                0, NULL,
                                REG_OPTION_NON_VOLATILE,
                                access,
                                NULL,
                                key,
                                &disp);
     ::RegCloseKey(newkey); // no longer needed
     return result;
    }

/****************************************************************************
*                                  SplitPathString
* Inputs:
*       IN const CString & var: Value or subpath ("name" or "name1\name2\...\namen")
*       OUT CString & path: Place where key-sequence of path is placed
*       OUT CString & name: Place to put value name
* Result: void
*       
* Effect: 
*       Takes a path of the form
*               \this\that
*       and a variable name 'var' of the form
*               value
*               whatever\value
*       and sets 'path' so that it is
*               \this\that\whatever
*       and sets 'name' so that it is
*               value
****************************************************************************/

static void SplitPathString(const CString & var, CString & path, CString & name) // REG #004
    {
     // locate the rightmost \ of the 'var'.  If there isn't one,
     // we simply copy var to name...

     if(var.IsEmpty())                                              // REG #004
        { /* illegal */                                             // REG #004
         ASSERT(FALSE); // error, input string is empty             // REG #004
         path = _T("");                                             // REG #004
         name = _T("");                                             // REG #004
         return;                                                    // REG #004
        } /* illegal */                                             // REG #004
                                                                    // REG #004
     // If it does not start with a "\", it is a path relative
     // to IDS_PROGRAM_ROOT; so the path will be prepended with
     // the IDS_PROGRAM_ROOT value
     
     // If it starts with a "\" character, it is an absolute
     // path. Strip the "\" off and return what is there

     if(var.GetAt(0) != _T('\\'))                                   // REG #004
        { /* relative path */                                       // REG #004
         path.LoadString(IDS_PROGRAM_ROOT);                         // REG #004
        } /* relative path */                                       // REG #004
     else                                                           // REG #004
        { /* absolute path */                                       // REG #004
         path = _T("");                                             // REG #004
        } /* absolute path */                                       // REG #004

     // See if there is a \ in the path passed in
     // If there is, everything ahead of the \ gets
     // appended to the 'path' value, and what is left
     // over is the 'name'

     int i = var.ReverseFind(_T('\\'));
     if(i < 0)
        { /* no \ */
         name = var;
         return;
        } /* no \ */

     // It had a \ in the input path value
     // reformulate the strings
     
     if(var.GetAt(0) != _T('\\'))                                   // REG #004
        { /* relative */                                            // REG #004
         // append the prefix of the var to the path, leaving only the name
         // E.g., the path is
         // IDS_PROGRAM_ROOT
         // and the var is
         // this\that
         // the result is that the path becomes
         // IDS_PROGRAM_ROOT\this
         // and the name becomes
         // that
         if(path.Right(1) != _T("\\"))
            path += _T("\\");
         path += var.Left(i);                                       // REG #004
         name = var.Mid(i + 1);                                     // REG #004
        } /* relative */                                            // REG #004
     else                                                           // REG #004
        { /* absolute */                                            // REG #004
         // The string that came in from var was an absolute
         // path of the form
         // \whatever\name1\name2\...\variablen
         // |<------path------------>|<-name->|
         //                          i

         path = var.Mid(1).Left(i - 1);                             // REG #004
         name = var.Mid(i + 1);
        } /* absolute */                                            // REG #004
    }

/****************************************************************************
*                              GetRegistryString
* Inputs:
*       HKEY root: HKEY_CURRENT_USER, etc.
*       const CString &var: Name of variable
*       CString &value: place to put value
* Result: BOOL
*       TRUE if registry key found, &val is updated
*       FALSE if registry key not found, &val is not modified
* Effect: 
*       Retrieves the key based on 
*       root\IDS_PROGRAM_ROOT\var
* Notes:
*       This presumes the value is a text string (SZ_TEXT). If it is not,
*       returns FALSE and GetLastError() returns INVALID_DATA_TYPE
* Errors:
*       ERROR_INVALID_DATATYPE - type is not REG_SZ, REG_MULTI_SZ or REG_EXPAND_SZ
****************************************************************************/

BOOL GetRegistryString(HKEY root, const CString &var, CString & val)
    {
     CString path;
     CString name;
     SplitPathString(var, path, name);

     if (path.Right(1) == _T("\\"))
        path = path.Left(path.GetLength()-1);
         
     HKEY key;
     LONG result = ::RegOpenKeyEx(root,                             // REG #012
                                  path,
                                  0,
                                  KEY_QUERY_VALUE,
                                  &key);
     if(result != ERROR_SUCCESS)
        { /* not found */
         ::SetLastError(result);                                    // REQ #013
         return FALSE;
        } /* not found */

     DWORD len = 0; // Desired length
     DWORD type;

     result = ::RegQueryValueEx(key, name, 0, &type, NULL, &len);
     // The len parameter is the required length of the string, including
     // the NUL character
     switch(result)
        { /* check result */
         case ERROR_MORE_DATA:
            // We now know the size
            break;
         case ERROR_SUCCESS:
            break; // returns ERROR_SUCCESS when buffer ptr is NULL
         default:
            ::SetLastError(result);
            return FALSE;
        } /* check result */

     LPTSTR p = val.GetBuffer((len / sizeof(TCHAR)) + 1);
     result = ::RegQueryValueEx(key, name, 0, &type, (LPBYTE)p, &len);
     ::RegCloseKey(key);
     val.ReleaseBuffer(len / sizeof(TCHAR));

     if(result != ERROR_SUCCESS)
        { /* error */
         ::SetLastError(result);
         return FALSE;
        } /* error */

     if(type != REG_SZ && type != REG_EXPAND_SZ && type != REG_MULTI_SZ)
        { /* wrong type */
         ::SetLastError(ERROR_INVALID_DATATYPE);                    // REG #006
         return FALSE;
        } /* wrong type */

     ::SetLastError(ERROR_SUCCESS);                                 // REG #013
     return TRUE;
    }

/****************************************************************************
*                              GetRegistryString
* Inputs:
*       HKEY root: Root key
*       const CString & path: Path value
*       const CString & var: Variable name
*       CString & val: Place to put value
* Result: BOOL
*       TRUE if successful
*       FALSE if error (::GetLastError gives error result)
* Effect: 
*       Reads the Registry value from the specified path/var
* Errors:
*       See underlying GetRegistryString
****************************************************************************/

BOOL GetRegistryString(HKEY root, const CString & path, const CString & var, CString & val)
   {
    CString s;
    s = path;
    if(s.Right(1) != _T("\\"))
       s += _T("\\");
    s += var;
    return GetRegistryString(root, s, val);
   } // GetRegistryString

/****************************************************************************
*                              GetRegistryString
* Inputs:
*       HKEY root: Root key
*       const CString & path: Path value
*       UINT var: Variable name, as string ID
*       CString & val: Place to put value
* Result: BOOL
*       TRUE if successful
*       FALSE if error (::GetLastError gives error result)
* Effect: 
*       Reads the Registry value from the specified path/var
* Errors:
*       See underlying GetRegistyString
****************************************************************************/

BOOL GetRegistryString(HKEY root, const CString & path, UINT var, CString & val)
   {
    CString s;
    s = path;
    if(s.Right(1) != _T("\\"))
       s += _T("\\");
    CString t;
    t.LoadString(var);
    s += t;
    return GetRegistryString(root, s, val);
   } // GetRegistryString

/****************************************************************************
*                             DeleteRegistryValue
* Inputs:
*       HKEY root: Root of path
*       const CString &var: Name of variable
* Result: BOOL
*       TRUE if item was deleted, or did not exist
*       FALSE if item was not deleted
* Effect: 
*       Deletes the variable named
* Errors:
*       ERROR_FILE_NOT_FOUND: will actually return TRUE, because value is gone!
****************************************************************************/

BOOL DeleteRegistryValue(HKEY root, const CString & var)
    {
     CString path;
     CString name;
     SplitPathString(var, path, name);

     HKEY key;
     LONG result = ::RegOpenKeyEx(root, path, 0, DELETE, &key);     // REG #012
     
     switch(result)
        { /* failed to delete */                                    // REG #006
         case ERROR_FILE_NOT_FOUND:
            return TRUE; // value did not exist, so it is "deleted"
         case ERROR_SUCCESS:
            break;
         default:
            ::SetLastError(result);                                 // REG #006
            return FALSE; // failed to delete                       // REG #001
        } /* failed to delete */                                    // REG #006

     result = ::RegDeleteValue(key, name);                          // REG #001
                                                                    // REG #001
     ::RegCloseKey(key);                                            // REG #001
                                                                    // REG #001
     switch(result)                                                 // REG #001
        { /* result */                                              // REG #001
         case ERROR_SUCCESS:                                        // REG #001
            ::SetLastError(ERROR_SUCCESS);                          // REG #013
            return TRUE;                                            // REG #001
         case ERROR_ACCESS_DENIED:                                  // REG #001
            ::SetLastError(result);                                 // REG #006
            return FALSE;                                           // REG #001
         default:                                                   // REG #006
            // Might want to ASSERT(FALSE) here...                  // REG #006
            ::SetLastError(result);                                 // REG #006
            return FALSE;                                           // REG #006
        } /* result */                                              // REG #001
    }

/****************************************************************************
*                              DeleteRegistryKey
* Inputs:
*       HKEY root: Root of path
*       const CString & key: Key name to delete
* Result: BOOL
*       TRUE if successful, or the key was not found (postcondition: key 
*                                          will not exist!)
*       FALSE if error
* Effect: 
*       Deletes the key and all its subkeys
* Notes:
*       This must be compiled with a suitably modern Platform SDK or
*       SHDeleteKey will not be defined. 
* Errors:
*       ERROR_FILE_NOT_FOUND: actually returns TRUE, because the key is gone!
****************************************************************************/

BOOL DeleteRegistryKey(HKEY root, const CString & keyname)
   {
    CString path;
    CString name;
    SplitPathString(keyname, path, name);

    HKEY key;
    LONG result = ::RegOpenKeyEx(root, path, 0, DELETE, &key);      // REQ #012

    if(result == ERROR_FILE_NOT_FOUND)
       return TRUE; // couldn't find it, so it is deleted already

    if(result != ERROR_SUCCESS)
       { /* error */
        ::SetLastError(result);
        return FALSE;
       } /* error */

    result = (LONG)SHDeleteKey(key, name); // if undefined, you have obsolete Platform SDK. Update it.

    if(result != ERROR_SUCCESS)
       { /* error */
        ::SetLastError(result);
        return FALSE;
       } /* error */
    ::SetLastError(ERROR_SUCCESS);                                  // REG #013
    return TRUE; 
   } // DeleteRegistryKey

/****************************************************************************
*                               GetRegistryInt
* Inputs:
*       HKEY  root: root of path
*       const CString &var: Name of variable
*       DWORD &val: Place to put value
* Result: BOOL
*       TRUE if registry key found, &val is updated
*       FALSE if registry key not found, &val is not modified
* Effect: 
*       Retrieves the key based on 
*       root\IDS_PROGRAM_ROOT\var
* Notes:
*       This presumes the value is a 32-bit value
* Errors:
*       ERROR_INVALID_DATATYPE: Value is not REG_DWORD
****************************************************************************/

BOOL GetRegistryInt(HKEY root, const CString &var, DWORD & val)
    {
     CString path;

     CString name;
     SplitPathString(var, path, name);

     HKEY key;
     LONG result = ::RegOpenKeyEx(root,                             // REG #012
                                  path,
                                  0,
                                  KEY_QUERY_VALUE,
                                  &key);
     if(result != ERROR_SUCCESS)
        { /* failed */
         ::SetLastError(result);
         return FALSE;
        } /* failed */

     DWORD buffer;
     DWORD len =  sizeof(buffer);
     DWORD type;

     result = ::RegQueryValueEx(key, name, 0, &type, (LPBYTE)&buffer, &len);
     ::RegCloseKey(key);

     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;
        } /* failed */                                              // REG #006

     if(type != REG_DWORD)
        { /* type mismatch */                                       // REG #006
         ::SetLastError(ERROR_INVALID_DATATYPE);                    // REG #006
         return FALSE;
        } /* type mismatch */                                       // REG #006

     val = buffer;

     ::SetLastError(ERROR_SUCCESS);                                 // REG #013
     return TRUE;
    }

/****************************************************************************
*                               GetRegistryInt
* Inputs:
*       HKEY  root: root of path
*       const CString &path: path to variable
*       const CString &var: rest of path to variable (may be in path form)
*       DWORD &val: Place to put value
* Result: BOOL
*       TRUE if successful, value is changed
*       FALSE in unsuccessful, value is unchanged
****************************************************************************/

BOOL GetRegistryInt(HKEY root, const CString & path, const CString & var, DWORD & val)
   {
    CString s;
    s = path;
    if(s.Right(1) != _T("\\"))                                      // REQ #148// FBG #334
       s += _T("\\");
    s += var;
    return GetRegistryInt(root, s, val);
   } // GetRegistryInt

/****************************************************************************
*                               GetRegistryInt
* Inputs:
*       HKEY  root: root of path
*       const CString &path: path to variable
*       UINT var: rest of path to variable expressed as a stringtable ID
*       DWORD &val: Place to put value
* Result: BOOL
*       TRUE if successful, value is changed
*       FALSE in unsuccessful, value is unchanged
****************************************************************************/

BOOL GetRegistryInt(HKEY root, const CString & path, UINT var, DWORD & val)
   {
    CString s;
    s = path;
    if(s.Right(1) != _T("\\"))                                      // REQ #148 // FBG #334
       s += _T("\\");
    CString t;
    t.LoadString(var);
    s += t;
    return GetRegistryInt(root, s, val);
   } // GetRegistryInt

/****************************************************************************
*                               GetRegistryInt64
* Inputs:
*       HKEY  root: root of path
*       const CString &var: rest of path to variable (may be in path form)
*       ULONGLONG &val: Place to put value
* Result: BOOL
*       TRUE if successful, value is changed
*       FALSE in unsuccessful, value is unchanged
* Errors:
*       ERROR_INVALID_DATATYPE: Value is not REG_BINARY
*       ERROR_INVALID_DATA: Length is not sizeof(ULONGLONG)
****************************************************************************/

BOOL GetRegistryInt64(HKEY root, const CString & var, __int64 & val) // REG #013
   {
    CString path;
    CString name;
    SplitPathString(var, path, name);

    HKEY key;
    LONG result = ::RegOpenKeyEx(root, path, 0, KEY_QUERY_VALUE, &key);// REG #012
    if(result != ERROR_SUCCESS)
       { /* failed */
        ::SetLastError(result);
        return FALSE;
       } /* failed */

    ULONGLONG buffer;
    DWORD len =  sizeof(buffer);
    DWORD type;

    result = ::RegQueryValueEx(key, name, 0, &type, (LPBYTE)&buffer, &len);
    ::RegCloseKey(key);

    if(result != ERROR_SUCCESS)
       { /* save error */
        ::SetLastError((DWORD)result);
        return FALSE;
       } /* save error */

    if(type != REG_BINARY)
       { /* wrong type */
        ::SetLastError(ERROR_INVALID_DATATYPE);
        return FALSE;
       } /* wrong type */

    if(len != sizeof(ULONGLONG))
       { /* wrong length */
        ::SetLastError(ERROR_INVALID_DATA);
        return FALSE;
       } /* wrong length */

    val = buffer;

    ::SetLastError(ERROR_SUCCESS);                                  // REG #013
    return TRUE;
   } // GetRegistryInt64

/****************************************************************************
*                            GetRegistryDWordArray
* Inputs:
*       HKEY  root: root of path
*       const CString &var: Name of variable
*       CDWordArray & data: Array to be filled in
* Result: BOOL
*       TRUE if successful
*       FALSE if there is an error
* Effect: 
*       Allocates a DWORD array and reads data into it
* Notes:
*       A RemoveAll is performed before the array is filled
* Errors:
*       ERROR_INVALID_DATATYPE - data is not REG_BINARY
*       ERROR_INVALID_DATA     - data is not a multiple of sizeof(DWORD)
****************************************************************************/

BOOL GetRegistryDWordArray(HKEY root, const CString &var, CDWordArray & data)// REG #006
    {
     CString path;
     CString name;
     SplitPathString(var, path, name);
     
     data.RemoveAll();                                              // REG #006

     HKEY key;
     LONG result = ::RegOpenKeyEx(root,                             // REG #012
                                  path,
                                  0,
                                  KEY_QUERY_VALUE,
                                  &key);
     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;                                              // REG #006
        } /* failed */                                              // REG #006

     DWORD len = 0;
     DWORD type;

     // By using a NULL pointer, we get back the length
     // in bytes of the data
     result = ::RegQueryValueEx(key, name, 0, &type, NULL, &len);

     if(result != ERROR_SUCCESS)
        { /* failed */
         ::RegCloseKey(key);
         ::SetLastError(result);                                    // REG #006
         return FALSE;                                              // REG #006
        } /* failed */

     if(type != REG_BINARY)
        { /* wrong type */
         ::SetLastError(ERROR_INVALID_DATATYPE);
         ::RegCloseKey(key);
         return FALSE;
        } /* wrong type */

     // Compute the count of DWORDs. 
     DWORD count = len / sizeof(DWORD);

     // Make sure we have an integral number of DWORDs

     if(count * sizeof(DWORD) != len)
        { /* bad data */
         ::SetLastError(ERROR_INVALID_DATA);
         ::RegCloseKey(key);
         return FALSE;
        } /* bad data */

     data.SetSize(count); // preallocate the array data             // REG #006

     result = ::RegQueryValueEx(key, name, 0, &type, (LPBYTE)data.GetData(), &len);// REG #006

     if(result != ERROR_SUCCESS)
        { /* failed */
         ::RegCloseKey(key);
         data.RemoveAll();                                          // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;                                              // REG #006
        } /* failed */

     ::RegCloseKey(key);
     ::SetLastError(ERROR_SUCCESS);                                 // REQ #013
     return TRUE;                                                   // REG #006
    }

/****************************************************************************
*                            GetRegistryDWordArray
* Inputs:
*       HKEY  root: root of path
*       UINT id: IDS_ name of string
* Result: BOOL
*       TRUE if successful
*       FALSE if there is an error
* Effect: 
*       Fills in the DWordArray data with the data from the Registry value
*       The array will be cleared even if there is an error
****************************************************************************/

BOOL GetRegistryDWordArray(HKEY root, UINT id, CDWordArray & data)  // REG #002// REG #006
    {                                                               // REG #002
     CString s;                                                     // REG #002
     s.LoadString(id);                                              // REG #002
     return GetRegistryDWordArray(root, s, data);                   // REG #002// REG #006
    } // GetRegistryDWordArray                                      // REG #002

/****************************************************************************
*                            SetRegistryDWordArray
* Inputs:
*       HKEY  root: root of path
*       const CString &var: Name of variable
*       const CDWordArray & data: Data to write                     // REG #006
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Writes the data for the key
****************************************************************************/

BOOL SetRegistryDWordArray(HKEY root, const CString & var, const CDWordArray & data)// REG #006
    {
     CString path;
     CString name;
     SplitPathString(var, path, name);

     HKEY key;
     LONG result = CreateKeyPathIfNecessary(root,
                                            path,
                                            KEY_SET_VALUE,          // REG #012
                                            &key);
     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;
        } /* failed */                                              // REG #006
     
     result = ::RegSetValueEx(key, name, 0, REG_BINARY, (LPBYTE)data.GetData(),  // REG #006 
                               (DWORD) data.GetSize() * (DWORD)sizeof(DWORD));
     ::RegCloseKey(key);
     if(result != ERROR_SUCCESS)
        { /* failed */
         ::SetLastError(result);
         return FALSE;
        } /* failed */

     return TRUE;
    }

/****************************************************************************
*                            SetRegistryDWordArray
* Inputs:
*       HKEY  root: root of path
*       UINT id: IDS_ value of key
*       const CDWordArray & data: Data to write
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Loads the string then calls SetRegistryDWordArray
****************************************************************************/

BOOL SetRegistryDWordArray(HKEY root, UINT id, const CDWordArray & data)  // REG #002// REG #006
    {                                                               // REG #002
     CString s;                                                     // REG #002
     s.LoadString(id);                                              // REG #002
     return SetRegistryDWordArray(root, s, data);                   // REG #002
    } // SetRegistryDWordArray                                      // REG #002

/****************************************************************************
*                              SetRegistryString
* Inputs:
*       HKEY root: root of search
*       const CString & var: Name of variable
*       CString & val: Value to write
* Result: BOOL
*       TRUE if registry key set
*       FALSE if registry key not set
* Effect: 
*       Retrieves the key based on 
*       root\IDS_PROGRAM_ROOT\var
* Notes:
*       This presumes the value is a string.
*       This does not support writing REG_MULTI_SZ or REG_EXPAND_SZ
****************************************************************************/

BOOL SetRegistryString(HKEY root, const CString & var, const CString & val)
    {
     CString path;
     CString name;
     SplitPathString(var, path, name);

     HKEY key;

     LONG result = CreateKeyPathIfNecessary(root, path, KEY_SET_VALUE, &key);

     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;
        } /* failed */                                              // REG #006

     result = ::RegSetValueEx(key, name, 0, REG_SZ, (LPBYTE)(LPCTSTR)val, (lstrlen(val) + 1) * sizeof(TCHAR));// REG #003
     ::RegCloseKey(key);

     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;
        } /* failed */                                              // REG #006

     return TRUE;
    }

/****************************************************************************
*                              SetRegistryString
* Inputs:
*       HKEY root: Root key
*       const CString & path: Path to variable
*       const CString & var: Variable name
*       const CString & val: Value to set
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Writes the Cstring value to the Registry
****************************************************************************/

BOOL SetRegistryString(HKEY root, const CString & path, const CString & var, const CString & val)
   {
    CString s;
    s = path;
    if(s.Right(1) != _T("\\"))                                      // REQ #148
       s += _T("\\");
    s += var;
    return SetRegistryString(root, s, val);
   } // SetRegistryString

/****************************************************************************
*                              SetRegistryString
* Inputs:
*       HKEY root: Root key
*       const CString & path: Path to variable
*       const CString & var: Variable name
*       UINT val: Value to set, expressed as string table ID
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Writes the Cstring value to the Registry
****************************************************************************/

BOOL SetRegistryString(HKEY root, const CString & path, UINT var, const CString & val)
   {
    CString s;
    s = path;
    if(s.Right(1) != _T("\\"))                                      // REQ #148
       s += _T("\\");
    CString t;
    t.LoadString(var);
    s += t;
    return SetRegistryString(root, s, val);
   } // SetRegistryString

/****************************************************************************
*                              SetRegistryInt
*       HKEY root : root of search
*       const CString var: Name of variable, including possibly path info
*       DWORD val: Value to set
* Result: BOOL
*       TRUE if registry key set
*       FALSE if registry key not set
* Effect: 
*       Retrieves the key based on 
*       root\IDS_PROGRAM_ROOT\var
****************************************************************************/

BOOL SetRegistryInt(HKEY root, const CString & var, DWORD val)
    {
     CString path;
     CString name;

     SplitPathString(var, path, name);

     HKEY key;
     LONG result = CreateKeyPathIfNecessary(root, path, KEY_SET_VALUE, &key); // REG #012

     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;
        } /* failed */                                              // REG #006

     result = ::RegSetValueEx(key, name, 0, REG_DWORD, (LPBYTE)&val, sizeof(DWORD));
     ::RegCloseKey(key);

     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;
        } /* failed */                                              // REG #006

     return TRUE;
    }

/****************************************************************************
*                               SetRegistryInt
* Inputs:
*       HKEY root:
*       const CString & path: initial path to the variable
*       const CString & var: rest of the path to the variable
*       DWORD val: value to write
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Writes the data to the Registry
****************************************************************************/

BOOL SetRegistryInt(HKEY root, const CString & path, const CString & var, DWORD val)
   {
    CString s = path;
    if(s.Right(1) != _T("\\"))                                      // REQ #148 // REQ #334
       s += _T("\\");
    s += var;
    return SetRegistryInt(root, s, val);
   } // SetRegistryInt

/****************************************************************************
*                              SetRegistryInt64
*       HKEY root : root of search
*       const CString var: Name of variable, including possibly path info
*       __int64 val: Value to set
* Result: BOOL
*       TRUE if registry key set
*       FALSE if registry key not set
* Effect: 
*       Retrieves the key based on 
*       root\IDS_PROGRAM_ROOT\var
* Notes:
*       This presumes the value is a string
****************************************************************************/

BOOL SetRegistryInt64(HKEY root, const CString & var, __int64 val)
   {
    CString path;
    CString name;

    SplitPathString(var, path, name);

    HKEY key;
    LONG result = CreateKeyPathIfNecessary(root, path, KEY_SET_VALUE, &key); // REG #012

    if(result != ERROR_SUCCESS)
       { /* save error result */
        ::SetLastError((DWORD)result);
        return FALSE;
       } /* save error result */

    result = ::RegSetValueEx(key, name, 0, REG_BINARY, (LPBYTE)&val, sizeof(ULONGLONG));
    ::RegCloseKey(key);

    if(result != ERROR_SUCCESS)
       ::SetLastError((DWORD)result);

    return result == ERROR_SUCCESS;
   }

/****************************************************************************
*                               SetRegistryInt
* Inputs:
*       HKEY root:
*       const CString & path: initial path to the variable
*       UINT var: rest of the path to the variable, as string ID
*       DWORD val: value to write
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Writes the data to the Registry
****************************************************************************/

BOOL SetRegistryInt(HKEY root, const CString & path, UINT var, DWORD val)
   {
    CString s = path;
    if(s.Right(1) != _T("\\"))                                      // REQ #148 // REQ #334
       s += _T("\\");
    CString t;
    t.LoadString(var);
    s += t;
    return SetRegistryInt(root, s, val);
   } // SetRegistryInt

/****************************************************************************
*                               SetRegistryGUID
* Inputs:
*       HKEY root:
*       const CString &var: Name to store it under
*       const GUID & val: Value to store
* Result: BOOL
*       TRUE if successful, FALSE if error
* Effect: 
*       Retrieves the key based on
*       root\IDS_PROGRAM_ROOT\var
****************************************************************************/

BOOL SetRegistryGUID(HKEY root, const CString & var, const GUID & val)
    {
     CString path;
     CString name;
     SplitPathString(var, path, name);

     HKEY key;
     LONG result = CreateKeyPathIfNecessary(root, path, KEY_SET_VALUE, &key); // REG #012
     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;                                              // REG #006
        } /* failed */                                              // REG #006

     result = ::RegSetValueEx(key, name, 0, REG_BINARY, (LPBYTE)&val, sizeof(GUID));
     ::RegCloseKey(key);

     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;                                              // REG #006
        } /* failed */                                              // REG #006

     return TRUE;
    } // SetRegistryGUID

/****************************************************************************
*                               GetRegistryGUID
* Inputs:
*       HKEY root: root of key
*       const CString & Var: Name to store it under
*       GUID & val: Place to store result
* Result: BOOL
*       TRUE if successful, FALSE if error
* Effect: 
*       Retrieves the key based on
*       root\IDS_PROGRAM_ROOT\var
****************************************************************************/

BOOL GetRegistryGUID(HKEY root, const CString & var, GUID & val)
    {
     CString path;
     CString name;
     SplitPathString(var, path, name);

     HKEY key;
     LONG result = ::RegOpenKeyEx(root, path, 0, KEY_QUERY_VALUE, &key);// REG #012
     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;                                              // REG #006
        } /* failed */                                              // REG #006

     DWORD type;
     DWORD len = sizeof(GUID);
     result = ::RegQueryValueEx(key, name, 0, &type, (LPBYTE)&val, &len);

     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;                                              // REG #006
        } /* failed */                                              // REG #006
        return FALSE;

     if(type != REG_BINARY)
        { /* failed */                                              // REG #006
         ::SetLastError(ERROR_INVALID_DATATYPE);                    // REG #006
         return FALSE;                                              // REG #006
        } /* failed */                                              // REG #006

     if(len != sizeof(GUID))
        { /* wrong length */
         ::SetLastError(ERROR_INVALID_DATA);
         return FALSE;
        } /* wrong length */
     return TRUE;
    } // GetRegistryGUID

/****************************************************************************
*                              EnumRegistryKeys
* Inputs:
*       HKEY root: Root of search
*       const CString & group: Name of group key
*       CStringArray & keys: Array to be filled in
* Result: BOOL
*       TRUE if successful
*       FALSE if failure
* Effect: 
*       Will delete all elements from the CStringArray before
*       installing the new list of keys
* Notes:
*       If the length is 0 and the result is TRUE, ::GetLastError will tell
*       why
****************************************************************************/

BOOL EnumRegistryKeys(HKEY root, const CString & group, CStringArray & keys)// REG #006
    {
     CString path;

     TCHAR itemName[MAX_PATH];

     path = BuildKeyNameString(group);                                  // REG #007

     HKEY key;

     LONG result = ::RegOpenKeyEx(root, path, 0, KEY_ENUMERATE_SUB_KEYS, &key);// REG #012
     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;                                              // REG #006
        } /* failed */                                              // REG #006

     keys.RemoveAll();                                              // REG #006
     DWORD i = 0;
     while(TRUE)
        { /* loop */
         FILETIME time;
         DWORD len = (sizeof(itemName)/sizeof(TCHAR));
         LONG result = ::RegEnumKeyEx(key, i, itemName, &len, NULL, NULL, NULL, &time);
         if(result != ERROR_SUCCESS)
            { /* failed */                                          // REG #006
             ::SetLastError(result);                                // REG #006
             break;
            } /* failed */                                          // REG #006
         // we have a valid key name
         keys.SetAtGrow(i, itemName);                               // REG #006
         i++;
        } /* loop */

     ::RegCloseKey(key);
     return TRUE;                                                   // REG #006
    } // EnumRegistryKeys (CString)

/****************************************************************************
*                              EnumRegistryKeys
* Inputs:
*       HKEY root: Root of search
*       UINT group: Group name ID
*       CStringArray & keys: Array to be filled in
* Result: BOOL
*       TRUE if successful
*       FALSE if failure
* Effect: 
*       Will delete all elements from the CStringArray before
*       installing the new list of keys
* Notes:
*       If the length is 0 and the result is TRUE, ::GetLastError will tell
*       why
****************************************************************************/

BOOL EnumRegistryKeys(HKEY root, UINT group, CStringArray & keys)// REG #006
   {
    CString s;
    s.LoadString(group);
    return EnumRegistryKeys(root, s, keys);
   } // EnumRegistryKeys (UINT)

/****************************************************************************
*                              EnumRegistryValues
* Inputs:
*       HKEY root: Root of search
*       const CString & group: Name of value group key
*       CStringArray & keys: Value names
* Result: BOOL
*       TRUE if successful
*       FALSE if failure
* Effect: 
*       Will delete all elements from the CStringArray before
*       installing the new list of value names
* Notes:
*       If the length is 0 and the result is TRUE, ::GetLastError will tell
*       why
****************************************************************************/

BOOL EnumRegistryValues(HKEY root, const CString & group, CStringArray & keys)// REG #006
    {
     CString path;

     TCHAR itemName[MAX_PATH];

     path = BuildKeyNameString(group);                              // REG #007

     HKEY key;

     LONG result = ::RegOpenKeyEx(root, path, 0, KEY_QUERY_VALUE, &key); // REG #012
     if(result != ERROR_SUCCESS)
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;
        } /* failed */                                              // REG #006

     keys.RemoveAll();                                              // REG #006
     DWORD i = 0;

     while(TRUE)
        { /* loop */
         DWORD length = sizeof(itemName)/sizeof(TCHAR);
         LONG result = ::RegEnumValue(key, // key selection
                                      i,   // which key
                                      itemName, // place to put value name
                                      &length,  // in: length of buffer
                                                // out: length of name
                                      NULL,     // reserved, NULL
                                      NULL,     // place to put type
                                      NULL,     // place to put value
                                      NULL);    // place to put value length
         if(result != ERROR_SUCCESS)                                // REG #006
            { /* failed */                                          // REG #006
             ::SetLastError(result);                                // REG #006
             break;       
            } /* failed */                                          // REG #006
         // we have a valid key name
         keys.SetAtGrow(i, itemName);
         i++;
        } /* loop */

     ::RegCloseKey(key);
     return TRUE;                                                   // REG #006
    }

/****************************************************************************
*                             SetRegistryValues
* Inputs:
*       HKEY root: Root key
*       const CString & group: subgroup under which keys are found
*       const CStringArray & values
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Saves the keys. The key names are based on a 5-digit key
****************************************************************************/
 
BOOL SetRegistryValues(HKEY root, const CString & group, const CStringArray & values)
    {
     BOOL ok = TRUE;
     DeleteRegistryKey(root, group);
     for(int i = 0; i < values.GetSize(); i++)
        { /* enumerate values */
         CString key;
         key.Format(_T("%05d"), i);
         ok &= SetRegistryString(root, group, key, values[i]);
        } /* enumerate values */
     return ok;
    } // SetRegistryValues

/****************************************************************************
*                             SetRegistryValues
* Inputs:
*       HKEY root: root key
*       UINT group: String ID of group
*       const CStringArray & values: array of values
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Saves the array of string values
****************************************************************************/

BOOL SetRegistryValues(HKEY root, UINT group, const CStringArray & values)
    {
     CString groupname;
     groupname.LoadString(group);
     return SetRegistryValues(root, groupname, values);
    } // SetRegistryValues

/****************************************************************************
*                             SetRegistryValues
* Inputs:
*       HKEY root: Root key
*       const CString & group: subgroup under which keys are found
*       const CDWordArray & values
* Result: BOOL
*       TRUE if successul
*       FALSE if error
* Effect: 
*       Stores the array as a sequence of enumerated DWORD values
****************************************************************************/

BOOL SetRegistryValues(HKEY root, const CString & group, const CDWordArray & values)
    {
     BOOL ok = TRUE;
     DeleteRegistryKey(root, group);
     for(int i = 0; i < values.GetSize(); i++)
        { /* enumerate values */
         CString key;
         key.Format(_T("%05d"), i);
         ok &= SetRegistryInt(root, group, key, values[i]);
        } /* enumerate values */
     return ok;
    } // SetRegistryValues

/****************************************************************************
*                             SetRegistryValues
* Inputs:
*       HKEY root: root key
*       UINT group: String ID of group
*       const CDWordArray & values: array of values
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Saves the array of string values
****************************************************************************/

BOOL SetRegistryValues(HKEY root, UINT group, const CDWordArray & values)
    {
     CString groupname;
     groupname.LoadString(group);
     return SetRegistryValues(root, groupname, values);
    } // SetRegistryValues

/****************************************************************************
*                              GetRegistryValues
* Inputs:
*       HKEY root: Root of search
*       const CString & group: Group name
*       CStringArray & values: Values to be filled in
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Reads the elements under the group into the array
* Notes:
*       No matter what happens, the contents of the array are deleted
****************************************************************************/

BOOL GetRegistryValues(HKEY root, const CString & group, CStringArray & values)
    {
     DWORD err = ERROR_SUCCESS;
     values.RemoveAll();
     CStringArray keys;
     if(!EnumRegistryValues(root, group, keys))
        return FALSE;
     for(int i = 0; i < keys.GetSize(); i++)
        { /* get each */
         CString val;
         if(GetRegistryString(root, group, keys[i], val))
            values.Add(val);
         else
            { /* had error */
             err = ::GetLastError();
            } /* had error */
        } /* get each */
     if(err != ERROR_SUCCESS)
        { /* failed */
         ::SetLastError(err);
         return FALSE;
        } /* failed */
     return TRUE;
    } // GetRegistryValues

/****************************************************************************
*                              GetRegistryValues
* Inputs:
*       HKEY root: Root of search
*       UINT group: Group name
*       CStringArray & values: values to be filled in
* Result: BOOL
*       TRUE if success
*       FALSE if failure
* Effect: 
*       Reads in all the values stored under the group. The key names
*       are discarded. It is assumed that this is going to be a sequence
*       of anonymous values stored by SetRegistryValues
****************************************************************************/

BOOL GetRegistryValues(HKEY root, UINT group, CStringArray & values)
    {
     CString groupname;
     groupname.LoadString(group);
     return GetRegistryValues(root, groupname, values);
    } // GetRegistryValues

/****************************************************************************
*                               GetRegistryKey
* Inputs:
*       HKEY root: Root of key
*       const CString & name: Name of key to open
*       HKEY &key: Place to put open key
* Result: BOOL
*       TRUE if key successfully found, key value is valid
*       FALSE if key not found key value undefined
* Effect: 
*       Opens a registry key based on the standard path. It is the
*       responsibility of the caller to close this key.
*       If the key does not exist, the path to it is created
* Notes:
*       This is used when it is necessary to iterate through subkeys or
*       other purposes. It does force the key to be created.
****************************************************************************/

BOOL GetRegistryKey(HKEY root, const CString & keyname, HKEY & key, DWORD access /* = KEY_ALL_ACCESS */)
   {
    CString path;
    CString name;
    SplitPathString(keyname, path, name);

    // Because we are looking for a key, and SplitPathString has already
    // taken a path of the form
    // a\b\c\d
    // and made it into:
    // path: a\b\c
    // name: d
    // We need to reassemble them back into a complete path.
    // OK, a bit kludgy, but it handles this one special case fine
    if(path.Right(1) != _T("\\"))
       path += _T("\\");
    path += name;

    LONG result = ::RegOpenKeyEx(root, path, 0, access, &key);      // REG #012

    if(result != ERROR_SUCCESS)
       { /* failed */
        ::SetLastError(result);
        return FALSE;
       } /* failed */
    return TRUE;
   } // GetRegistryKey

/****************************************************************************
*                               FindRegistryKey
* Inputs:
*       HKEY root: Root of key
*       const CString & name: Name of key to open
*       HKEY &key: Place to put open key
*       DWORD access: Desired access (defaults to KEY_ALL_ACCESS)
* Result: BOOL
*       TRUE if the path exists; key is the open key to that path
*       FALSE if the path does not exist; key is unmodified
* Effect: 
*       Unlike GetRegistryKey, this does not attempt to create the key
****************************************************************************/

BOOL FindRegistryKey(HKEY root, const CString & keyname, HKEY & key, DWORD access /* = KEY_ALL_ACCESS */)
   {
    CString path;
    CString name;
    SplitPathString(keyname, path, name);                                  // REG #004

    // Because we are looking for a key, and SplitPathString has already
    // taken a path of the form
    // a\b\c\d
    // and made it into:
    // path: a\b\c
    // name: d
    // We need to reassemble them back into a complete path.
    // OK, a bit kludgy, but it handles this one special case fine
    if(path.Right(1) != _T("\\"))
       path += _T("\\");
    path += name;

    LONG result = ::RegOpenKeyEx(root, path, 0, access, &key);      // REG #012
    if(result != ERROR_SUCCESS)
       { /* failed */
        ::SetLastError(result);
        return FALSE;
       } /* failed */
    return TRUE;
   } // FindRegistryKey

/****************************************************************************
*                                  CreateKey
* Inputs:
*       HKEY root: Root of key
*       CString & key: Key to create
*       DWORD access: Desired access
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Creates the indicated key under the default path. The key is
*       left closed.
****************************************************************************/

BOOL CreateKey(HKEY root, const CString & keyname, DWORD access /* = KEY_ALL_ACCESS */) // REG #010// REG #012
    {                                                               // REG #010
     CString path;                                                  // REG #010
     CString name;                                                  // REG #010
     SplitPathString(keyname, path, name);                          // REG #010
                                                                    // REG #010
    // Because we are looking for a key, and SplitPathString has already
    // taken a path of the form
    // a\b\c\d
    // and made it into:
    // path: a\b\c
    // name: d
    // We need to reassemble them back into a complete path.
    // OK, a bit kludgy, but it handles this one special case fine
     if(path.Right(1) != _T("\\") )                                  // REG #010
        path += _T("\\");                                           // REG #010
     path += keyname;                                               // REG #010
                                                                    // REG #010
     HKEY key;                                                      // REG #010
     DWORD disp;                                                    // REG #010
     LONG result = ::RegCreateKeyEx(root,                           // REG #010
                                    path,                           // REG #010
                                    0, NULL,                        // REG #010
                                    REG_OPTION_NON_VOLATILE,        // REG #010
                                    access,                         // REG #010// REG #012
                                    NULL,                           // REG #010
                                    &key,                           // REG #010
                                    &disp);                         // REG #010
     if(result != ERROR_SUCCESS)                                    // REG #010
        { /* failed */                                              // REG #006// REG #010
         ::SetLastError(result);                                    // REG #006// REG #010
         return FALSE;                                              // REG #010
        } /* failed */                                              // REG #006// REQ #010
                                                                    // REG #010
     ::RegCloseKey(key);                                            // REG #010
     return TRUE;                                                   // REG #010
    } // CreateKey                                                  // REG #010

/****************************************************************************
*                                   OpenKey
* Inputs:
*       HKEY root: Root to open
*       const CString & key: Key to open
* Result: HKEY
*       Key, or NULL if error
* Effect: 
*       Opens the key. Does not create it if it does not exist
****************************************************************************/

HKEY OpenKey(HKEY root, const CString & path, DWORD access /* = KEY_ALL_ACCESS */) // REG #010
    {                                                               // REG #010
     CString fullpath;                                              // REG #010
     fullpath = BuildKeyNameString(path);                           // REG #007// REG #010
                                                                    // REG #010
     HKEY key;                                                      // REG #010
     LONG result = ::RegOpenKeyEx(root, fullpath, 0, access, &key); // REG #010// REG #012
     if(result != ERROR_SUCCESS)                                    // REG #010
        { /* failed */                                              // REG #006// REG #010
         ::SetLastError(result);                                    // REG #006// REG #010
         return NULL; // error                                      // REG #010
        } /* failed */                                              // REG #006// REG #010
     return key;                                                    // REG #010
    } // OpenKey                                                    // REG #010

/****************************************************************************
*                                 GetKeyPath
* Inputs:
*       const CString & path: Key name
* Result: CString
*       Absolute path to a name
****************************************************************************/

CString GetKeyPath(const CString & path)                            // REG #011
    {                                                               // REG #011
     CString fullpath;                                              // REG #011
     fullpath = BuildKeyNameString(path);                           // REG #011
     return fullpath;                                               // REG #011
    } // GetKeyPath                                                 // REG #011

/****************************************************************************
*                                 GetKeyPath
* Inputs:
*       UINT id: ID of string
* Result: CString
*       Complete registry path to value
****************************************************************************/

CString GetKeyPath(UINT id)                                         // REG #011
    {                                                               // REG #011
     CString s;                                                     // REG #011
     s.LoadString(id);                                              // REG #011
     return GetKeyPath(s);                                          // REG #011
    } // GetKeyPath                                                 // REG #011

/****************************************************************************
*                               SetRegistryFont
* Inputs:
*       HKEY root: Root to write
*       const CString & var: Name of property
*       const LOGFONT * f: Logical font, or NULL to delete the font subkey
* Result: BOOL
*       TRUE if successful, FALSE if error
* Effect: 
*       Writes logical font information to the Registry
* Notes:
*       This does not save all the LOGFONT information, just the
*       face name, font height, font weight, italic flag, underline flag,
*       and strikeout flag
****************************************************************************/

BOOL SetRegistryFont(HKEY root, const CString & var, const LOGFONT * f)// REG #005
    {                                                               // REG #005
     CString path;                                                  // REG #005
     CString name;                                                  // REG #005
     SplitPathString(var, path, name);                              // REG #005
                                                                    // REG #005
     HKEY key;                                                      // REG #005
     DWORD flag;
     if(f == NULL)
        flag = DELETE;
     else
        flag = KEY_CREATE_SUB_KEY | KEY_SET_VALUE;
     
     LONG result = CreateKeyPathIfNecessary(root, path, flag, &key); // REG #005
     if(result != ERROR_SUCCESS)                                    // REG #005
        { /* failed */                                              // REG #005// REG #006
         ::SetLastError(result);                                    // REG #005// REG #006
         return FALSE;                                              // REG #005
        } /* failed */                                              // REG #005// REG #006
                                                                    // REG #005
     HKEY rkey;                                                     // REG #005
     DWORD disp;                                                    // REG #005
     if(f == NULL)                                                  // REG #005
        { /* delete key */                                          // REG #005
         result = SHDeleteKey(key, name); // deletes key and all subkeys// REG #005
         switch(result)                                             // REG #005
            { /* result */                                          // REG #005
             case ERROR_FILE_NOT_FOUND: // already gone!            // REG #005
             case ERROR_SUCCESS: // deleted                         // REG #005
                ::RegCloseKey(key);                                 // REG #005
                return TRUE;                                        // REG #005
             default:                                               // REG #005
                ASSERT(FALSE);                                      // REG #005
                ::RegCloseKey(key);                                 // REG #005
                return FALSE;                                       // REG #005
            } /* result */                                          // REG #005
         return TRUE;                                               // REG #005
        } /* delete key */                                          // REG #005
                                                                    // REG #005
     result = ::RegCreateKeyEx(key,                                 // REG #005
                               name,                                // REG #005
                               0, NULL,                             // REG #005
                               REG_OPTION_NON_VOLATILE,             // REG #005
                               KEY_SET_VALUE,                       // REG #005
                               NULL,                                // REG #005
                               &rkey,                               // REG #005
                               &disp);                              // REG #005
     if(result != ERROR_SUCCESS)                                    // REG #005
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #006
         return FALSE;                                              // REG #005
        } /* failed */                                              // REG #006
                                                                    // REG #005
     // Note that we save only a subset of the LOGFONT information  // REG #005
     // Note also that we use constant names from the LOGFONT structure// REG #005
     // so we don't load them from the resource strings             // REG #005
                                                                    // REG #005
#define RegSetFontValue(field) ::RegSetValueEx(rkey, _T(#field), 0, \
                                 sizeof(f->field) == sizeof(DWORD) ? REG_DWORD : REG_BINARY, \
                                 (LPBYTE)&f->field, sizeof(f->field))

     VERIFY((result = RegSetFontValue(lfHeight))    == ERROR_SUCCESS);// REG #005
     VERIFY((result = RegSetFontValue(lfWeight))    == ERROR_SUCCESS);// REG #005
     VERIFY((result = RegSetFontValue(lfItalic))    == ERROR_SUCCESS);// REG #005
     VERIFY((result = RegSetFontValue(lfUnderline)) == ERROR_SUCCESS);// REG #005
     VERIFY((result = RegSetFontValue(lfStrikeOut)) == ERROR_SUCCESS);// REG #005
     result = ::RegSetValueEx(rkey, _T("lfFaceName"), 0, REG_SZ, (LPBYTE)f->lfFaceName, (lstrlen(f->lfFaceName) + 1) * sizeof(TCHAR));// REG #005// REG #014
                                                                    // REG #005
     ::RegCloseKey(rkey);                                           // REG #005
     ::RegCloseKey(key);                                            // REG #005
     return TRUE; // assume success                                 // REG #005
    } // SetRegistryFont                                            // REG #005

/****************************************************************************
*                               GetRegistryFont
* Inputs:
*       HKEY root: Root key
*       const CString & keyname: Key name
*       LPLOGFONT f: Logfont structure to be loaded
* Result: BOOL
*       TRUE if successful
*       FALSE if error. LOGFONT will be zeroed, or may be only partially correct
* Effect: 
*       Loads the font information. In addition, sets the other LOGFONT
*       values
****************************************************************************/

BOOL GetRegistryFont(HKEY root, const CString & keyname, LPLOGFONT f)// REG #005
    {                                                               // REG #005
     ASSERT(f != NULL);                                             // REG #015
     if(f == NULL)                                                  // REG #015
        return FALSE;                                               // REG #015
     CString path;                                                  // REG #005
     path.LoadString(IDS_PROGRAM_ROOT);                             // REG #005
                                                                    // REG #005
     CString name;                                                  // REG #005
     SplitPathString(keyname, path, name);                          // REG #005
                                                                    // REG #005
     ::ZeroMemory(f, sizeof(LOGFONT));                              // REG #005//REG #014
                                                                    // REG #005
     HKEY key;                                                      // REG #005
     LONG result = ::RegOpenKeyEx(root, path, 0, KEY_QUERY_VALUE, &key); // REG #005
     if(result != ERROR_SUCCESS)                                    // REG #005
        { /* failed */                                              // REG #005// REG #006
         ::SetLastError(result);                                    // REG #005// REG #006
         return FALSE;                                              // REG #005
        } /* failed */                                              // REG #005// REG #006
                                                                    // REG #005
     HKEY rkey;                                                     // REG #005
     result = ::RegOpenKeyEx(key,                                   // REG #005// REG #012
                             name,                                  // REG #005
                             0, // reserved                         // REG #005
                             KEY_READ,                              // REG #005
                             &rkey);                                // REG #005
     if(result != ERROR_SUCCESS)                                    // REG #005
        { /* failed */                                              // REG #006
         ::SetLastError(result);                                    // REG #005// REG #006
         return FALSE;                                              // REG #005
        } /* failed */                                              // REG #005// REG #006
                                                                    // REG #005
     ::ZeroMemory(f, sizeof(LOGFONT));                              // REG #005//REG #014
                                                                    // REG #005
     BOOL ok = FALSE;                                               // REG #005
     DWORD datalen;                                                 // REG #005
     //----------------                                             // REG #005
     // lfHeight                                                    // REG #005
     //----------------                                             // REG #005
     datalen = sizeof(f->lfHeight);                                 // REG #005
     result = RegQueryValueEx(rkey, _T("lfHeight"),                 // REG #005
                              NULL, NULL, // reserved, type buffer  // REG #005
                              (LPBYTE)&f->lfHeight,                 // REG #005
                              &datalen);                            // REG #005
     if(result == ERROR_SUCCESS && f->lfHeight > 0)                 // REG #005
        ok = TRUE;                                                  // REG #005
     else                                                           // REG #005// REG #006
        ::SetLastError(result);                                     // REG #005// REG #006
                                                                    // REG #005
     //----------------                                             // REG #005
     // lfWeight                                                    // REG #005
     //----------------                                             // REG #005
     datalen = sizeof(f->lfWeight);                                 // REG #005
     result = RegQueryValueEx(rkey, _T("lfWeight"),                 // REG #005
                              NULL,  NULL, // reserved, type buffer // REG #005
                              (LPBYTE)&f->lfWeight,                 // REG #005
                              &datalen);                            // REG #005
     if(result == ERROR_SUCCESS && f->lfWeight > 0)                 // REG #005
        ok = TRUE;                                                  // REG #005
     else                                                           // REG #005// REG #006
        ::SetLastError(result);                                     // REG #005// REG #006
                                                                    // REG #005
     //----------------                                             // REG #005
     // lfItalic                                                    // REG #005
     //----------------                                             // REG #005
     datalen = sizeof(f->lfItalic);                                 // REG #005
     result = RegQueryValueEx(rkey, _T("lfItalic"),                 // REG #005
                              NULL, NULL,                           // REG #005
                              (LPBYTE)&f->lfItalic,                 // REG #005
                              &datalen);                            // REG #005
     if(result == ERROR_SUCCESS)                                    // REG #005
        ok = TRUE;                                                  // REG #005
     else                                                           // REG #005// REG #006
        ::SetLastError(result);                                     // REG #005// REG #006
                                                                    // REG #005
     //----------------                                             // REG #005
     // lfUnderline                                                 // REG #005
     //----------------                                             // REG #005
     datalen = sizeof(f->lfUnderline);                              // REG #005
     result = RegQueryValueEx(rkey, _T("lfUnderline"),              // REG #005
                              NULL, NULL,                           // REG #005
                              (LPBYTE)&f->lfUnderline,              // REG #005
                              &datalen);                            // REG #005
     if(result == ERROR_SUCCESS)                                    // REG #005
        ok = TRUE;                                                  // REG #005
     else                                                           // REG #005// REG #006
        ::SetLastError(result);                                     // REG #005// REG #006
                                                                    // REG #005
     //----------------                                             // REG #005
     // lfStrikeout                                                 // REG #005
     //----------------                                             // REG #005
     datalen = sizeof(f->lfStrikeOut);                              // REG #005
     result = RegQueryValueEx(rkey, _T("lfStrikeOut"),              // REG #005
                              NULL, NULL,                           // REG #005
                              (LPBYTE)&f->lfStrikeOut,              // REG #005
                              &datalen);                            // REG #005
     if(result == ERROR_SUCCESS)                                    // REG #005
        ok = TRUE;                                                  // REG #005
     else                                                           // REG #005// REG #006
        ::SetLastError(result);                                     // REG #005// REG #006
                                                                    // REG #005
     //----------------                                             // REG #005
     // lfFaceName                                                  // REG #005
     //----------------                                             // REG #005
     datalen = sizeof(f->lfFaceName);                               // REG #005
     result = RegQueryValueEx(rkey, _T("lfFaceName"),               // REG #005
                              NULL, NULL,                           // REG #005
                              (LPBYTE)f->lfFaceName,                // REG #005
                              &datalen);                            // REG #005
     if(result == ERROR_SUCCESS)                                    // REG #005
        ok = TRUE;                                                  // REG #005
     else                                                           // REG #005// REG #006
        ::SetLastError(result);                                     // REG #005// REG #006
                                                                    // REG #005
     ::RegCloseKey(rkey);                                           // REG #005
     ::RegCloseKey(key);                                            // REG #005
                                                                    // REG #005
     return ok;                                                     // REG #005
    } // GetRegistryFont                                            // REG #005

/****************************************************************************
*                         GetRegistryWindowPlacement
* Inputs:
*       HKEY root:
*       const CString & var: base key
*       WINDOWPLACEMENT * wp: Window placement object
* Result: BOOL
*       TRUE if successful; wp is modified, and the length field is also set
*       FALSE if unsuccessful. wp is unchanged
* Effect: 
*       Reads the WindowPlacement from
*               root\path\var\WindowPlacement\...
****************************************************************************/

BOOL GetRegistryWindowPlacement(HKEY root, const CString & var, WINDOWPLACEMENT & wp)// REG #009
   {                                                                // REG #009
    DWORD val;                                                      // REG #009
    //================                                              // REG #009
    // length                                                       // REG #009
    // flags                                                        // REG #009
    //================                                              // REG #009
    if(!GetRegistryInt(root, var, _T("flags"), val))                // REG #009
       return FALSE; // no data                                     // REG #009
                                                                    // REG #009
    wp.length = sizeof(WINDOWPLACEMENT);                            // REG #009
    wp.flags = val;                                                 // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // showCmd                                                      // REG #009
    //================                                              // REG #009
    GetRegistryInt(root, var, _T("showCmd"), val);                  // REG #009
    wp.showCmd = val;                                               // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // ptMinPosition.x                                              // REG #009
    //================                                              // REG #009
    GetRegistryInt(root, var, _T("ptMinPosition\\x"), val);         // REG #009
    wp.ptMinPosition.x = val;                                       // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // ptMinMposition.y                                             // REG #009
    //================                                              // REG #009
    GetRegistryInt(root, var, _T("ptMinPosition\\y"), val);         // REG #009
    wp.ptMinPosition.y = val;                                       // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // ptMaxPosition.x                                              // REG #009
    //================                                              // REG #009
    GetRegistryInt(root, var, _T("ptMaxPosition\\x"), val);         // REG #009
    wp.ptMaxPosition.x = val;                                       // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // ptMaxMposition.y                                             // REG #009
    //================                                              // REG #009
    GetRegistryInt(root, var, _T("ptMaxPosition\\y"), val);         // REG #009
    wp.ptMaxPosition.y = val;                                       // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // rcNormalPosition.left                                        // REG #009
    //================                                              // REG #009
    GetRegistryInt(root, var, _T("rcNormalPosition\\left"), val);   // REG #009
    wp.rcNormalPosition.left = val;                                 // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // rcNormalPosition.top                                         // REG #009
    //================                                              // REG #009
    GetRegistryInt(root, var, _T("rcNormalPosition\\top"), val);    // REG #009
    wp.rcNormalPosition.top = val;                                  // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // rcNormalPosition.right                                       // REG #009
    //================                                              // REG #009
    GetRegistryInt(root, var, _T("rcNormalPosition\\right"), val);  // REG #009
    wp.rcNormalPosition.right = val;                                // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // rcNormalPosition.bottom                                      // REG #009
    //================                                              // REG #009
    GetRegistryInt(root, var, _T("rcNormalPosition\\bottom"), val); // REG #009
    wp.rcNormalPosition.bottom = val;                               // REG #009
                                                                    // REG #009
    return TRUE;                                                    // REG #009
   } // GetRegistryWindowPlacement                                  // REG #009

/****************************************************************************
*                         SetRegistryWindowPlacement
* Inputs:
*       HKEY root:
*       const CString & var: base key
*       const WINDOWPLACEMENT * wp: Window placement object
* Result: BOOL
*       TRUE if successful; wp is modified, and the length field is also set
*       FALSE if unsuccessful. wp is unchanged
* Effect: 
*       Writes the WindowPlacement from
*               root\path\var\WindowPlacement\...
****************************************************************************/

BOOL SetRegistryWindowPlacement(HKEY root, const CString & var, const WINDOWPLACEMENT & wp)// REG #009
   {                                                                // REG #009
    CString s;                                                      // REG #009
    s = _T("flags");                                                // REG #009
    //================                                              // REG #009
    // length                                                       // REG #009
    // flags                                                        // REG #009
    //================                                              // REG #009
    SetRegistryInt(root, var, s, wp.flags);                         // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // showCmd                                                      // REG #009
    //================                                              // REG #009
    s = _T("showCmd");                                              // REG #009
    SetRegistryInt(root, var, s, wp.showCmd);                       // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // ptMinPosition.x                                              // REG #009
    //================                                              // REG #009
    s = _T("ptMinPosition\\x");                                     // REG #009
    SetRegistryInt(root, var, s, wp.ptMinPosition.x);               // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // ptMinMposition.y                                             // REG #009
    //================                                              // REG #009
    s = _T("ptMinPosition\\y");                                     // REG #009
    SetRegistryInt(root, var, s, wp.ptMinPosition.y);               // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // ptMaxPosition.x                                              // REG #009
    //================                                              // REG #009
    s = _T("ptMaxPosition\\x");                                     // REG #009
    SetRegistryInt(root, var, s, wp.ptMaxPosition.x);               // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // ptMaxMposition.y                                             // REG #009
    //================                                              // REG #009
    s = _T("ptMaxPosition\\y");                                     // REG #009
    SetRegistryInt(root, var, s, wp.ptMaxPosition.y);               // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // rcNormalPosition.left                                        // REG #009
    //================                                              // REG #009
    s = _T("rcNormalPosition\\left");                               // REG #009
    SetRegistryInt(root, var, s, wp.rcNormalPosition.left);         // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // rcNormalPosition.top                                         // REG #009
    //================                                              // REG #009
    s =_T("rcNormalPosition\\top");                                 // REG #009
    SetRegistryInt(root, var, s, wp.rcNormalPosition.top);          // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // rcNormalPosition.right                                       // REG #009
    //================                                              // REG #009
    s =_T("rcNormalPosition\\right");                               // REG #009
    SetRegistryInt(root, var, s, wp.rcNormalPosition.right);        // REG #009
                                                                    // REG #009
    //================                                              // REG #009
    // rcNormalPosition.bottom                                      // REG #009
    //================                                              // REG #009
    s = _T("rcNormalPosition\\bottom");                             // REG #009
    SetRegistryInt(root, var, s, wp.rcNormalPosition.bottom);       // REG #009
                                                                    // REG #009
    return TRUE;                                                    // REG #009
   } // SetRegistryWindowPlacement                                  // REG #009
