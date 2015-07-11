// addstream
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
    wchar_t* filename; wchar_t* streamname;
    FILE* in;

    IStorage* storage;

    if (argc < 4) {
        (void)do_help(argc, argv);
        return 1;
    }
    filename = strdupwstr(argv[1]);
    streamname = strdupwstr(argv[2]);
    
    in = fopen(argv[3], "rb");
    if (in == NULL)
        fatal("Error opening file to read stream data from");

    res = OpenStore( filename, &storage );
    if (res == 0)
        fatal("Error creating store");
    free(filename);

    res = AddStream(storage, streamname, in);
    if (res == 0)
        fatal("Error adding stream");

    (void)free(streamname);            
    (int)CloseStore(storage);
    (int)fclose(in);
    return 0;
}