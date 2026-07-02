#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <shellapi.h>
//#include <winternl.h>
/*
typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
    PWSTR Buffer;
} UNICODE_STRING;

typedef struct _PEB_LDR_DATA {
    BYTE Reserved1[8];
    PVOID Reserved2[3];
    LIST_ENTRY InMemoryOrderModuleList;
} PEB_LDR_DATA,*PPEB_LDR_DATA;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
    BYTE Reserved1[16];
    PVOID Reserved2[10];
    UNICODE_STRING ImagePathName;
    UNICODE_STRING CommandLine;
} RTL_USER_PROCESS_PARAMETERS,*PRTL_USER_PROCESS_PARAMETERS;
  
typedef VOID (NTAPI *PPS_POST_PROCESS_INIT_ROUTINE)(VOID);
  
typedef struct _PEB {
    BYTE Reserved1[2];
    BYTE BeingDebugged;
    BYTE Reserved2[1];
    PVOID Reserved3[2];
    PPEB_LDR_DATA Ldr;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID Reserved4[3];
    PVOID AtlThunkSListPtr;
    PVOID Reserved5;
    ULONG Reserved6;
    PVOID Reserved7;
    ULONG Reserved8;
    ULONG AtlThunkSListPtr32;
    PVOID Reserved9[45];
    BYTE Reserved10[96];
    PPS_POST_PROCESS_INIT_ROUTINE PostProcessInitRoutine;
    BYTE Reserved11[128];
    PVOID Reserved12[1];
    ULONG SessionId;
} PEB, *PPEB;

*/
// дефайны для дефолтных значений
#define DEFAULT_DOMAIN "proto.mrim.su"
#define DEFAULT_AVATAR_DOMAIN "obraz.mrim.su"
#define DEFAULT_SERVICES_DOMAIN "obraz.mrim.su"
#define WEBSITE_REGISTER_URL (PWSTR)L"http://mrim.su/reg"

#define BACKDOOR_WINDOW_STRING (PWSTR)L"fuck fuck"

PSTR MrimProtocolDomain = NULL,
    MrimAvatarsDomain = NULL,
    MrimServicesDomain = NULL;

PWSTR WebRegisterUrl = NULL;

typedef struct hostent *(WSAAPI *_gethostbyname) (const char *name);
typedef BOOL (WINAPI *_ShowWindow) (HWND hWnd, int nCmdShow);
typedef BOOL (WINAPI *_CreatePipe) (PHANDLE hReadPipe, PHANDLE hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize);
typedef BOOL (WINAPI *_CreateProcessW) (
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation
    );

_gethostbyname OriginalGethostbyname = NULL;
_ShowWindow OriginalShowWindow = NULL;
_CreatePipe OriginalCreatePipe = NULL;
_CreateProcessW OriginalCreateProcessW = NULL;

/* 
this is sorta a fix for mail.ru's reverse shell backdoor which is present in version 5.7 to 6.5 
it just prevents CreatePipe from being ran (beacuse this only had an xref in that one function responsible for it)
and it hooks CreateProcessW to check if it creates a cmd prompt with *the funny* title
*/
BOOL WINAPI DetourCreatePipe(PHANDLE hReadPipe, PHANDLE hWritePipe, LPSECURITY_ATTRIBUTES lpPipeAttributes, DWORD nSize) {
    UNREFERENCED_PARAMETER(hReadPipe);
    UNREFERENCED_PARAMETER(hWritePipe);
    UNREFERENCED_PARAMETER(lpPipeAttributes);
    (void)nSize;
    return FALSE;
}

BOOL WINAPI DetourCreateProcessW (
    _In_opt_ LPCWSTR lpApplicationName,
    _Inout_opt_ LPWSTR lpCommandLine,
    _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ BOOL bInheritHandles,
    _In_ DWORD dwCreationFlags,
    _In_opt_ LPVOID lpEnvironment,
    _In_opt_ LPCWSTR lpCurrentDirectory,
    _In_ LPSTARTUPINFOW lpStartupInfo,
    _Out_ LPPROCESS_INFORMATION lpProcessInformation
    ) {
        if (lpStartupInfo != NULL)
            if ((wcscmp(lpApplicationName, L"C:\\windows\\system32\\cmd.exe") == 0) && (wcscmp(lpStartupInfo->lpTitle, BACKDOOR_WINDOW_STRING) == 0)) return FALSE;
        return OriginalCreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
}

// вот здесь мы делаем тёмные делишки 🔥
struct hostent * WSAAPI DetourGethostbyname(const char* name) {
    if (strcmp(name, "mrim.mail.ru") == 0) 
        return OriginalGethostbyname(MrimProtocolDomain);

