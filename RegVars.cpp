#include "stdafx.h"
#include "registry.h"
#include "regvars.h"

/****************************************************************************
*                             RegistryVar::getInt
* Inputs:
*       UINT id: ID value
*       DWORD & value: Place to put value
*       DWORD def: Default value (= 0)
*       HKEY root: default HKEY_CURRENT_USER
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Retrieves the integer value
****************************************************************************/

BOOL RegistryVar::getInt(UINT id, DWORD & value, DWORD def, HKEY root)
    {
     DWORD v;
     CString pathname;
     pathname.LoadString(id);
     if(!GetRegistryInt(root, pathname, v))
        { /* use default */
         value = def;
         if(::GetLastError() == ERROR_FILE_NOT_FOUND)
            return TRUE;
         return FALSE;
        } /* use default */
     value = v;
     return TRUE;
    }

/****************************************************************************
*                             RegistryVar::getInt64
* Inputs:
*       UINT id: ID value
*       __int64 & value: Place to put value
*       __int64 def: Default value (= 0)
*       HKEY root: default HKEY_CURRENT_USER
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Retrieves the integer value
****************************************************************************/

BOOL RegistryVar::getInt64(UINT id, __int64 & value, __int64 def, HKEY root)
    {
     __int64 v;
     CString pathname;
     pathname.LoadString(id);
     if(!GetRegistryInt64(root, pathname, v))
        { /* use default */
         value = def;
         if(::GetLastError() == ERROR_FILE_NOT_FOUND)
            return TRUE;
         return FALSE;
        } /* use default */
     value = v;
     return TRUE;
    }

/****************************************************************************
*                             RegistryVar::setInt
* Inputs:
*       UINT id: ID to set
*       DWORD value: Value to set
*       HKEY root: default HKEY_CURRENT_USER
* Result: BOOL
*       
* Effect: 
*       Sets the registry key
****************************************************************************/

BOOL RegistryVar::setInt(UINT id, DWORD value, HKEY root)
    {
     CString pathname;

     pathname.LoadString(id);
     return SetRegistryInt(root, pathname, value);
    }

/****************************************************************************
*                             RegistryVar::setInt64
* Inputs:
*       UINT id: ID to set
*       __int64 value: Value to set
*       HKEY root: default HKEY_CURRENT_USER
* Result: BOOL
*       
* Effect: 
*       Sets the registry key
****************************************************************************/

BOOL RegistryVar::setInt64(UINT id, __int64 value, HKEY root)
    {
     CString pathname;

     pathname.LoadString(id);
     return SetRegistryInt64(root, pathname, value);
    }

/****************************************************************************
*                           RegistryVar::setString
* Inputs:
*       UINT id: ID of variable in 
*       LPCTSTR value: String value to save
*       HKEY root: default HKEY_CURRENT_USER
* Result: BOOL
*       
* Effect: 
*       Stores the string value in the registry
****************************************************************************/

BOOL RegistryVar::setString(UINT id, LPCTSTR value, HKEY root)
    {
     CString pathname;

     pathname.LoadString(id);

     return SetRegistryString(root, pathname, value);
    }

/****************************************************************************
*                           RegistryVar::getString
* Inputs:
*       UINT id: ID of subkey
*       CString & s: Place to put string
*       LPCTSTR def: Default value, or NULL
*       HKEY root: default HKEY_CURRENT_USER
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Loads the string from the registry
****************************************************************************/

BOOL RegistryVar::getString(UINT id, CString & value, LPCTSTR def, HKEY root)
    {
     CString pathname;

     pathname.LoadString(id);

     if(!GetRegistryString(root, pathname, value))
        { /* failed */
         if(def != NULL)
            value = def;
         else
            value = _T("");
         return ::GetLastError() == ERROR_FILE_NOT_FOUND;
        } /* failed */
     return TRUE;
    }

/****************************************************************************
*                            RegistryVar::getGUID
* Inputs:
*       UINT id: ID of variable to load
*       GUID & value: GUID
*       const GUID * def: Default, or NULL if no default
*       HKEY root: Root of key
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Loads a GUID from the Registry
****************************************************************************/

