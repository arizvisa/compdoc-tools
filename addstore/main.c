// addstore
#include "stdafx.h"

void
do_help(int argc, char** argv)
{
	if (argv[0] == NULL)
		argv[0] = "(null)";
	(int)fprintf(stderr, "usage:\n%s storefilename streamname infilename\n", argv[0]);
}

int
main(int argc, char** argv)
{
    int res;
    wchar_t* filename; wchar_t* streamname; wchar_t* storename;

    IStorage* storage;

    if (argc < 4) {
        (void)do_help(argc, argv);
        return 1;
    }
    filename = strdupwstr(argv[1]);
    streamname = strdupwstr(argv[2]);
    storename = strdupwstr(argv[3]);
    
    res = OpenStore(filename, TRUE, &storage);
    if (res == 0)
        fatal("Error opening store %S", filename);
    free(filename);

    res = AddStore(storage, streamname, storename);
    if (res == 0)
        fatal("Error adding store %S as %S", storename, streamname);

    (void)free(streamname);
    (void)free(storename);
    (int)CloseStore(storage);
    return 0;
}
