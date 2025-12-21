#include "flashpatch.h"

CONST BYTE FlashConst[16] = {110, 219, 124, 210, 109, 174, 207, 17, 150, 184, 68, 69, 83, 84, 0, 0},
    PatchedFlashConst[16] = {146, 63, 64, 234, 153, 13, 10, 70, 176, 39, 220, 101, 108, 237, 23, 81};

BOOL WINAPI RegisterFlashClass(VOID) {
    HKEY Hkey;
    RegCreateKeyW(HKEY_CLASSES_ROOT, L"CLSID\\{EA403F92-0D99-460A-B027-DC656CED1751}\\InprocServer32", &Hkey);
    
    PCWSTR Value = L"flashdll";

    if (RegSetValueW(Hkey, NULL, REG_SZ, Value, wcslen(Value) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        RegCloseKey(Hkey);
        return FALSE;
    }
    RegCloseKey(Hkey);

    return TRUE;
}

BOOL WINAPI PatchFlash(PWSTR Input) {
    HANDLE MagentPe = CreateFileW(Input, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (!MagentPe) return FALSE;

	HANDLE FileMap = CreateFileMappingW(MagentPe, NULL, PAGE_READWRITE, 0, 0, NULL);

	SIZE_T PeSize = GetFileSize(MagentPe, 0);

	PBYTE MapBuffer = (PBYTE) MapViewOfFile(FileMap, FILE_MAP_ALL_ACCESS, 0, 0, PeSize);
	
	if (!MapBuffer) return FALSE;

    if (!RegisterFlashClass()) {
        UnmapViewOfFile(MapBuffer);
	    CloseHandle(MagentPe);
	    CloseHandle(FileMap);
        return FALSE;
    } 

    for (DWORD a = 0; a + 16 < PeSize; a++) {
        if (memcmp(&MapBuffer[a], FlashConst, 16) == 0) {
            memcpy(&MapBuffer[a], PatchedFlashConst, 16);
        }
    }

	UnmapViewOfFile(MapBuffer);
	CloseHandle(MagentPe);
	CloseHandle(FileMap);
    return TRUE;
}