BOOL RegistryVar::getGUID(UINT id, GUID & value, const GUID * def, HKEY root)
    {
     CString pathname;

     pathname.LoadString(id);
     if(!GetRegistryGUID(root, pathname, value))
        { /* failed */
         if(def != NULL)
            value = *def;
         else
            ::ZeroMemory(&value, sizeof(GUID));
         return ::GetLastError() == ERROR_ALREADY_EXISTS;
        } /* failed */
     return TRUE;
    } // RegistryVar::getGUID

/****************************************************************************
*                            RegistryVar::setGUID
* Inputs:
*       UINT id: String ID of var
*       const GUID &value: GUID to store
*       HKEY root: Root of key (default HKEY_CURRENT_USER)
* Result: BOOL
*       
* Effect: 
*       Stores the GUID
****************************************************************************/

BOOL RegistryVar::setGUID(UINT id, const GUID &value, HKEY root)
    {
     CString pathname;
     pathname.LoadString(id);
     return SetRegistryGUID(root, pathname, value);
    } // RegistryVar::setGUID

/****************************************************************************
*                              RegistryInt::load
* Result: BOOL
*       TRUE if loaded successfully
*       FALSE if no load occurred
* Effect: 
*       Loads the 'value'
****************************************************************************/

BOOL RegistryInt::load(int def)
    {
     return getInt(id, (DWORD &)value, def, root);
    }

/****************************************************************************
*                             RegistryInt::store
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Stores the 'value'
****************************************************************************/

BOOL RegistryInt::store()
    {
     return setInt(id, value, root);
    }

/****************************************************************************
*                            RegistryString::load
* Inputs:
*       LPCTSTR def: Default value, or NULL if no default
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Loads the string
****************************************************************************/

BOOL RegistryString::load(LPCTSTR def)
    {
     return getString(id, value, def, root);
    }

/****************************************************************************
*                            RegistryString::store
* Result: void
*       
* Effect: 
*       Stores the variable
****************************************************************************/

BOOL RegistryString::store()
    {
     return setString(id, (LPCTSTR)value, root);
    }

/****************************************************************************
*                             RegistryGUID::load
* Inputs:
*       GUID * def: Default value
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Loads the GUID
****************************************************************************/

BOOL RegistryGUID::load(const GUID * def)
    {
     return getGUID(id, value, def, root);
    } // RegistryGUID::load

/****************************************************************************
*                             RegistryGUID::store
* Result: void
*       
* Effect: 
*       Stores the GUID in the Registry
****************************************************************************/

BOOL RegistryGUID::store()
    {
     return setGUID(id, value, root);
    } // RegistryGUID::store

/****************************************************************************
*                             RegistryFont::load
* Inputs:
*       const LOGFONT * def: Default logfont, NULL if no default
* Result: BOOL
*       TRUE if font was loaded
*       FALSE if not found
* Effect: 
*       Loads the font data. If the font is not found and the default
*       is NULL, stores all zeroes in the value
****************************************************************************/

BOOL RegistryFont::load(const LOGFONT * def /* = NULL*/)
    {
     return getFont(id, value, def, root);
    } // RegistryFont::load

/****************************************************************************
*                             RegistryFont::store
* Result: BOOL
*       TRUE if success
*       FALSE if failure
* Effect: 
*       Stores the font
****************************************************************************/

BOOL RegistryFont::store()
    {
     return setFont(id, &value, root);
    } // RegistryFont::store

/****************************************************************************
*                            RegistryVar::getFont
* Inputs:
*       UINT id: Font ID
*       LOGFONT & result: Place to put result
*       const LOGFONT * def: Default value, NULL if none
*       HKEY root: Root of search
* Result: BOOL
*       TRUE if successful
*       FALSE if failed (not found and def == NULL)
* Effect: 
*       Loads the font information from the Registry
****************************************************************************/

