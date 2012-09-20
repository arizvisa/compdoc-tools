#include "stdafx.h"

char* wstrdupstr(wchar_t*);
wchar_t* strdupwstr(char*);
void _fatal(char*, unsigned long, char*);
int GetLastErrorString(DWORD, char**);

int
GetLastErrorString(DWORD code, char** string)
{
    int res;
    wchar_t* p;

    res = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
        0,
        code,
        0,
        (LPWSTR)(&p),	//wchar_t**
        0,
        NULL
    );

    if (res != 0)
        *string = wstrdupstr(p);
    else
        *string = NULL;

    LocalFree(p);
    return (res == 0)? 0 : 1;
}

void
_fatal(char* filename, unsigned long linenumber, char* message)
{
    int res;
    DWORD error;
    char* p;

    error = GetLastError();
    (int)fprintf(stderr, "fatal error at %s:%d -> %s\n", filename, linenumber, message);
    (int)fprintf(stderr, "  getlasterror() -> %08x\n", error);

    res = GetLastErrorString(error, &p);
    if (res != 0)
        (int)fprintf(stderr, "  getlasterrorstring() -> %s\n", p);
    exit(1);
}

wchar_t* 
strdupwstr(char* src)
{
    size_t n;
    wchar_t* p;

    n = strlen(src);

    p = calloc(n+1, sizeof(*p));
    if (p == NULL)
        return NULL;

    n = mbstowcs(p, src, n);
    p[n] = 0;
    return p;
}

char* 
wstrdupstr(wchar_t* src)
{
    size_t n;
    char* p;

    n = wcslen(src);
    p = calloc(n+1, sizeof(*p));
    if (p == NULL)
        return NULL;

    n = wcstombs(p, src, n);
    p[n] = 0;
    return p;
}
