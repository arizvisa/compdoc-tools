// storestream
#include "stdafx.h"

void
do_help(int argc, char** argv)
{
	if (argv[0] == NULL)
		argv[0] = "(null)";
	(int)fprintf(stderr, "usage:\n%s storefilename infilename streamindex\n", argv[0]);
}

int
main(int argc, char** argv)
{
    int res;
    wchar_t* filename; wchar_t* streamname;
    int index;
    char* infilename; FILE* in;

    STGOPTIONS options;
    IStorage* storage;

    if (argc < 4) {
        (void)do_help(argc, argv);
        return 1;
    }
    filename = strdupwstr(argv[1]);
    index = atoi(argv[3]);
    
    in = fopen(argv[2], "rb");
    if (in == NULL)
        fatal("Error opening file to read stream data from");

    res = OpenStore( filename, &options, &storage );
    if (res == 0)
        fatal("Error opening store");
    free(filename);

    res = GetStreamName(storage, index, &streamname);
    if (res == 0)
        fatal("Error geting stream name");

    res = AddStream(storage, streamname, in);
    if (res == 0)
        fatal("Error adding stream");

    (void)free(streamname);
    (int)CloseStore(storage);
    (int)fclose(in);
    return 0;
}
