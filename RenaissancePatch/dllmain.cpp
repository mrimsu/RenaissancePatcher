#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <WinSock2.h>
#include <windows.h>

// дефайны для дефолтных значений
#define DEFAULT_DOMAIN "mrim.su"
#define DEFAULT_AVATAR_DOMAIN "obraz.mrim.su"

char* MrimProtocolDomain;
char* MrimAvatarsDomain;

typedef struct hostent *(WSAAPI *_gethostbyname) (const char* name);

_gethostbyname OriginalGethostbyname = NULL;

// вот здесь мы делаем тёмные делишки 🔥
struct hostent * WSAAPI hijackedgethostbyname(const char* name) {
    if (strcmp(name, "mrim.mail.ru") == 0) 
        return OriginalGethostbyname(MrimProtocolDomain);

    if (strcmp(name, "obraz.foto.mail.ru") == 0)
        return OriginalGethostbyname(MrimAvatarsDomain);

    return OriginalGethostbyname(name);
}

char* WideToChar(const wchar_t* wideStr) {
    if (!wideStr) return NULL;

    int size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);
    if (size == 0) return NULL;

    char *buffer = (char *)GlobalAlloc(GMEM_ZEROINIT, size);
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, buffer, size, NULL, NULL);
    return buffer;
}

extern "C" __declspec(dllexport) DWORD __cdecl mainHakVzlom(); 

PVOID EnableTrampoline(PVOID Original, PVOID Detour) {
    CONST SIZE_T Length = 5;

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
DWORD __cdecl mainHakVzlom() {
    HKEY hKey = NULL;
    const wchar_t* regPatch = L"SOFTWARE\\Renaissance";
		// получаем доступ к реестру (при отсутствии ключа - создаём его) 
    if (RegCreateKeyW(HKEY_CURRENT_USER, regPatch, &hKey) != ERROR_SUCCESS) {
        MessageBoxW(NULL, L"Не удалось получить доступ к Реестру", L"Критическая ошибка Renaissance Patch", MB_OK | MB_ICONERROR);
        RegCloseKey(hKey);
    }
    else 
    {
        // буфер для домена протокола
        DWORD dwType = REG_SZ;
        wchar_t buf[255] = { 0 };
        DWORD dwBufSize = sizeof(buf);
        // сначала грузим настройки
        // тут раньше был смешной баг, из-за которого патчер не работал на Windows XP
        if (RegQueryValueExW(hKey, L"MrimDomain", 0, &dwType, (LPBYTE)buf, &dwBufSize) == ERROR_SUCCESS)
        {
            // совершенно не безопасно, но функция в if и так защищает от переполнения буфера (выдаст ошибку, если буфер милипиздрический)
            MrimProtocolDomain = WideToChar(buf);
        }
        else {
            // в ином случае грузим дефолты ну и делаем ДУДОС ЭЛЬДОРАДО
            MrimProtocolDomain = (char*)DEFAULT_DOMAIN;

            // выставляем дефолт
            DWORD FirstTimeTmp = 0;
            RegSetValueExW(hKey, L"FirstTime", 0, REG_DWORD, (BYTE *)&FirstTimeTmp, sizeof(FirstTimeTmp));
            RegSetValueExA(hKey, "MrimDomain", 0, REG_SZ, (BYTE*)DEFAULT_DOMAIN, sizeof(DEFAULT_DOMAIN));
            RegSetValueExA(hKey, "MrimAvatarDomain", 0, REG_SZ, (BYTE*)DEFAULT_AVATAR_DOMAIN, sizeof(DEFAULT_AVATAR_DOMAIN)+1);
            MrimProtocolDomain = (char*)DEFAULT_DOMAIN;
            MrimAvatarsDomain = (char*)DEFAULT_AVATAR_DOMAIN;
            // юзера уведомляем
            MessageBoxW(NULL, L"Похоже, вы установили инджектор ручным способом. Отредактируйте параметры на соответствующие вашим в Редакторе Реестра по адресу HKCU/SOFTWARE/Renaissance", L"Renaissance Patch", MB_OK | MB_ICONINFORMATION);
        }
        memset(buf, 0, sizeof(buf));

        // буфер для домена авок
        wchar_t bufAva[255] = { 0 };
        DWORD dwAvaBufSize = sizeof(buf);
        if (RegQueryValueExW(hKey, L"MrimAvatarDomain", 0, &dwType, (LPBYTE)bufAva, &dwAvaBufSize) == ERROR_SUCCESS)
        {
            MrimAvatarsDomain = WideToChar(bufAva);
        }
        else {
            MrimAvatarsDomain = (char*)DEFAULT_AVATAR_DOMAIN;
            }
        // чистим буфер от гавна
        memset(buf, 0, sizeof(buf));

        // проверяем на первый запуск
        DWORD FirstTime = 0;
        DWORD FTType = REG_DWORD;
        DWORD FTLen = (DWORD)sizeof(FirstTime);
        if (RegQueryValueExW(hKey, L"FirstTime", 0, &FTType, (BYTE*)&FirstTime, &FTLen) == ERROR_SUCCESS)
        {
            if (FirstTime == 1) {
                FirstTime = 0;
                RegSetValueExW(hKey, L"FirstTime", 0, REG_DWORD, (BYTE *)&FirstTime, sizeof(FirstTime));
                MessageBoxW(NULL, L"Если вы видите это сообщение - поздравляем, патч сработал!\n\nНастроить его можно с помощью Renaissance Patcher.", L"Renaissance Patch", MB_OK | MB_ICONINFORMATION);
            }
        }

        // вычищаем всё нахуй, чтобы не возникало утечек памяти
        RegCloseKey(hKey);
    }
    return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    UNREFERENCED_PARAMETER(hModule);
    UNREFERENCED_PARAMETER(lpReserved);
    switch (ul_reason_for_call)
    {
	case DLL_PROCESS_ATTACH: {
    
        HMODULE WinSock2dll = GetModuleHandleW(L"ws2_32.dll");
        FARPROC ModuleFuncOffset = GetProcAddress(WinSock2dll, "gethostbyname");
        OriginalGethostbyname = (_gethostbyname) EnableTrampoline((PVOID)ModuleFuncOffset, (PVOID)hijackedgethostbyname);
        mainHakVzlom();
	}
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

