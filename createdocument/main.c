// createdocument
#include "stdafx.h"

void
do_help(int argc, char** argv)
{
	if (argv[0] == NULL)
		argv[0] = "(null)";
	(int)fprintf(stderr, "usage:\n%s storefilename\n", argv[0]);
}

int
main(int argc, char** argv)
{
    int res;
    wchar_t* filename;

    IStorage* storage;

    if (argc < 2) {
        (void)do_help(argc, argv);
        return 1;
    }
    filename = strdupwstr(argv[1]);
    
    res = CreateStore( filename, &storage );
    if (res == 0)
        fatal("Error creating store");
    free(filename);

    (int)CloseStore(storage);
    return 0;
}