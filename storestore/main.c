// storestore
#include "stdafx.h"

void
do_help(int argc, char** argv)
{
	if (argv[0] == NULL)
		argv[0] = "(null)";
	(int)fprintf(stderr, "usage:\n%s storefilename infilename storeindex\n", argv[0]);
}

int
main(int argc, char** argv)
{
    int res;
    wchar_t* filename; wchar_t* storename; wchar_t* sourcename;
    int index;

    IStorage* storage;

    if (argc < 4) {
        (void)do_help(argc, argv);
        return 1;
    }
    filename = strdupwstr(argv[1]);
    index = atoi(argv[3]);
    sourcename = strdupwstr(argv[2]);
    
    res = OpenStore( filename, &storage );
    if (res == 0)
        fatal("Error opening store");
    free(filename);

    res = GetStreamName(storage, index, &storename);
    if (res == 0)
        fatal("Error geting store name");

    res = AddStore(storage, storename, sourcename);
    if (res == 0)
        fatal("Error adding store");

    (void)free(storename);
    (void)free(sourcename);
    (int)CloseStore(storage);
    return 0;
}