    if (strcmp(name, "obraz.foto.mail.ru") == 0)
        return OriginalGethostbyname(MrimAvatarsDomain);

    if (strcmp(name, "pogoda.mail.ru") == 0)
        return OriginalGethostbyname(MrimServicesDomain);

    if (strcmp(name, "weather.agent.mail.ru") == 0)
        return OriginalGethostbyname(MrimServicesDomain);

    if (strcmp(name, "my.agent.mail.ru") == 0)
        return OriginalGethostbyname(MrimServicesDomain);

    if (strcmp(name, "agent.mail.ru") == 0)
        return OriginalGethostbyname(MrimServicesDomain);

    return OriginalGethostbyname(name);
}

BOOL WINAPI DetourShowWindow(HWND hWnd, int nCmdShow) {
    WCHAR ClassName[256];
    if (GetClassNameW(hWnd, ClassName, 256)) {

        if (_wcsicmp(ClassName, L"MAgentIE2") == 0) {
            DestroyWindow(hWnd);
            ShellExecuteW(NULL, L"open", WebRegisterUrl, NULL, NULL, SW_SHOW);
            return FALSE;
        }
    }
    return OriginalShowWindow(hWnd, nCmdShow);
}

PSTR WINAPI WideToChar(PCWSTR WideStr) {
    if (!WideStr) return NULL;

    INT size = WideCharToMultiByte(CP_UTF8, 0, WideStr, -1, NULL, 0, NULL, NULL);
    if (size == 0) return NULL;

    PSTR Buffer = (PSTR)GlobalAlloc(GMEM_ZEROINIT, size);
    WideCharToMultiByte(CP_UTF8, 0, WideStr, -1, Buffer, size, NULL, NULL);
    return Buffer;
}

extern "C" __declspec(dllexport) DWORD __cdecl MainHakVzlom(); 

PVOID WINAPI EnableTrampoline(PVOID Original, PVOID Detour, SIZE_T Length) {
    PVOID OldFuncPointer = VirtualAlloc(NULL, Length + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);

    CopyMemory(OldFuncPointer, Original, Length);
    *(PBYTE)((PBYTE)OldFuncPointer + Length) = 0xE9;

    DWORD OrigAddr = (DWORD)Original + 5,
        JmpTrampoline = (DWORD)OldFuncPointer + Length + 5,
        Offset = OrigAddr - JmpTrampoline;

    *(PDWORD)((PBYTE)OldFuncPointer + Length + 1) = Offset;
    
    DWORD OldProt;
    VirtualProtect(Original, Length, PAGE_EXECUTE_READWRITE, &OldProt);
    
    DWORD Addr = ((DWORD)Detour - (DWORD)Original) - 5;
    *(PBYTE)Original = 0xE9;
    *(PDWORD)((PBYTE)Original + 1) = Addr;

    VirtualProtect(Original, Length, OldProt, &OldProt);

    return OldFuncPointer;
}

