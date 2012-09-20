// extractstream
#include "stdafx.h"

void
do_help(int argc, char** argv)
{
	if (argv[0] == NULL)
		argv[0] = "(null)";
	(int)fprintf(stderr, "usage:\n%s storefilename streamindex outfilename\n", argv[0]);
}

int
main(int argc, char** argv)
{
    int res;
    wchar_t* filename;
    int index;
    FILE* out;

    STGOPTIONS options;
    IStorage* storage;

    if (argc < 4) {
        (void)do_help(argc, argv);
        return 1;
    }
    filename = strdupwstr(argv[1]);
    index = atoi(argv[2]);
    out = fopen(argv[3], "wb");
    if (out == NULL)
        fatal("Error opening file to save to");

    res = OpenStore( filename, &options, &storage );
    if (res == 0)
        fatal("Error opening store");
    free(filename);

    res = SaveStream(storage, index, out);
    if (res == 0)
        fatal("Error saving stream");

    (int)CloseStore(storage);
    (int)fclose(out);
    return 0;
}
