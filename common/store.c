/*
	Should do all things related to stores. Like dumping and enumerations, etc

    TODO: probably should use a real list implementation so we aren't enumerating
          everything in order to get a name by index..
*/

#include "stdafx.h"

char g_TransferBuffer[0x200];       // blockcopy

int
CreateStore(wchar_t* filename, IStorage** out)
{
    int res;

    assert(filename != NULL);
    assert(out != NULL);

    (int)fprintf(stderr, "Creating store %S\n", filename);
    res = StgCreateStorageEx(
        filename,
        STGM_CREATE | STGM_FAILIFTHERE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
        STGFMT_STORAGE,
        0, NULL, NULL, &IID_IStorage,
        out
    );
    return (res == S_OK)? 1 : 0;
}

int
OpenStore(wchar_t* filename, IStorage** out)
{
    int res;

    assert(filename != NULL);
    assert(out != NULL);

    (int)fprintf(stderr, "Opening store %S\n", filename);
    res = StgOpenStorageEx(
        filename,
        STGM_READWRITE | STGM_SHARE_DENY_NONE | STGM_TRANSACTED, STGFMT_STORAGE,
        0, NULL, NULL, &IID_IStorage,
        out);

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
        (int)fprintf(stdout, "%d:%s:%s\n", index++, s, STGTYtoString(stat.type));
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
GetStreamType(IStorage* storage, int index, STGTY* out)
{
    int res;
    
    ULONG count;
    LPENUMSTATSTG enumstatstg;
    STATSTG stat;

    assert(storage != NULL);
    assert(out != NULL);
	*out = 0;

    res = storage->lpVtbl->EnumElements(storage, 0, NULL, 0, &enumstatstg);
    if (res != S_OK)
        return 0;

    while (index >= 0) {
        res = enumstatstg->lpVtbl->Next(enumstatstg, 1, &stat, &count);
        if (res != S_OK)
            return 0;
        index--;
    }
    *out = stat.type;
    enumstatstg->lpVtbl->Release(enumstatstg);
    return 1;
}

int
GetStream(IStorage* storage, int index, IStream** out)
{
    int res;
    wchar_t* name;
    STGTY type;

    assert(storage != NULL); assert(out != NULL);
    *out = NULL;

    res = GetStreamType(storage, index, &type);
    if ((res == 0) || (type != STGTY_STREAM))
        return 0;

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
    STGTY type;

    assert(storage != NULL); assert(out != NULL);
    *out = NULL;

    res = GetStreamType(storage, index, &type);
    if ((res == 0) || (type != STGTY_STORAGE))
        return 0;

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
    int res; ULONG bytesread;
    STATSTG st;

    IStream* stream;

    IStorage* temporary; IStorage* source;
    FILE* in;

    LARGE_INTEGER li;
    (void)memset(&li, 0, sizeof(li));

    res = GetStreamType(storage,index,&st.type);
    if (res == 0)
        return 0;

    if (st.type == STGTY_STREAM) {
        fprintf(stderr, "Fetching stream\n");
        res = GetStream(storage, index, &stream);
    } else if (st.type == STGTY_STORAGE) {
        fprintf(stderr, "Fetching store\n");
        res = GetStorage(storage, index, &source);
    } else {
        fprintf(stderr, "Fetching unknown %x\n", st.type);
        res = 0;
    }
    if (res == 0)
        return 0;

    switch(st.type) {
    case STGTY_STREAM:
        fprintf(stderr, "Seeking to beginning of stream\n");
        (HRESULT)stream->lpVtbl->Seek(stream, li, STREAM_SEEK_SET, NULL);

        fprintf(stderr, "Writing to file\n");
        /* write stream to file */
        for (;;) {
            res = stream->lpVtbl->Read(stream, g_TransferBuffer, sizeof(g_TransferBuffer), &bytesread);
            if (res != S_OK) { res = 1; goto stream_release; }
            (size_t)fwrite(g_TransferBuffer, 1, bytesread, out);
            if (bytesread < sizeof(g_TransferBuffer))
                break;
        }
        res = 1;
        fprintf(stderr, "Done\n");

stream_release:
        (ULONG)stream->lpVtbl->Release(stream);
        break;

    case STGTY_STORAGE:
        fprintf(stderr, "Creating temporary storage on disk\n");
        res = StgCreateStorageEx(NULL, STGM_CREATE|STGM_READWRITE|STGM_SHARE_EXCLUSIVE|STGM_DELETEONRELEASE, STGFMT_STORAGE, 0, NULL, NULL, &IID_IStorage, &temporary);
        if (res != S_OK) { res = 0; goto source_release; }

        fprintf(stderr, "Copying store to temporary storage\n");
        res = source->lpVtbl->CopyTo(source, 0, NULL, NULL, temporary);
        if (res != S_OK) { res = 0; goto temporary_release; }

        res = source->lpVtbl->Commit(source, STGC_DEFAULT);
        if (res != S_OK) { res = 0; goto temporary_release; }

        fprintf(stderr, "Getting temporary storage filename\n");
        res = temporary->lpVtbl->Stat(temporary, &st, STATFLAG_DEFAULT);
        if (res != S_OK) { res = 0; goto temporary_release; }

        fprintf(stderr, "Opening temporary storage %S\n", st.pwcsName);
        in = _wfopen(st.pwcsName, L"rb");
        if (in == NULL) { res = 0; goto temporary_release; }

        fprintf(stderr, "Writing to file\n");
        for (;;) {
            bytesread = fread(&g_TransferBuffer, 1, sizeof(g_TransferBuffer), in);
            (size_t)fwrite(g_TransferBuffer, 1, bytesread, out);
            if (bytesread < sizeof(g_TransferBuffer))
                break;
        }
        fclose(in);

        res = 1;
        fprintf(stderr, "Done\n");

temporary_release:
        temporary->lpVtbl->Release(temporary);
source_release:
        source->lpVtbl->Release(source);
        break;
    }
    return res;
}

int
AddStore(IStorage* storage, wchar_t* storename, wchar_t* sourcename)
{
    int res;
    IStorage* store; IStorage* source;
    LARGE_INTEGER li;
    ULARGE_INTEGER uli;
    memset(&li, 0, sizeof(li));
    memset(&uli, 0, sizeof(uli));

    assert(storage != NULL);
    assert(storename != NULL);
    assert(sourcename != NULL);

    res = storage->lpVtbl->DestroyElement(storage, storename);
    if (res == S_OK)
        fprintf(stderr, "Deleted already existing store %S\n", storename);
    else if (res != STG_E_FILENOTFOUND)
        return 0;

    fprintf(stderr, "Creating store %S\n", storename);
    res = storage->lpVtbl->CreateStorage(storage, storename, STGM_WRITE | STGM_SHARE_EXCLUSIVE, 0, 0, &store);
    if (res != S_OK)
        return 0;

    res = OpenStore(sourcename, &source);
    if (res == 0)
        return 0;

    fprintf(stderr, "Copying source to storage\n");
    res = source->lpVtbl->CopyTo(source, 0, NULL, 0, store);
    if (res != S_OK)
        return 0;

    fprintf(stderr, "Committing\n");
    res = store->lpVtbl->Commit(store, STGC_DEFAULT);
    if (res != S_OK)
        return 0;

    (ULONG)store->lpVtbl->Release(store);
    (ULONG)source->lpVtbl->Release(source);
    fprintf(stderr, "Done\n");
    return 1;
}
