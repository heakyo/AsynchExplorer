#pragma once

#define DATABASE_RECORD_MULTIPLE 2
class SeekerException : public CException {
};

class SeekerBadFileException : public SeekerException {
};

class SeekerInternalErrorException : public SeekerException {
    public:
       SeekerInternalErrorException(DWORD e) { err = e; }
       DWORD GetError() { return err; }
    protected:
       DWORD err;
};

class Seeker {
    public:
       Seeker(LPCTSTR drive);
       DWORD GetBytesPerRecord();
       DWORD GetBytesPerSector();
       DWORD GetSeekOffset(DWORD n);
       long GetRecordCount() { return nrecs; }
       void SetRecordCount(long n) { nrecs = n; }
    protected:
       DWORD BytesPerSector;
       long nrecs;
};


