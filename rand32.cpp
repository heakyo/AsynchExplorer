#include "stdafx.h"
#include "rand32.h"
#include <WinCrypt.h>

#define RAND32() ((int)(((DWORD)rand() << 17) ^ ((DWORD)rand() << 9) ^ rand()))

/* protected: static */ HCRYPTPROV Rand32::provider = NULL;

__declspec(thread) DWORD Rand32::error = 0;

/****************************************************************************
*                                   rand32
* Result: int
*       A 32-bit random number
* Notes:
*       Sets ERROR_SUCCESS if successful
****************************************************************************/

/* static */ int Rand32::rand32()
    {
     error = ERROR_SUCCESS;

     if(provider == NULL)
        { /* get provider */
        if(!::CryptAcquireContext(&provider,
                                 NULL, // use default cryptographic container
                                 NULL, // use default cryptographic provider
                                 PROV_RSA_FULL, // as good a choice as any
                                 CRYPT_NEWKEYSET | CRYPT_SILENT))
            { /* failed */
             error = ::GetLastError();
             provider = (HCRYPTPROV)INVALID_HANDLE_VALUE;
            } /* failed */
        } /* get provider */

     if(provider != (HCRYPTPROV)INVALID_HANDLE_VALUE)
        { /* get number */
         int result;
         
         if(::CryptGenRandom(provider, sizeof(int), (LPBYTE)&result))
            return result;

         error = ::GetLastError();
         ::CryptReleaseContext(provider, 0); // failed, why?
         provider = (HCRYPTPROV)INVALID_HANDLE_VALUE;
        } /* get number */

     return RAND32();

    } // rand32


/****************************************************************************
*                                 GetRandType
* Result: RandType
*       Type of random number returned
****************************************************************************/

/* static */ Rand32::RandType Rand32::GetRandType()
    {
     if(provider == NULL)
        return None;
     if(provider == (HCRYPTPROV)INVALID_HANDLE_VALUE)
        return Fake;
     return Crypto;
    } // GetRandType
