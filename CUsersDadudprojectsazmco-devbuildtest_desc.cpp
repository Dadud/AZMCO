#include <windows.h>
#include <stdio.h>

struct Desc {
    unsigned int Signature;
    int Size;
    unsigned int Version;
    unsigned int Caps;
    unsigned int MinW, MaxW, MultW, MinH, MaxH, MultH;
    unsigned int ClipAlign;
    unsigned int ActiveFormatCount;
    int* FormatStates;
    unsigned int ActiveUnkCount; int* UnkValues;
    struct { unsigned int Count; void* Caps; } Capabilities;
    unsigned int MaxSimTextures; int Unk7;
    char Name[32]; unsigned int SubType;
    unsigned int MemorySize, MemoryType;
    const char* Author; unsigned int DXV;
    char DeviceName[80];
};

int main() {
    HMODULE h = LoadLibraryA("C:\Program Files (x86)\EA Games\Motor City Online\dx8z.dll");
    if (!h) { printf("LOAD FAILED\n"); return 1; }
    typedef const Desc* (*Fn)(void);
    Fn pFn = (Fn)GetProcAddress(h, "_THRASH_about");
    if (!pFn) { printf("EXPORT NOT FOUND\n"); return 1; }
    const Desc* d = pFn();
    printf("Signature = 0x%08X\n", d->Signature);
    printf("Size = %d\n", d->Size);
    printf("Version = %u\n", d->Version);
    printf("Name = '%.32s'\n", d->Name);
    printf("Author = '%s'\n", d->Author);
    printf("SubType = %u\n", d->SubType);
    printf("DXV = %u\n", d->DXV);
    printf("DeviceName = '%.80s'\n", d->DeviceName);
    FreeLibrary(h);
    return 0;
}
