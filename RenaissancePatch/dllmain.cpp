#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <MsHTML.h>
#include <Exdisp.h>
#include <ExDispid.h>
#include <shellapi.h>

// дефайны для дефолтных значений
#define DEFAULT_DOMAIN "proto.mrim.su"
#define DEFAULT_AVATAR_DOMAIN "obraz.mrim.su"
#define WEBSITE_URL (PWSTR)L"https://mrim.su"

PSTR MrimProtocolDomain = NULL;
PSTR MrimAvatarsDomain = NULL;

typedef struct hostent *(WSAAPI *_gethostbyname) (const char* name);
typedef HWND (WINAPI *_CreateWindowExW) (DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

_gethostbyname OriginalGethostbyname = NULL;
_CreateWindowExW OriginalCreateWindowExW = NULL;

// вот здесь мы делаем тёмные делишки 🔥
struct hostent * WSAAPI DetourGethostbyname(const char* name) {
    if (strcmp(name, "mrim.mail.ru") == 0) 
        return OriginalGethostbyname(MrimProtocolDomain);

    if (strcmp(name, "obraz.foto.mail.ru") == 0)
        return OriginalGethostbyname(MrimAvatarsDomain);

    return OriginalGethostbyname(name);
}

HWND WINAPI DetourCreateWindowExW(DWORD dwExStyle, LPCWSTR lpClassName, LPCWSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam) {
    if (lpClassName != NULL) {
        if (_wcsicmp(lpWindowName, L"IE2") == 0) {
            ShellExecuteW(NULL, L"open", WEBSITE_URL, NULL, NULL, SW_SHOW);
            return NULL;
        }
    }
    return OriginalCreateWindowExW(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
}

PSTR WideToChar(PCWSTR WideStr) {
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
            RegSetValueExA(hKey, "MrimDomain", 0, REG_SZ, (PBYTE)DEFAULT_DOMAIN, sizeof(DEFAULT_DOMAIN));
            RegSetValueExA(hKey, "MrimAvatarDomain", 0, REG_SZ, (PBYTE)DEFAULT_AVATAR_DOMAIN, sizeof(DEFAULT_AVATAR_DOMAIN)+1);
            MrimProtocolDomain = (PSTR)DEFAULT_DOMAIN;
            MrimAvatarsDomain = (PSTR)DEFAULT_AVATAR_DOMAIN;
            // юзера уведомляем
            MessageBoxW(NULL, L"Похоже, вы установили патч ручным способом. Отредактируйте параметры на соответствующие вашим в Редакторе Реестра по адресу HKCU/SOFTWARE/Renaissance", L"Renaissance Patch", MB_OK | MB_ICONINFORMATION);
        }

        memset(Buf, 0, sizeof(Buf));

        // буфер для домена авок
        WCHAR bufAva[255] = { 0 };
        DWORD dwAvaBufSize = sizeof(Buf);
        if (RegQueryValueExW(hKey, L"MrimAvatarDomain", 0, &dwType, (LPBYTE)bufAva, &dwAvaBufSize) == ERROR_SUCCESS) {
            MrimAvatarsDomain = WideToChar(bufAva);
        }

        else {
            MrimAvatarsDomain = (PSTR)DEFAULT_AVATAR_DOMAIN;
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

BOOL APIENTRY DllMain(HMODULE hModule,
                        DWORD ul_reason_for_call,
                        LPVOID lpReserved
                    )
    {
        UNREFERENCED_PARAMETER(hModule);
        UNREFERENCED_PARAMETER(lpReserved);

        switch (ul_reason_for_call) {
	        case DLL_PROCESS_ATTACH: {

                if (!CheckWinSock()) {
                    MessageBoxW(NULL, L"Данная программа не загрузила WinSock2. Вероятно был пропатчен не тот exe файл", L"Ошибка", MB_OK | MB_ICONERROR);
                    ExitProcess(1);
                }

                HMODULE WinSock2dll = GetModuleHandleW(L"ws2_32.dll"),
                    User32Dll = GetModuleHandleW(L"User32.dll");

                FARPROC GetHostbynameOffset = GetProcAddress(WinSock2dll, "gethostbyname"),
                    CreateWindowExWOffset = GetProcAddress(User32Dll, "CreateWindowExW");

                if (!GetHostbynameOffset) {
                    MessageBoxW(NULL, L"В этой имплементации WinSock2 отсутствует функция gethostbyname", L"Ошибка", MB_OK | MB_ICONERROR);
                    ExitProcess(1);
                }

                OriginalGethostbyname = (_gethostbyname) EnableTrampoline((PVOID)GetHostbynameOffset, (PVOID)DetourGethostbyname, 5);
                OriginalCreateWindowExW = (_CreateWindowExW) EnableTrampoline((PVOID)CreateWindowExWOffset, (PVOID)DetourCreateWindowExW, 5);

                MainHakVzlom();
                break;
	        }
            case DLL_THREAD_ATTACH:
            case DLL_THREAD_DETACH:
            case DLL_PROCESS_DETACH:
                break;
        }
        return TRUE;
}
