#define DATABASE_HEADER_BYTES 8

typedef struct {
   char reccount[DATABASE_HEADER_BYTES];
} Header;

