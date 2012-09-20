#ifndef __utils_h
#define __utils_h

#include <windows.h>	// for DWORD

char* wstrdupstr(wchar_t* src);
wchar_t* strdupwstr(char* src);
void _fatal(char* filename, unsigned long linenumber, char* message);
int GetLastErrorString(DWORD code, char** string);

#define fatal(x) do { (void)_fatal(__FILE__,__LINE__,(x)); } while (0)


#endif