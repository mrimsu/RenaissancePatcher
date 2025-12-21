#include "restools.h"

BOOL WINAPI ExtractRes(PWSTR Path, PCWSTR FileName, CONST DWORD ResId, PWSTR ResType) {
    
    WCHAR FilePath[MAX_PATH];

    ZeroMemory(FilePath, sizeof(WCHAR) * MAX_PATH);

    wcscpy(FilePath, Path);
    wcscat(FilePath, FileName);

    HRSRC Resource = FindResourceW(NULL, MAKEINTRESOURCE(ResId), ResType);

    if (!Resource) return FALSE;

    DWORD ResSize = SizeofResource(NULL, Resource);
    HGLOBAL ResData = LoadResource(NULL, Resource);
    PVOID ResBuffer = LockResource(ResData);

    HANDLE ResFile = CreateFileW(FilePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    DWORD Written;

    WriteFile(ResFile, ResBuffer, ResSize, &Written, NULL);

    CloseHandle(ResFile);
    FreeResource(ResData);

    return TRUE;
}