BOOL RegistryVar::getFont(UINT id, LOGFONT & result, const LOGFONT * def /* = NULL */, HKEY root /* = HKEY_CURRENT_USER */)
    {
     CString pathname;

     pathname.LoadString(id);
     
     if(!GetRegistryFont(root, pathname, &result))
        { /* failed to load */
         if(def == NULL)
            { /* failed */
             ::ZeroMemory(&result, sizeof(LOGFONT));
             return FALSE;
            } /* failed */
         else
            { /* use default */
             result = *def;
             return TRUE; // "successful" in that default was used
            } /* use default */
        } /* failed to load */
     return TRUE;
    } // RegistryVar::getFont

/****************************************************************************
*                            RegistryVar::setFont
* Inputs:
*       UINT id: ID of key
*       const LOGFONT * font: Font information to write; NULL to delete font
*       HKEY root: Root key to write it in
* Result: BOOL
*       
* Effect: 
*       Writes the font information to the Registry
****************************************************************************/

BOOL RegistryVar::setFont(UINT id, const LOGFONT * font, HKEY root /* = HKEY_CURRENT_USER */)
    {
     CString pathname;

     pathname.LoadString(id);
     return SetRegistryFont(root, pathname, font);
    } // RegistryVar::setFont

/****************************************************************************
*                 RegistryVar::getWindowPlacement
* Inputs:
*       UINT id: ID string
*       WINDOWPLACEMENT & result: Result string
*       const WINDOWPLACEMENT * def: Default value, or NULL if no default
*       HKEY root: Root value
* Result: BOOL
*       TRUE if found, or not found and def != NULL
*       FALSE if not found and def == NULL
* Effect: 
*       Loads the WINDOWPLACEMENT structure
****************************************************************************/

BOOL RegistryVar::getWindowPlacement(UINT id, WINDOWPLACEMENT & result, const WINDOWPLACEMENT * def, HKEY root)
    {
     CString name;
     name.LoadString(id);
     
     BOOL b = GetRegistryWindowPlacement(root, name, result);
     if(!b)
        { /* not found */
         if(def == NULL)
            return FALSE;
         result = *def;
         return TRUE;
        } /* not found */
     return TRUE;
    } // RegistryVar::getWindowPlacement

/****************************************************************************
*                 RegistryWindowVar::setWindowPlacement
* Inputs:
*       UINT id: name of variable
*       const WINDOWPLACEMENT & value: value to write
*       HKEY root: Root
* Result: BOOL
*       
* Effect: 
*       Stores the registry window placement value
****************************************************************************/

BOOL RegistryVar::setWindowPlacement(UINT id, const WINDOWPLACEMENT & value, HKEY root)
    {
     CString name;
     name.LoadString(id);
     
    return SetRegistryWindowPlacement(root, name, value);
    } // RegistryVar::setWindowPlacement

/****************************************************************************
*                        RegistryWindowPlacement::load
* Inputs:
*       const WINDOWPLACEMENT * def: Default position, or NULL
* Result: BOOL
*       TRUE if successful or def != NULL
*       FALSE if key not found and def == NULL
****************************************************************************/

BOOL RegistryWindowPlacement::load(const WINDOWPLACEMENT * def)
    {
     return getWindowPlacement(id, value, def, root);
    } // RegistryWindowPlacement::load

/****************************************************************************
*                       RegistryWindowPlacement::store
* Result: BOOL
*       
* Effect: 
*       Stores the window placement
****************************************************************************/

BOOL RegistryWindowPlacement::store()
    {
     return setWindowPlacement(id, value, root);
    } // RegistryWindowPlacement::store

/****************************************************************************
*                          RegistryDWordArray::load
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Loads the DWORD array from the Registry
* Notes:
*       There is no default value
****************************************************************************/

BOOL RegistryDWordArray::load()
    {
     return GetRegistryDWordArray(root, id, value);
    } // RegistryDWordArray::load

/****************************************************************************
*                          RegistryDWordArray::store
* Result: BOOL
*       TRUE if successful
*       FALSE if error
* Effect: 
*       Stores the CDWordArray
****************************************************************************/

BOOL RegistryDWordArray::store()
    {
     return SetRegistryDWordArray(root, id, value);
    } // RegistryDWordArray::store
