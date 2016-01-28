#include "stdafx.h"

#include "Seeker.h"

/****************************************************************************
*                               Seeker::Seeker
* Inputs:
*       LPCTSTR drive: File name which has d:\ for d: some drive name
* Effect: 
*       Sets the BytesPerSector value
* Notes:
*       The terminal "\" is required
****************************************************************************/

Seeker::Seeker(LPCTSTR filename)
    {
     TCHAR drive[MAX_PATH];
     _tsplitpath(filename, drive, NULL, NULL, NULL);
     StringCchCat(drive, MAX_PATH, _T("\\"));

     if(_tcslen(drive) == 0)
        { /* failure */
         ASSERT(FALSE);
         throw new SeekerBadFileException;
        } /* failure */
     
     DWORD ignore;
     if(!GetDiskFreeSpace(drive, &ignore, &BytesPerSector, &ignore, &ignore))
        { /* error */
         DWORD err = ::GetLastError();
         ASSERT(FALSE);
         throw new SeekerInternalErrorException(err);
        } /* error */
    } // Seeker::Seeker

/****************************************************************************
*                          Seeker::GetBytesPerSector
* Result: DWORD
*       Number of bytes per sector
****************************************************************************/

DWORD Seeker::GetBytesPerSector()
    {
     return BytesPerSector;
    } // Seeker::GetBytesPerSector

/****************************************************************************
*                       CCreateData::GetBytesPerRecord
* Result: DWORD
*       Size of record, in bytes
* Effect: 
*       Computes the size of a record
****************************************************************************/

DWORD Seeker::GetBytesPerRecord()
   {
    return BytesPerSector * DATABASE_RECORD_MULTIPLE;
   } // CCreateData::GetBytesPerRecord

/****************************************************************************
*                            Seeker::GetSeekOffset
* Inputs:
*       DWORD n: Record number
* Result: DWORD
*       Offset of record in file
* Notes:
*       This is based on a one-sector header
****************************************************************************/

DWORD Seeker::GetSeekOffset(DWORD n)
    {
     DWORD pos = n * GetBytesPerRecord() + BytesPerSector;
     return pos;
    } // Seeker::GetSeekOffset
