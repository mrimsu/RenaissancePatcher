#include "bytepatch.h"

static CONST BYTE FlashPattern[16] = {110, 219, 124, 210, 109, 174, 207, 17, 150, 184, 68, 69, 83, 84, 0, 0},
    FlashPatched[16] = {146, 63, 64, 234, 153, 13, 10, 70, 176, 39, 220, 101, 108, 237, 23, 81};

static CONST BYTE AttribPattern[6] = {0x24, 0xC1, 0xE8, 0x1F, 0xF7, 0xD0},
    AttribPatched[3] = {0x31, 0xC0, 0x40};

static CONST DWORD NullDerefOffset = 0x2CF0BC;
    
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

BOOL WINAPI PatchBytes(PWSTR Input) {
    BOOL IsAgent65 = FALSE;

    DWORD Unused = 0;
	DWORD VerSize = GetFileVersionInfoSizeW(Input, &Unused);
	
    if (VerSize == 0) {
        return FALSE;
    }
    
    PVOID Version = (PVOID)GlobalAlloc(GMEM_ZEROINIT, VerSize * sizeof(VS_FIXEDFILEINFO));

	if (!GetFileVersionInfoW(Input, 0, VerSize, Version)) {
        GlobalFree(Version);
        return FALSE;
    }

	VS_FIXEDFILEINFO *FileVer = NULL;
	UINT FileVerSize = 0;
    
    if (!VerQueryValueW(Version, L"\\", (PVOID*)&FileVer, &FileVerSize)) {
        GlobalFree(Version);
        return FALSE;
    }

	DWORD MajorVersion = (FileVer->dwFileVersionMS >> 16) & 0xFFFF,
        MinorVersion = FileVer->dwFileVersionMS & 0xFFFF;  

    GlobalFree(Version);

    if (MajorVersion == 6 && MinorVersion == 5) IsAgent65 = TRUE;

    HANDLE MagentPe = CreateFileW(Input, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (!MagentPe) return FALSE;

	HANDLE FileMap = CreateFileMappingW(MagentPe, NULL, PAGE_READWRITE, 0, 0, NULL);

	SIZE_T PeSize = GetFileSize(MagentPe, 0);

	PBYTE MapBuffer = (PBYTE)MapViewOfFile(FileMap, FILE_MAP_ALL_ACCESS, 0, 0, PeSize);
	
	if (!MapBuffer) return FALSE;

    if (!RegisterFlashClass()) {
        UnmapViewOfFile(MapBuffer);
	    CloseHandle(MagentPe);
	    CloseHandle(FileMap);
        return FALSE;
    } 

    for (DWORD a = 0; a + 16 < PeSize; a++) {
        if (memcmp(&MapBuffer[a], FlashPattern, 16) == 0) {
            memcpy(&MapBuffer[a], FlashPatched, 16);
        }

        if (memcmp(&MapBuffer[a], AttribPattern, 6) == 0) {
            memcpy(&MapBuffer[a + 6], AttribPatched, 3);
        }
    }

    if (IsAgent65) {
        if (NullDerefOffset > PeSize) {
            UnmapViewOfFile(MapBuffer);
	        CloseHandle(MagentPe);
	        CloseHandle(FileMap);
            return FALSE;
        }

        *(PWORD)(&MapBuffer[NullDerefOffset]) = 0x9090;
    }

	UnmapViewOfFile(MapBuffer);
	CloseHandle(MagentPe);
	CloseHandle(FileMap);
    return TRUE;
}

