#pragma once
#include <WinCrypt.h>

class Rand32 {
    public:
       typedef enum {None, Crypto, Fake} RandType;

       static RandType GetRandType();
       static int rand32();
       static GetLastError() { return error; }
    protected:
       static HCRYPTPROV provider;
       static __declspec(thread) DWORD error;

};

