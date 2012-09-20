#ifndef __store_h
#define __store_h

int OpenStore(wchar_t* filename, STGOPTIONS* opts, IStorage** out);
int CloseStore(IStorage* storage);
int GetStreamName(IStorage* storage, int index, wchar_t** out);
void PrintStreams(IStorage* storage);

int GetStream(IStorage* storage, int index, IStream** out);
int GetStorage(IStorage* storage, int index, IStorage** out);

/* TODO: will need to add methods for creating streams and storages for a storage too */
int SaveStream(IStorage* storage, int index, FILE* out);
int AddStream(IStorage* storage, wchar_t* streamname, FILE* in);

#endif