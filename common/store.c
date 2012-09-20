/*
	Should do all things related to stores. Like dumping and enumerations, etc

    TODO: probably should use a real list implementation so we aren't enumerating
          everything in order to get a name by index..
*/

#include "stdafx.h"

char g_TransferBuffer[0x200];       // blockcopy

int
OpenStore(wchar_t* filename, STGOPTIONS* opts, IStorage** out)
{
    int res;

    assert(filename != NULL);
    assert(opts != NULL);
    assert(out != NULL);

    (int)fprintf(stderr, "Opening store %S\n", filename);
    res = StgOpenStorage(
        filename,
        NULL,
        STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
        NULL,
        0,
        out);

    #if 0
        res = StgOpenStorageEx(
            filename,
            STGM_READ | STGM_SHARE_DENY_NONE,
            STGFMT_ANY,
            0,
            NULL,
            NULL,
            IID_IStorage,
            out);
    #endif
    
    return (res == S_OK)? 1 : 0;
}

int
CloseStore(IStorage* storage)
{
    int res;
    assert(storage != NULL);
    res = storage->lpVtbl->Release(storage);
    return (res == S_OK)? 1 : 0;
}

char* _STGTYENUM[] = {"UNKNOWN", "STORAGE", "STREAM", "LOCKBYTES", "PROPERTY"};

char*
STGTYtoString(int stgty)
{ return _STGTYENUM[stgty]; }

void
PrintStreams(IStorage* storage)
{
    int res, index = 0;
    ULONG count;
    LPENUMSTATSTG enumstatstg;
    STATSTG stat;
    char* s;

    res = storage->lpVtbl->EnumElements(storage, 0, NULL, 0, &enumstatstg);
    assert(res == S_OK);

    while (enumstatstg->lpVtbl->Next(enumstatstg, 1, &stat, &count) == S_OK) {
        s = wstrdupstr(stat.pwcsName);
        (int)fprintf(stderr, "[%d] %s -- %s\n", index++, s, STGTYtoString(stat.type));
        (void)free(s);
    }
    enumstatstg->lpVtbl->Release(enumstatstg);
}

int
GetStreamName(IStorage* storage, int index, wchar_t** out)
{
    int res;
    
    ULONG count;
    LPENUMSTATSTG enumstatstg;
    STATSTG stat;

    assert(storage != NULL);
    assert(out != NULL);
	*out = NULL;

    res = storage->lpVtbl->EnumElements(storage, 0, NULL, 0, &enumstatstg);
    if (res != S_OK)
        return 0;

    while (index >= 0) {
        res = enumstatstg->lpVtbl->Next(enumstatstg, 1, &stat, &count);
        if (res != S_OK)
            return 0;
        index--;
    }
    *out = _wcsdup(stat.pwcsName);
    enumstatstg->lpVtbl->Release(enumstatstg);
	return (*out == NULL)? 0 : 1;
}

int
GetStream(IStorage* storage, int index, IStream** out)
{
    int res;
    wchar_t* name;

    assert(storage != NULL); assert(out != NULL);
    *out = NULL;

    res = GetStreamName(storage, index, &name);
    if (res == 0)
        return 0;

    res = storage->lpVtbl->OpenStream(storage, name, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, out);
    free(name);

    return (res == S_OK)? 1 : 0;
}

int
GetStorage(IStorage* storage, int index, IStorage** out)
{
    int res;
    wchar_t* name;

    assert(storage != NULL); assert(out != NULL);
    *out = NULL;

    res = GetStreamName(storage, index, &name);
    if (res == 0)
        return 0;

    res = storage->lpVtbl->OpenStorage(storage, name, NULL, STGM_READWRITE | STGM_SHARE_EXCLUSIVE, NULL, 0, out);
    free(name);
    return (res == S_OK)? 1 : 0;
}

int
AddStream(IStorage* storage, wchar_t* streamname, FILE* in)
{
    int res;
    IStream* stream;
    size_t bytesread;
    LARGE_INTEGER li;
    ULARGE_INTEGER uli;
    memset(&li, 0, sizeof(li));
    memset(&uli, 0, sizeof(uli));

    assert(storage != NULL);
    assert(streamname != NULL);
    assert(in != NULL);

    res = storage->lpVtbl->DestroyElement(storage, streamname);
    if (res == S_OK)
        fprintf(stderr, "Deleted already existing stream %S\n", streamname);
    else if (res != STG_E_FILENOTFOUND)
        return 0;

    fprintf(stderr, "Creating stream %S\n", streamname);
    res = storage->lpVtbl->CreateStream(storage, streamname, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &stream);
    if (res != S_OK)
        return 0;

    fprintf(stderr, "Resetting stream\n");
    res = stream->lpVtbl->SetSize(stream, uli);
    if (res != S_OK)
        return 0;

    res = stream->lpVtbl->Seek(stream, li, STREAM_SEEK_SET, NULL);
    if (res != S_OK)
        return 0;

    fprintf(stderr, "Writing to stream\n");

    /* open filename and store it to stream */
    while (!feof(in)) {
        bytesread = fread(g_TransferBuffer, 1, sizeof(g_TransferBuffer), in);
        res = stream->lpVtbl->Write(stream, g_TransferBuffer, bytesread, NULL);
        if (res != S_OK)
            return 0;
    }

    fprintf(stderr, "Committing\n");
    res = stream->lpVtbl->Commit(stream, STGC_DEFAULT);
    if (res != S_OK)
        return 0;
    (ULONG)stream->lpVtbl->Release(stream);

    res = storage->lpVtbl->Commit(storage, STGC_DEFAULT);
    if (res != S_OK)
        return 0;

    fprintf(stderr, "Done\n");
    return 1;
}

int
SaveStream(IStorage* storage, int index, FILE* out)
{
    int res;
    ULONG bytesread;
    IStream* stream;
    LARGE_INTEGER li;
    (void)memset(&li, 0, sizeof(li));

    fprintf(stderr, "Fetching stream\n");
    res = GetStream(storage, index, &stream);
    if (res == 0)
        return 0;

    fprintf(stderr, "Seeking to beginning of stream\n");
    (HRESULT)stream->lpVtbl->Seek(stream, li, STREAM_SEEK_SET, NULL);

    fprintf(stderr, "Writing to file\n");
    /* write stream to file */
    for (;;) {
        res = stream->lpVtbl->Read(stream, g_TransferBuffer, sizeof(g_TransferBuffer), &bytesread);
        if (res != S_OK)
            return 0;
        (size_t)fwrite(g_TransferBuffer, 1, bytesread, out);
        if (bytesread < sizeof(g_TransferBuffer))
            break;
    }

    fprintf(stderr, "Done\n");
    (ULONG)stream->lpVtbl->Release(stream);
    return 1;
}