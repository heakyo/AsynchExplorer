/*****************************************************************************
*           Change Log
*  Date     | Change
*-----------+-----------------------------------------------------------------
*  3-Jun-99 | Created
*  3-Jun-99 | FBG #210: DWordArray values overloaded on UINT for IDS_name
* 11-Jun-99 | FBG #263: Added operations to support FBG #263
* 23-Mar-04 | FBG #334: Added functions from newer versions of the file
* 24-Mar-04 | PPR #1303: Added support to get key path
* 24-Mar-04 | PPR #1377: Added font support
* 26-Mar-04 | FBG #340: Upgraded to new versions of EnumRegistryKeys,
*           | EnumRegistryValues, Set/SetRegistryDWordArray
*****************************************************************************/
// Copyright © 2005, Joseph M. Newcomer, All Rights Reserved
// You must record any changes you make to this source by putting // and your
// initials at the end of each line you change. You may not remove any lines
// from this source; instead, put // in front of them and your initials at the end.
// You may not charge any fee for the redistribution, nor incorporate this code in
// any commercial product. This is freeware, and must remain so.
//
BOOL GetRegistryString(HKEY root, const CString &var, CString &val);
BOOL SetRegistryString(HKEY root, const CString &var, const CString &val);

BOOL GetRegistryString(HKEY root, const CString &path, const CString &var, CString &val);
BOOL SetRegistryString(HKEY root, const CString &path, const CString &var, const CString &val);
BOOL GetRegistryString(HKEY root, const CString &path, UINT var, CString &val);
BOOL SetRegistryString(HKEY root, const CString &path, UINT var, const CString &val);

         BOOL SetRegistryInt(HKEY root, const CString &var, DWORD val);
__inline BOOL SetRegistryInt(HKEY root, const CString &var, int val) {return SetRegistryInt(root, var, (DWORD)val); }

         BOOL GetRegistryInt(HKEY root, const CString &var, DWORD & val);
__inline BOOL GetRegistryInt(HKEY root, const CString &var, int & val) { return GetRegistryInt(root, var, (DWORD &) val); }

         BOOL GetRegistryInt(HKEY root, const CString &path, const CString &var, DWORD & val);
__inline BOOL GetRegistryInt(HKEY root, const CString &path, const CString &var, int & val) { return GetRegistryInt(root, path, var, (DWORD &)val); }

         BOOL SetRegistryInt(HKEY root, const CString &path, const CString &var, DWORD val);

         BOOL GetRegistryInt(HKEY root, const CString &path, UINT var, DWORD &val);

         BOOL SetRegistryInt(HKEY root, const CString &path, UINT var, DWORD val);
__inline BOOL SetRegistryInt(HKEY root, const CString &path, UINT var, int val) { return SetRegistryInt(root, path, var, (DWORD)val); }

BOOL GetRegistryGUID(HKEY root, const CString &var, GUID &val);
BOOL SetRegistryGUID(HKEY root, const CString &var, const GUID &val);

BOOL DeleteRegistryValue(HKEY root, const CString & var);
BOOL DeleteRegistryKey(HKEY root, const CString & keyname);

BOOL GetRegistryKey(HKEY root, const CString & name, HKEY & key, DWORD access = KEY_ALL_ACCESS);// REG #012
BOOL FindRegistryKey(HKEY root, const CString & keyname, HKEY & key, DWORD access = KEY_ALL_ACCESS);// REG #012

BOOL EnumRegistryKeys(HKEY root, const CString & group, CStringArray & keys);// FBG #340
BOOL EnumRegistryKeys(HKEY root, UINT group, CStringArray & keys); // REG #006

BOOL EnumRegistryValues(HKEY root, const CString & group, CStringArray & keys);// FBG #340

BOOL SetRegistryValues(HKEY root, const CString & group, const CStringArray & values);
BOOL SetRegistryValues(HKEY root, UINT group, const CStringArray & values);
BOOL SetRegistryValues(HKEY root, const CString & group, const CDWordArray & values);
BOOL SetRegistryValues(HKEY root, UINT group, const CDWordArray & values);

BOOL GetRegistryValues(HKEY root, const CString & group, CStringArray & values);
BOOL GetRegistryValues(HKEY root, UINT group, CStringArray & values);
BOOL GetRegistryValues(HKEY root, const CString & group, CDWordArray & values);
BOOL GetRegistryValues(HKEY root, UINT group, CDWordArray & values);

BOOL GetRegistryDWordArray(HKEY root, const CString &var, CDWordArray & data);// FBG #340
BOOL GetRegistryDWordArray(HKEY root, UINT id, CDWordArray & data);            // FBG #210// FBG #340
BOOL SetRegistryDWordArray(HKEY root, const CString & var, const CDWordArray & data);// FBG #340
BOOL SetRegistryDWordArray(HKEY root, UINT var, const CDWordArray & data);// FBG #210// FBG #340

BOOL SetRegistryDWordArray(HKEY root, const CString & var, CDWordArray & data);

BOOL CreateKey(HKEY root, const CString & key, DWORD access = KEY_ALL_ACCESS); // FBG #263// REG #012
HKEY OpenKey(HKEY root, const CString & path, DWORD access = KEY_ALL_ACCESS); // FBG #263// REG #012

CString GetKeyPath(const CString & key);                           // PPR #1303
CString GetKeyPath(UINT key);                                      // PPR #1303

BOOL GetRegistryFont(HKEY root, const CString & var, LPLOGFONT f); // PPR #1377
BOOL SetRegistryFont(HKEY root, const CString & var, const LOGFONT * f);// PPR #1377

BOOL GetRegistryWindowPlacement(HKEY root, const CString & var, WINDOWPLACEMENT & wp);// REQ #088
BOOL SetRegistryWindowPlacement(HKEY root, const CString & var, const WINDOWPLACEMENT & wp);// REQ #088

         BOOL GetRegistryInt64(HKEY root, const CString & var,          __int64 & val); // REG #013
__inline BOOL GetRegistryInt64(HKEY root, const CString & var, unsigned __int64 & val) { return GetRegistryInt64(root, var, (__int64 &)val); } // REG #013

         BOOL SetRegistryInt64(HKEY root, const CString & var,          __int64 val);   // REG #013
__inline BOOL SetRegistryInt64(HKEY root, const CString & var, unsigned __int64 val) { return SetRegistryInt64(root, var, (__int64)val); }   // REG #013