// я эту функцию специально так назвал, я не нуп
DWORD __cdecl MainHakVzlom(VOID) {
    HKEY hKey = NULL;
    PCWSTR regPatch = L"SOFTWARE\\Renaissance";
		// получаем доступ к реестру (при отсутствии ключа - создаём его) 
    if (RegCreateKeyW(HKEY_CURRENT_USER, regPatch, &hKey) != ERROR_SUCCESS) {
        MessageBoxW(NULL, L"Не удалось получить доступ к Реестру", L"Критическая ошибка Renaissance Patch", MB_OK | MB_ICONERROR);
        RegCloseKey(hKey);
    }

    else {
        // буфер для домена протокола
        DWORD dwType = REG_SZ;
        WCHAR Buf[255] = { 0 };
        DWORD dwBufSize = sizeof(Buf);
        // сначала грузим настройки
        // тут раньше был смешной баг, из-за которого патчер не работал на Windows XP
        if (RegQueryValueExW(hKey, L"MrimDomain", 0, &dwType, (LPBYTE)Buf, &dwBufSize) == ERROR_SUCCESS)
        {
            // совершенно не безопасно, но функция в if и так защищает от переполнения буфера (выдаст ошибку, если буфер милипиздрический)
            MrimProtocolDomain = WideToChar(Buf);
        }
        else {
            // в ином случае грузим дефолты ну и делаем ДУДОС ЭЛЬДОРАДО
            MrimProtocolDomain = (PSTR) DEFAULT_DOMAIN;

            // выставляем дефолт
            DWORD FirstTimeTmp = 0;
            RegSetValueExW(hKey, L"FirstTime", 0, REG_DWORD, (PBYTE)&FirstTimeTmp, sizeof(FirstTimeTmp));
            RegSetValueExA(hKey, "MrimDomain", 0, REG_SZ, (PBYTE)DEFAULT_DOMAIN, strlen(DEFAULT_DOMAIN));
            RegSetValueExA(hKey, "MrimAvatarDomain", 0, REG_SZ, (PBYTE)DEFAULT_AVATAR_DOMAIN, strlen(DEFAULT_AVATAR_DOMAIN)+1);
            RegSetValueExA(hKey, "MrimServices", 0, REG_SZ, (PBYTE)DEFAULT_SERVICES_DOMAIN, strlen(DEFAULT_SERVICES_DOMAIN)+1);
            RegSetValueExW(hKey, L"WebRegisterUrl", 0, REG_SZ, (PBYTE)WEBSITE_REGISTER_URL, wcslen(WEBSITE_REGISTER_URL) * sizeof(WCHAR) + 1);
            MrimProtocolDomain = (PSTR)DEFAULT_DOMAIN;
            MrimAvatarsDomain = (PSTR)DEFAULT_AVATAR_DOMAIN;
            MrimServicesDomain = (PSTR)DEFAULT_SERVICES_DOMAIN;
            WebRegisterUrl = (PWSTR)WEBSITE_REGISTER_URL;
            // юзера уведомляем
            MessageBoxW(NULL, L"Похоже, вы установили патч ручным способом. Отредактируйте параметры на соответствующие вашим в Редакторе Реестра по адресу HKCU/SOFTWARE/Renaissance", L"Renaissance Patch", MB_OK | MB_ICONINFORMATION);
        }

        memset(Buf, 0, sizeof(Buf));

        // буфер для домена авок
        WCHAR bufAva[255] = { 0 };
        DWORD dwAvaBufSize = sizeof(bufAva);
        if (RegQueryValueExW(hKey, L"MrimAvatarDomain", 0, &dwType, (LPBYTE)bufAva, &dwAvaBufSize) == ERROR_SUCCESS) {
            MrimAvatarsDomain = WideToChar(bufAva);
        }

        else {
            MrimAvatarsDomain = (PSTR)DEFAULT_AVATAR_DOMAIN;
        }

        WCHAR bufServices[255] = { 0 };
        DWORD dwServiceBufSize = sizeof(bufServices);
        if (RegQueryValueExW(hKey, L"MrimServices", 0, &dwType, (LPBYTE)bufServices, &dwServiceBufSize) == ERROR_SUCCESS) {
            MrimServicesDomain = WideToChar(bufServices);
        }

        else {
            MrimServicesDomain = (PSTR)DEFAULT_SERVICES_DOMAIN;
        }

        WCHAR bufRegister[255] = { 0 };
        DWORD dwRegisterBufSize = sizeof(bufRegister) ;
        if (RegQueryValueExW(hKey, L"WebRegisterUrl", 0, &dwType, (LPBYTE)bufRegister, &dwRegisterBufSize) == ERROR_SUCCESS) {
            WebRegisterUrl = (PWSTR)GlobalAlloc(GMEM_ZEROINIT, wcslen(bufRegister) * sizeof(WCHAR) + 1);
            wcscpy(WebRegisterUrl, bufRegister);
        }

        else {
            WebRegisterUrl = (PWSTR)WEBSITE_REGISTER_URL;
        }

        // чистим буфер от гавна
        memset(Buf, 0, sizeof(Buf));

        // проверяем на первый запуск
        DWORD FirstTime = 0;
        DWORD FTType = REG_DWORD;
        DWORD FTLen = (DWORD)sizeof(FirstTime);
        if (RegQueryValueExW(hKey, L"FirstTime", 0, &FTType, (PBYTE)&FirstTime, &FTLen) == ERROR_SUCCESS)
        {
            if (FirstTime == 1) {
                FirstTime = 0;
                RegSetValueExW(hKey, L"FirstTime", 0, REG_DWORD, (PBYTE)&FirstTime, sizeof(FirstTime));
                MessageBoxW(NULL, L"Если вы видите это сообщение - поздравляем, патч сработал!\n\nНастроить его можно с помощью Renaissance Patcher.", L"Renaissance Patch", MB_OK | MB_ICONINFORMATION);
            }
        }
        // вычищаем всё нахуй, чтобы не возникало утечек памяти
        RegCloseKey(hKey);
    }
    return 0;
}

