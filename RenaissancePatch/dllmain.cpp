#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include "include/MinHook.h"
#include <WinSock2.h>

#pragma comment(lib, "ws2_32.lib")

// дефайны для дефолтных значений
#define DEFAULT_DOMAIN "mrim.su"
#define DEFAULT_AVATAR_DOMAIN "obraz.mrim.su"

#if defined _M_IX86
#pragma comment(lib, "MinHook.x86.lib")
#else
#error RenaissancePatch does not support any other architecture except for x86 (Win32)
#endif

char* MrimProtocolDomain;
char* MrimAvatarsDomain;

typedef hostent* (WSAAPI *GETHOSTBYNAME)(const char* name);

// поинтер для оригинального gethostbyname
GETHOSTBYNAME oggethostbyname = NULL;

// вот здесь мы делаем тёмные делишки 🔥
static hostent* WSAAPI hijackedgethostbyname(const char* name) {
    if (strcmp(name, "mrim.mail.ru") == 0) 
        return oggethostbyname(MrimProtocolDomain);

    if (strcmp(name, "obraz.foto.mail.ru") == 0)
        return oggethostbyname(MrimAvatarsDomain);

    return oggethostbyname(name);
}

char* WideToChar(const wchar_t* wideStr) {
    if (!wideStr) return NULL;

    int size = WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, NULL, 0, NULL, NULL);
    if (size == 0) return NULL;

    char* buffer = new char[size];
    WideCharToMultiByte(CP_UTF8, 0, wideStr, -1, buffer, size, NULL, NULL);
    return buffer;
}

__declspec(dllexport) DWORD WINAPI mainHakVzlom();

// я эту функцию специально так назвал, я не нуп
DWORD WINAPI mainHakVzlom() {
    if (MH_Initialize() != MH_OK)
    {
        MessageBoxA(NULL, "Не получилось проинициализировать работу с библиотекой MinHook", "Критическая ошибка Renaissance Patch", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (MH_CreateHookApiEx(L"ws2_32", "gethostbyname", reinterpret_cast<LPVOID>(&hijackedgethostbyname), reinterpret_cast<LPVOID*>(&oggethostbyname), NULL) != MH_OK)
    {
        MessageBoxA(NULL, "Не получилось найти нужную функцию из программы", "Критическая ошибка Renaissance Patch", MB_OK | MB_ICONERROR);
        return 1;
    }

    if (MH_EnableHook(reinterpret_cast<LPVOID>(&gethostbyname)) != MH_OK)
    {
        MessageBoxA(NULL, "Не получилось перехватить функцию из программы", "Критическая ошибка Renaissance Patch", MB_OK | MB_ICONERROR);
        return 1;
    }
    else 
    {
        HKEY hKey = NULL;
        const wchar_t* regPatch = L"SOFTWARE\\Renaissance";
		// получаем доступ к реестру (при отсутствии ключа - создаём его) 
        if (RegCreateKeyW(HKEY_CURRENT_USER, regPatch, &hKey) != ERROR_SUCCESS) {
            MessageBoxA(NULL, "Не удалось получить доступ к Реестру", "Критическая ошибка Renaissance Patch", MB_OK | MB_ICONERROR);
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
                RegSetValueExW(hKey, L"FirstTime", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&FirstTimeTmp), sizeof(FirstTimeTmp));
                RegSetValueExA(hKey, "MrimDomain", 0, REG_SZ, (BYTE*)DEFAULT_DOMAIN, sizeof(DEFAULT_DOMAIN));
                RegSetValueExA(hKey, "MrimAvatarDomain", 0, REG_SZ, (BYTE*)DEFAULT_AVATAR_DOMAIN, sizeof(DEFAULT_AVATAR_DOMAIN)+1);
                MrimProtocolDomain = (char*)DEFAULT_DOMAIN;
                MrimAvatarsDomain = (char*)DEFAULT_AVATAR_DOMAIN;
                // юзера уведомляем
                MessageBoxA(NULL, "Похоже, вы установили инджектор ручным способом. Отредактируйте параметры на соответствующие вашим в Редакторе Реестра по адресу HKCU/SOFTWARE/Renaissance", "Renaissance Patch", MB_OK | MB_ICONINFORMATION);
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
            if (RegQueryValueExA(hKey, "FirstTime", 0, &FTType, (BYTE*)&FirstTime, &FTLen) == ERROR_SUCCESS)
            {
                if (FirstTime == 1) {
                    FirstTime = 0;
                    RegSetValueExW(hKey, L"FirstTime", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&FirstTime), sizeof(FirstTime));
                    MessageBoxA(NULL, "Если вы видите это сообщение - поздравляем, патч сработал!\n\nНастроить его можно с помощью Renaissance Patcher.", "Renaissance Patch", MB_OK | MB_ICONINFORMATION);
                }
            }

            // вычищаем всё нахуй, чтобы не возникало утечек памяти
            RegCloseKey(hKey);
        }
    }
    return 0;
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainHakVzlom, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

