/*****************************************************************************
*           Change Log
*  Date     | Change
*-----------+-----------------------------------------------------------------
* 30-Aug-99 | [1.384] Created change log
* 30-Aug-99 | [1.384] Added RegistryGUID
* 26-Dec-99 | [1.387] REQ #134: Minor changes to handle Lint warnings
*  5-May-04 | Rework. Values are BOOL. Added types for DWORD arrays, fonts, etc.
* 28-Mar-05 | Added remove()
* 25-Mar-05 | Added new data types: RegistryWindowPlacement, RegistryGUID,
*           | RegistryFont, RegistryInt64, RegDWordArray
*****************************************************************************/
// Copyright © 2005, Joseph M. Newcomer, All Rights Reserved
// You must record any changes you make to this source by putting // and your
// initials at the end of each line you change. You may not remove any lines
// from this source; instead, put // in front of them and your initials at the end.
// You may not charge any fee for the redistribution, nor incorporate this code in
// any commercial product. This is freeware, and must remain so.
//

class RegistryVar {
    public:
        static BOOL setInt(UINT id, DWORD value, HKEY root = HKEY_CURRENT_USER);
        static BOOL getInt(UINT id, DWORD & value, DWORD def = 0, HKEY root = HKEY_CURRENT_USER);

        static BOOL setString(UINT id, LPCTSTR s, HKEY root = HKEY_CURRENT_USER);
        static BOOL getString(UINT id, CString & s, LPCTSTR def = NULL, HKEY root = HKEY_CURRENT_USER);

        static BOOL setGUID(UINT id, const GUID & value, HKEY root = HKEY_CURRENT_USER);
        static BOOL getGUID(UINT id, GUID & result, const GUID * def, HKEY root = HKEY_CURRENT_USER);

        static BOOL setInt64(UINT id, __int64 value, HKEY root = HKEY_CURRENT_USER);
        static BOOL getInt64(UINT id, __int64 & value, __int64 def, HKEY root = HKEY_CURRENT_USER);

        static BOOL setFont(UINT id, const LOGFONT * value, HKEY root = HKEY_CURRENT_USER);
        static BOOL getFont(UINT id, LOGFONT & result, const LOGFONT * def = NULL, HKEY root = HKEY_CURRENT_USER);

        static BOOL setWindowPlacement(UINT id, const WINDOWPLACEMENT & value, HKEY root = HKEY_CURRENT_USER);
        static BOOL getWindowPlacement(UINT id, WINDOWPLACEMENT & result, const WINDOWPLACEMENT * def = NULL, HKEY root = HKEY_CURRENT_USER);
    protected:
        RegistryVar(UINT n, HKEY r) { id = n; root = r; }
        BOOL remove();
        UINT id;
        HKEY root;
               };

/****************************************************************************
*                                 RegistryInt
****************************************************************************/

/*lint -e1735 */
class RegistryInt : public RegistryVar {
     public:
        RegistryInt(UINT n, HKEY r = HKEY_CURRENT_USER ) : RegistryVar(n, r) { value = 0; } // REQ #134
        virtual BOOL load(int def = 0);
        virtual BOOL store();

        int value; // the value
                                       };

/****************************************************************************
*                                RegistryInt64
****************************************************************************/
            
class RegistryInt64 : public RegistryVar {
    public:
       RegistryInt64(UINT n, HKEY r = HKEY_CURRENT_USER) : RegistryVar(n, r) { value = 0; }
       virtual BOOL load(__int64 def = 0);
       virtual BOOL store();

       __int64 value;
};



/****************************************************************************
*                                RegistryString
****************************************************************************/

class RegistryString : public RegistryVar {
     public:
        RegistryString(UINT n, HKEY r = HKEY_CURRENT_USER) : RegistryVar(n, r) { }
        virtual BOOL load(LPCTSTR def = NULL);
        virtual BOOL store();

        CString value; // the value
};

/****************************************************************************
*                                 RegistryGUID
****************************************************************************/
                                       
#ifdef GUID_DEFINED
class RegistryGUID : public RegistryVar {
    public:
       RegistryGUID(UINT n, HKEY r = HKEY_CURRENT_USER) : RegistryVar(n, r) { ::ZeroMemory(&value, sizeof(value)); }
       virtual BOOL load(const GUID * def = NULL);
       virtual BOOL store();

       GUID value; // the value
};
#endif

/****************************************************************************
*                                 RegistryFont
****************************************************************************/
class RegistryFont : public RegistryVar {
    public:
       RegistryFont(UINT n, HKEY r = HKEY_CURRENT_USER) : RegistryVar(n, r) { ::ZeroMemory(&value, sizeof(value)); }
       virtual BOOL load(const LOGFONT * def = NULL);
       virtual BOOL store();

       LOGFONT value;
};

/****************************************************************************
*                           RegistryWindowPlacement
****************************************************************************/
class RegistryWindowPlacement : public RegistryVar {
    public:
       RegistryWindowPlacement(UINT n, HKEY r = HKEY_CURRENT_USER) : RegistryVar(n, r) { ::ZeroMemory(&value, sizeof(value)); }
       virtual BOOL load(const WINDOWPLACEMENT * def = NULL);
       virtual BOOL store();

       WINDOWPLACEMENT value;
};

/****************************************************************************
*                             RegistryDWordArray
****************************************************************************/

class RegistryDWordArray : public RegistryVar {
    public:
       RegistryDWordArray(UINT n, HKEY r = HKEY_CURRENT_USER) : RegistryVar(n, r) { value.SetSize(0); }
       virtual BOOL load();
       virtual BOOL store();

       CDWordArray value;
    }; // RegistryDWordArray