//fool-proof 
BOOL WINAPI CheckWinSock(VOID) {
    HANDLE ProcSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    MODULEENTRY32W ModuleEntry;
    ModuleEntry.dwSize = sizeof(MODULEENTRY32);

    if (!Module32FirstW(ProcSnapshot, &ModuleEntry)) {
        CloseHandle(ProcSnapshot);
        return FALSE;
    }

    do {
        if (_wcsicmp(ModuleEntry.szModule, L"ws2_32.dll") == 0) {
            CloseHandle(ProcSnapshot);
            return TRUE;
        }
    } while (Module32Next(ProcSnapshot, &ModuleEntry));

    CloseHandle(ProcSnapshot);
    return FALSE;
}

PVOID HookIatFunc(PWSTR LibraryName, PSTR HookFunctionName, PVOID DetourFunction) {
    PVOID ImageBase = GetModuleHandleW(NULL),
        TargetFunctionAddress = (PVOID)GetProcAddress(GetModuleHandleW(LibraryName), HookFunctionName);

    if (!ImageBase || !TargetFunctionAddress) return NULL;

	PIMAGE_DOS_HEADER DosHeaders = (PIMAGE_DOS_HEADER)ImageBase;
	PIMAGE_NT_HEADERS NtHeaders = (PIMAGE_NT_HEADERS)((PBYTE)ImageBase + DosHeaders->e_lfanew);
    IMAGE_DATA_DIRECTORY ImportsDirectory = NtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)ImageBase + ImportsDirectory.VirtualAddress);
    
    for(; ImportDescriptor->Name; ImportDescriptor++) {
        DWORD ThunkData = ImportDescriptor->OriginalFirstThunk ? ImportDescriptor->OriginalFirstThunk : ImportDescriptor->FirstThunk;
        PIMAGE_THUNK_DATA OriginalThunk = (PIMAGE_THUNK_DATA)((PBYTE)ImageBase + ThunkData);
        PIMAGE_THUNK_DATA FirstThunk = (PIMAGE_THUNK_DATA)((PBYTE)ImageBase + ImportDescriptor->FirstThunk);
        
        for(; OriginalThunk->u1.AddressOfData; OriginalThunk++, FirstThunk++) {
            if (FirstThunk->u1.Function == (DWORD)TargetFunctionAddress) {
                DWORD OldProtect = 0;

				VirtualProtect((PVOID)(&FirstThunk->u1.Function), sizeof(PVOID), PAGE_READWRITE, &OldProtect);

                PVOID OriginalFunction = (PVOID)FirstThunk->u1.Function;
				FirstThunk->u1.Function = (DWORD)DetourFunction;

                VirtualProtect((PVOID)(&FirstThunk->u1.Function), sizeof(PVOID), OldProtect, &OldProtect);

                FlushInstructionCache(GetCurrentProcess(), NULL, 0);
                return OriginalFunction;
            }
        }
    }
    return NULL;
}

VOID NTAPI InitHook(VOID) {
    //((PPEB)__readfsdword(0x30))->PostProcessInitRoutine = NULL;

    if (!CheckWinSock()) {
        MessageBoxW(NULL, L"Данная программа не загрузила WinSock2. Вероятно был пропатчен не тот exe файл", L"Ошибка", MB_OK | MB_ICONERROR);
        ExitProcess(1);
    }

    OriginalGethostbyname = (_gethostbyname)HookIatFunc((PWSTR)L"ws2_32.dll", (PSTR)"gethostbyname", (PVOID)DetourGethostbyname);
    OriginalShowWindow = (_ShowWindow)HookIatFunc((PWSTR)L"user32.dll", (PSTR)"ShowWindow", (PVOID)DetourShowWindow);
    OriginalCreatePipe = (_CreatePipe)HookIatFunc((PWSTR)L"kernel32.dll", (PSTR)"CreatePipe", (PVOID)DetourCreatePipe);
    OriginalCreateProcessW = (_CreateProcessW)HookIatFunc((PWSTR)L"kernel32.dll", (PSTR)"CreateProcessW", (PVOID)DetourCreateProcessW);

    MainHakVzlom();
}

BOOL APIENTRY DllMain(HMODULE hModule,
                        DWORD ul_reason_for_call,
                        LPVOID lpReserved
                    )
    {
        UNREFERENCED_PARAMETER(lpReserved);

        switch (ul_reason_for_call) {
	        case DLL_PROCESS_ATTACH: {
                //NtCurrentTeb()->ProcessEnvironmentBlock->PostProcessInitRoutine = InitHook;
                //((PPEB)__readfsdword(0x30))->PostProcessInitRoutine = InitHook;

                InitHook();

                DisableThreadLibraryCalls(hModule);
                break;
	        }
            case DLL_THREAD_ATTACH:
            case DLL_THREAD_DETACH:
            case DLL_PROCESS_DETACH:
                break;
        }
        return TRUE;
}
