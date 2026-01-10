#include "main.h"
#include "peutils.h"
#include "flashpatch.h"
#include "restools.h"
#include "resources.h"

#define PATCH_BUTTON 101
#define EXTRAS_BUTTON 102
#define APPLY_BUTTON 103
#define SSL_CHECKBOX 104
#define MRA_CLEAN_BUTTON 105
#define REN_CLEAN_BUTTON 106
#define SERVER_TEXTBOX 107
#define AVATAR_SERVER_TEXTBOX 108

typedef struct _ControlHandles {
	HWND SslCheckbox;
	HWND ServerEditBox;
	HWND AvatarServerEditBox;
} ControlHandles;

ControlHandles ExtraDialogControls;

BOOL WINAPI GetRegistryValues(VOID);
BOOL WINAPI SetRegistryValues(BOOL IsRanFromExtras);
BOOL WINAPI FileExists(PWSTR FilePath);
VOID WINAPI KillRunningProcess(PWSTR FilePath);
BOOL WINAPI MainRoutine(HWND Hwnd, PWSTR FilePath);
LRESULT CALLBACK MainWindowProc(HWND Hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ExtrasWindowProc(HWND Hwnd, UINT Msg, WPARAM wParam, LPARAM lParam);

HWND MainWindow = NULL; // workaround only needed for enabling/disabling the main window 

PCWSTR MainClassName = L"Renaissance001", //source engine nerds should guess the reference :D
	ExtrasClassName = L"Renaissance002"; 

INT WINAPI WinMain(HINSTANCE Instance, HINSTANCE OldInst, LPSTR CmdLine, INT CmdShow) {
	UNREFERENCED_PARAMETER(OldInst);
	UNREFERENCED_PARAMETER(CmdLine);

	//this part is intended for silent install (online installer etc)
	INT Argc;
	PWSTR *Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
	
	if (Argc > 1) {
		if (!MainRoutine(NULL, Argv[1])) return 1;
		return 0;
	}

    WNDCLASSW MainWndClass;
	ZeroMemory(&MainWndClass, sizeof(WNDCLASSW));

    MainWndClass.hInstance = Instance;
    MainWndClass.hCursor = LoadCursorW(Instance, IDC_ARROW);
    MainWndClass.hbrBackground = CreatePatternBrush((HBITMAP)LoadImageW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION));
    MainWndClass.hIcon = LoadIconW(Instance, MAKEINTRESOURCEW(IDI_ICON1));
    MainWndClass.lpszClassName = MainClassName;
    MainWndClass.lpfnWndProc = MainWindowProc;
    MainWndClass.style = 0;

    RegisterClass(&MainWndClass);

	WNDCLASSW ExtrasWndClass;
	ZeroMemory(&ExtrasWndClass, sizeof(WNDCLASSW));

    ExtrasWndClass.hInstance = Instance;
    ExtrasWndClass.hCursor = LoadCursorW(Instance, IDC_ARROW);
    ExtrasWndClass.hbrBackground = (HBRUSH)GetSysColorBrush(COLOR_3DFACE);
    ExtrasWndClass.hIcon = LoadIconW(Instance, MAKEINTRESOURCEW(IDI_ICON1));
    ExtrasWndClass.lpszClassName = ExtrasClassName;
    ExtrasWndClass.lpfnWndProc = ExtrasWindowProc;
    ExtrasWndClass.style = 0;

    RegisterClass(&ExtrasWndClass);

    MainWindow = CreateWindowW(MainClassName, L"Renaissance Patcher", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 800, 230, NULL, NULL, Instance, NULL);
	
    ShowWindow(MainWindow, CmdShow);
    UpdateWindow(MainWindow);

    MSG Msg;

    while (GetMessage(&Msg, NULL, 0, 0)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return 0;
}

BOOL WINAPI GetRegistryValues(VOID) {
	HKEY Hkey;
	if (RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Mail.ru\\Agent", &Hkey) != ERROR_SUCCESS)
		return FALSE;

	PWSTR SslData = NULL;
	DWORD SslDataSize = 0,
		SslDataType = 0;
	
	if (RegQueryValueExW(Hkey, L"ssl", 0, &SslDataType, (PBYTE)SslData, &SslDataSize) == ERROR_SUCCESS) {
		if (SslDataType == REG_SZ) {
			SslData = (PWSTR)GlobalAlloc(GMEM_ZEROINIT, SslDataSize * sizeof(WCHAR));

			if (!SslData) {
				RegCloseKey(Hkey);
				return FALSE;
			}

			RegQueryValueExW(Hkey, L"ssl", 0, &SslDataType, (PBYTE)SslData, &SslDataSize);

			if (_wcsicmp((SslData), L"deny") != 0)
				SendMessageW(ExtraDialogControls.SslCheckbox, BM_SETCHECK, BST_CHECKED, 0);
			GlobalFree(SslData);
		}
	}

	else 
		SendMessageW(ExtraDialogControls.SslCheckbox, BM_SETCHECK, BST_CHECKED, 0);

	RegCloseKey(Hkey);

	if (RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Renaissance", &Hkey) != ERROR_SUCCESS)
		return FALSE;

	PWSTR ServerAddrData = NULL;
	DWORD ServerAddrSize = 0,
		ServerAddrType = 0;
	
	if (RegQueryValueExW(Hkey, L"MrimDomain", 0, &ServerAddrType, (PBYTE)ServerAddrData, &ServerAddrSize) == ERROR_SUCCESS) {
		if (ServerAddrType == REG_SZ) {
			ServerAddrData = (PWSTR)GlobalAlloc(GMEM_ZEROINIT, ServerAddrSize * sizeof(WCHAR));

			if (!ServerAddrData) {
				RegCloseKey(Hkey);
				return FALSE;
			}

			RegQueryValueExW(Hkey, L"MrimDomain", 0, &ServerAddrType, (PBYTE)ServerAddrData, &ServerAddrSize);
			SendMessageW(ExtraDialogControls.ServerEditBox, WM_SETTEXT, NULL, (LPARAM)ServerAddrData);

			GlobalFree(ServerAddrData);
		}
	}

	else 
		SendMessageW(ExtraDialogControls.ServerEditBox, WM_SETTEXT, NULL, (LPARAM)DEFAULT_SERVER);

	RegCloseKey(Hkey);

	if (RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Renaissance", &Hkey) != ERROR_SUCCESS)
		return FALSE;

	PWSTR AvatarServerAddrData = NULL;
	DWORD AvatarServerAddrSize = 0,
		AvatarServerAddrType = 0;
	
	if (RegQueryValueExW(Hkey, L"MrimAvatarDomain", 0, &AvatarServerAddrType, (PBYTE)AvatarServerAddrData, &AvatarServerAddrSize) == ERROR_SUCCESS) {
		if (AvatarServerAddrType == REG_SZ) {
			AvatarServerAddrData = (PWSTR)GlobalAlloc(GMEM_ZEROINIT, AvatarServerAddrSize * sizeof(WCHAR));

			if (!AvatarServerAddrData) {
				RegCloseKey(Hkey);
				return FALSE;
			}

			RegQueryValueExW(Hkey, L"MrimAvatarDomain", 0, &AvatarServerAddrType, (PBYTE)AvatarServerAddrData, &AvatarServerAddrSize);
			SendMessageW(ExtraDialogControls.AvatarServerEditBox, WM_SETTEXT, NULL, (LPARAM)AvatarServerAddrData);

			GlobalFree(AvatarServerAddrData);
		}
	}

	else 
		SendMessageW(ExtraDialogControls.AvatarServerEditBox, WM_SETTEXT, NULL, (LPARAM)DEFAULT_AVATAR_SERVER);

	RegCloseKey(Hkey);

	return TRUE;
}

BOOL WINAPI SetRegistryValues(BOOL IsRanFromExtras) {
	HKEY Hkey;

	/*if (RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Mail.ru\\Agent", &Hkey) != ERROR_SUCCESS) return FALSE;
    
    PCWSTR Value = L"proto.mrim.su:2041"; 

    if (RegSetValueExW(Hkey, L"strict_server", 0, REG_SZ, (PBYTE)Value, wcslen(Value) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        RegCloseKey(Hkey);
        return FALSE;
    }
    RegCloseKey(Hkey);*/

	if (IsRanFromExtras) {
		if (RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Renaissance", &Hkey) != ERROR_SUCCESS) return FALSE;

		DWORD FirstTimeTmp = 0;

    	RegSetValueExW(Hkey, L"FirstTime", 0, REG_DWORD, (BYTE *)&FirstTimeTmp, sizeof(FirstTimeTmp));

		WCHAR UserServerAddr[MAX_ADDR_LEN],
			UserAvatarServerAddr[MAX_ADDR_LEN];

		ZeroMemory(UserServerAddr, sizeof(WCHAR) * MAX_ADDR_LEN);
		ZeroMemory(UserAvatarServerAddr, sizeof(WCHAR) * MAX_ADDR_LEN);

		GetWindowTextW(ExtraDialogControls.ServerEditBox, UserServerAddr, MAX_ADDR_LEN);
		GetWindowTextW(ExtraDialogControls.AvatarServerEditBox, UserAvatarServerAddr, MAX_ADDR_LEN);

		UserServerAddr[MAX_ADDR_LEN - 1 ] = '\0';
		UserAvatarServerAddr[MAX_ADDR_LEN - 1] = '\0';

		size_t ServStringLen = wcslen(UserServerAddr),
			AvatarServStringLen = wcslen(UserAvatarServerAddr);

		RegSetValueExW(Hkey, L"MrimDomain", 0, REG_SZ, (BYTE*)UserServerAddr, sizeof(WCHAR) * ServStringLen);
    	RegSetValueExW(Hkey, L"MrimAvatarDomain", 0, REG_SZ, (BYTE*)UserAvatarServerAddr, sizeof(WCHAR) * AvatarServStringLen);

		RegCloseKey(Hkey);

		if (RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Mail.ru\\Agent", &Hkey) != ERROR_SUCCESS) return FALSE;

		if (SendMessageW(ExtraDialogControls.SslCheckbox, BM_GETCHECK, 0, 0) == BST_UNCHECKED) {
			PCWSTR SslValue = L"deny"; 

			if (RegSetValueExW(Hkey, L"ssl", 0, REG_SZ, (PBYTE)SslValue, wcslen(SslValue) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        		RegCloseKey(Hkey);
        		return FALSE;
    		}
		}

		else
			RegDeleteValueW(Hkey, L"ssl");

		RegCloseKey(Hkey);

    	return TRUE;
	}

	if (RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Renaissance", &Hkey) != ERROR_SUCCESS) return FALSE;

	DWORD FirstTimeTmp = 0;

    RegSetValueExW(Hkey, L"FirstTime", 0, REG_DWORD, (BYTE *)&FirstTimeTmp, sizeof(FirstTimeTmp));
    RegSetValueExW(Hkey, L"MrimDomain", 0, REG_SZ, (BYTE*)DEFAULT_SERVER, sizeof(WCHAR) * wcslen(DEFAULT_SERVER));
    RegSetValueExW(Hkey, L"MrimAvatarDomain", 0, REG_SZ, (BYTE*)DEFAULT_AVATAR_SERVER, sizeof(WCHAR) * wcslen(DEFAULT_AVATAR_SERVER));

	RegCloseKey(Hkey);

    return TRUE;
}

VOID WINAPI KillRunningProcess(PWSTR FilePath) {
	PROCESSENTRY32W ProcEntry;
	ProcEntry.dwSize = sizeof(PROCESSENTRY32W);
	HANDLE ProcSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	
	if (!ProcSnapshot) return;

	if (!Process32First(ProcSnapshot, &ProcEntry)) return;

	do {
		if (_wcsicmp(PathFindFileNameW(FilePath), ProcEntry.szExeFile) == 0) {
			HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcEntry.th32ProcessID);

			if (!Process) {
				CloseHandle(Process);
				return;
			}

			TerminateProcess(Process, 0);
			CloseHandle(Process);
		}
	} while (Process32NextW(ProcSnapshot, &ProcEntry));
	CloseHandle(ProcSnapshot);
}

BOOL WINAPI FileExists(PWSTR FilePath) {
	DWORD dwAttrib = GetFileAttributes(FilePath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

BOOL WINAPI MainRoutine(HWND Hwnd, PWSTR FilePath) {
	if (!FileExists(FilePath)) return FALSE;

	KillRunningProcess(FilePath);

	Sleep(900);

	WCHAR OrigPath[MAX_PATH];

	ZeroMemory(OrigPath, sizeof(WCHAR) * MAX_PATH);

	GetFullPathNameW(FilePath, MAX_PATH, OrigPath, NULL);
	PathRemoveFileSpecW(OrigPath);
	
	WCHAR DestPath[MAX_PATH];
	ZeroMemory(DestPath, sizeof(WCHAR) * MAX_PATH);

	wcscpy(DestPath, OrigPath);

	WCHAR BakPath[MAX_PATH];
	ZeroMemory(BakPath, sizeof(WCHAR) * MAX_PATH);

	wcscpy(BakPath, OrigPath);

	wcscat(BakPath, L"\\magent.bak");

	CopyFileW(FilePath, BakPath, FALSE);
	PWSTR TmpFilePath = wcscat(OrigPath, L"\\magent.exe.tmp");

	if (PatchPE(FilePath, TmpFilePath)) {
		BOOL FlashStatus = PatchFlash(TmpFilePath);
							
		DeleteFileW(FilePath);
		MoveFileW(TmpFilePath, FilePath);
		DeleteFileW(TmpFilePath);

		CONST DWORD ResIds[2] = {
			IDR_RCDATA1, 
			IDR_RCDATA2
		};

		PCWSTR DllNames[2] = {
			L"\\flashdll.dll",
			L"\\RenaissancePatch.dll"
		};

		for (DWORD a = 0; a < 2; a++)
			ExtractRes(DestPath, DllNames[a], ResIds[a], RT_RCDATA);

		if (!SetRegistryValues(FALSE))
			MessageBoxW(
				Hwnd, 
				L"Не удалось задать значение HKCU\\Software\\Mail.ru\\Agent | strict_server. Необходимо ввести значение proto.mrim.su:2041", 
				L"Возможная проблема", 
				MB_OK | MB_ICONEXCLAMATION
			);

		if (Hwnd != NULL) {
			if (FlashStatus == TRUE)
				MessageBoxW(
					Hwnd, 
					L"Патчи для Renaissance и работы Flash-мультов были применены успешно.", 
					L"Успех", 
					MB_OK | MB_ICONINFORMATION);

			else
				MessageBoxW(Hwnd, 
					L"Патч для Renaissance был применён успешно.", 
					L"Успех", 
					MB_OK | MB_ICONINFORMATION);	
			}	

		}

		else {
			MessageBoxW(
				Hwnd, 
				L"Не удалось пропатчить клиент. Возможно выбранный файл уже пропатчен, предназначен для архитектуры отличной от I386 или же вовсе не является файлом формата PE.", 
				L"Ошибка при применении патча", 
				MB_OK | MB_ICONERROR
			);
			return FALSE;
		}
	return TRUE;
}

LRESULT CALLBACK MainWindowProc(HWND Hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
        case WM_DESTROY: {
            PostQuitMessage(WM_QUIT);
            break;
        }

		case WM_CREATE: {
			HWND PatchButton = CreateWindowW(L"BUTTON", L"Пропатчить", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 680, 161, 100, 30, Hwnd, (HMENU)PATCH_BUTTON, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL),
				WorkaroundsButton = CreateWindowW(L"BUTTON", L"Дополнительно", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 570, 161, 100, 30, Hwnd, (HMENU)EXTRAS_BUTTON, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL);

			SendMessage(PatchButton, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
			SendMessage(WorkaroundsButton, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
			break;
		}

		case WM_PAINT: {
			PAINTSTRUCT Ps;
			HDC DC = BeginPaint(Hwnd, &Ps);

			SetBkMode(DC, TRANSPARENT);

			HFONT BodyFont = CreateFontW(
				-MulDiv(9, GetDeviceCaps(DC, LOGPIXELSY), 72),
				0,
				0,
				0,
				FW_NORMAL,
				FALSE,
				FALSE,
				FALSE,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_CHARACTER_PRECIS,
				CLEARTYPE_QUALITY,
				DEFAULT_PITCH,
				L"Verdana" // ΜΛΜΛ СМΘΤΡΝ Я ПΝШY КΛΚ МΛNКРΘСΘФТ
			 );
			
			PCWSTR Body = L"Выберите вашу копию magent.exe для установки патча сервера Renaissance";

			SelectObject(DC, BodyFont);
			TextOutW(DC, 8, 126, Body, wcslen(Body));

			EndPaint(Hwnd, &Ps);
			DeleteObject(BodyFont);
			break;
		}

		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case PATCH_BUTTON: {
					WCHAR FilePath[MAX_PATH];
					ZeroMemory(FilePath, sizeof(WCHAR) * MAX_PATH);

					OPENFILENAMEW OpenFileName;
					ZeroMemory(&OpenFileName, sizeof(OPENFILENAMEW));
					OpenFileName.hwndOwner = Hwnd;
					OpenFileName.lpstrFilter = L"Агент (magent.exe)\0magent.exe;\0Все файлы (*.*)\0*.*\0\0";
					OpenFileName.lStructSize = sizeof(OPENFILENAMEW);
					OpenFileName.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
					OpenFileName.lpstrTitle = L"Выберите файл\0";
					OpenFileName.nMaxFile = MAX_PATH;
					OpenFileName.lpstrFile = FilePath;

					if (GetOpenFileNameW(&OpenFileName)) {
						MainRoutine(Hwnd, OpenFileName.lpstrFile);
					}
					break;
				}

				case EXTRAS_BUTTON: {
					HWND ExtraWnd = CreateWindowW(ExtrasClassName, L"Дополнительно", WS_OVERLAPPED | WS_VISIBLE | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 200, 240, Hwnd, NULL, GetModuleHandleW(NULL), NULL);
					EnableWindow(Hwnd, FALSE);
    				UpdateWindow(ExtraWnd);
					break;
				}
			}
			break;
		}

        default: {
            return DefWindowProc(Hwnd, Msg, wParam, lParam);
        }
    }
    return 0;
}

LRESULT CALLBACK ExtrasWindowProc(HWND Hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
		case WM_CREATE: {
			HWND ApplyButton = CreateWindowW(L"BUTTON", L"Применить", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 50, 180, 95, 20, Hwnd, (HMENU)APPLY_BUTTON, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL),
				CleanMraButton = CreateWindowW(L"BUTTON", L"Очистить", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 130, 20, 55, 20, Hwnd, (HMENU)MRA_CLEAN_BUTTON, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL),
				CleanRenButton = CreateWindowW(L"BUTTON", L"Очистить", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 130, 50, 55, 20, Hwnd, (HMENU)REN_CLEAN_BUTTON, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL);
			
			HWND CleanMraCaption = CreateWindowW(L"STATIC", L"Очистить Agent", WS_VISIBLE | WS_CHILD, 10, 20, 60, 30, Hwnd, NULL, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL),
				CleanRenCaption = CreateWindowW(L"STATIC", L"Очистить Renaissance", WS_VISIBLE | WS_CHILD, 10, 50, 65, 30, Hwnd, NULL, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL);
			
			ExtraDialogControls.SslCheckbox = CreateWindowW(L"BUTTON", L"Включить SSL", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 10, 78, 90, 20, Hwnd, (HMENU)SSL_CHECKBOX, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL);

			ExtraDialogControls.ServerEditBox = CreateWindowW(L"EDIT", DEFAULT_SERVER, WS_TABSTOP | WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOVSCROLL, 10, 102, 150, 20, Hwnd, (HMENU)SERVER_TEXTBOX, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL);
			ExtraDialogControls.AvatarServerEditBox = CreateWindowW(L"EDIT", DEFAULT_AVATAR_SERVER, WS_TABSTOP | WS_BORDER | WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOVSCROLL, 10, 125, 150, 20, Hwnd, (HMENU)AVATAR_SERVER_TEXTBOX, (HINSTANCE)GetWindowLongPtr(Hwnd, GWLP_HINSTANCE), NULL);;

			SendMessage(ApplyButton, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
			SendMessage(CleanMraButton, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
			SendMessage(CleanRenButton, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

			SendMessage(CleanMraCaption, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
			SendMessage(CleanRenCaption, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

			SendMessage(ExtraDialogControls.SslCheckbox, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

			SendMessage(ExtraDialogControls.ServerEditBox, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
			SendMessage(ExtraDialogControls.AvatarServerEditBox, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

			GetRegistryValues();

			break;
		}

		case WM_COMMAND: {
			switch (LOWORD(wParam)) {
				case APPLY_BUTTON: {
					if (!SetRegistryValues(TRUE)) MessageBoxW(Hwnd, L"Произошла ошибка при записи настроек в реестр.", L"Ошибка", MB_OK | MB_ICONERROR);
					PostMessage(Hwnd, WM_CLOSE, 0, 0);
					break;
				}

				case MRA_CLEAN_BUTTON: {
					if (MessageBoxW(Hwnd, L"ВНИМАНИЕ! Данная операция необратимо удалит данные Агента из реестра. Она необходима лишь в крайних случаях. Убедитесь в наличии резервных копий и продолжайте на свой страх и риск.", L"Внимание, опасная операция!", MB_YESNO | MB_ICONWARNING) != IDYES)
						break;

					HKEY Hkey;

					if (RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Mail.ru", &Hkey) != ERROR_SUCCESS) {
						MessageBoxW(Hwnd, L"Раздел HKCU\\Software\\Mail.ru не существует", L"Ошибка очистки", MB_OK | MB_ICONERROR);
						break;
					}

					if (SHDeleteKeyW(Hkey, L"Agent") == ERROR_SUCCESS)
						MessageBoxW(Hwnd, L"Раздел HKCU\\Software\\Mail.ru\\Agent был успешно очищен!", L"Успех", MB_OK | MB_ICONINFORMATION);

					else 
						MessageBoxW(Hwnd, L"Ошибка очистки раздела реестра. Возможно у пользователя недостаточно прав или этого раздела не существует.", L"Ошибка очистки", MB_OK | MB_ICONERROR);

					RegCloseKey(Hkey);
					break;
				}

				case REN_CLEAN_BUTTON: {
					if (MessageBoxW(Hwnd, L"ВНИМАНИЕ! Данная операция необратимо очистит настройки патча. Это приведёт к сбросу настроек сервера.", L"Внимание, опасная операция!", MB_YESNO | MB_ICONWARNING) != IDYES)
						break;

					HKEY Hkey;

					if (RegOpenKeyW(HKEY_CURRENT_USER, L"Software", &Hkey) != ERROR_SUCCESS) {
						MessageBoxW(Hwnd, L"Ошибка открытия раздела HKCU\\Software. Возможно у пользователя недостаточно прав.", L"Ошибка очистки", MB_OK | MB_ICONERROR);
						break;
					}

					if (SHDeleteKeyW(Hkey, L"Renaissance") == ERROR_SUCCESS)
						MessageBoxW(Hwnd, L"Раздел HKCU\\Software\\Renaissance был успешно очищен!", L"Успех", MB_OK | MB_ICONINFORMATION);

					else 
						MessageBoxW(Hwnd, L"Ошибка очистки раздела реестра. Возможно у пользователя недостаточно прав или этого раздела не существует.", L"Ошибка очистки", MB_OK | MB_ICONERROR);

					RegCloseKey(Hkey);
					break;
				}
			}
			break;
		}

        case WM_CLOSE: {
			EnableWindow(MainWindow, TRUE);
			DestroyWindow(Hwnd);
			SetFocus(MainWindow);
			break;
		}

        default: {
            return DefWindowProc(Hwnd, Msg, wParam, lParam);
        }
    }
    return 0;
}