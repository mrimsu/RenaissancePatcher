#include "main.h"
#include "peutils.h"
#include "flashpatch.h"
#include "restools.h"
#include "resources.h"

#define BUTTON_1 1

BOOL WINAPI PatchPE(PWSTR InputFile, PWSTR OutFile);
BOOL WINAPI SetStrictServer(VOID);
BOOL WINAPI FileExists(PWSTR FilePath);
BOOL WINAPI MainRoutine(HWND Hwnd, PWSTR FilePath);

LRESULT CALLBACK WindowProc(HWND Hwnd, UINT Msg, WPARAM wParam, LPARAM lParam) {
    switch (Msg) {
        case WM_DESTROY: {
            PostQuitMessage(WM_QUIT);
            break;
        }

		case WM_PAINT: {
			PAINTSTRUCT Ps;
			HDC DC = BeginPaint(Hwnd, &Ps);

			SetBkMode(DC, TRANSPARENT);

			HFONT HeadingFont = CreateFontW(
				-MulDiv(30, GetDeviceCaps(DC, LOGPIXELSY), 72),
				0,
				0,
				0,
				FW_MEDIUM,
				FALSE,
				FALSE,
				FALSE,
				DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS,
				CLIP_CHARACTER_PRECIS,
				CLEARTYPE_QUALITY,
				DEFAULT_PITCH,
				L"Verdana" // ΜΛΜΛ СМΘΤΡΝ Я ПΝШY КΛΚ МΛNКРΘСΘФТ
			 ),

			 BodyFont = CreateFontW(
				-MulDiv(11, GetDeviceCaps(DC, LOGPIXELSY), 72),
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
				L"Verdana"
			 );
			
			SelectObject(DC, HeadingFont);

			PCWSTR Heading = L"Патчер Renaissance",
				Body = L"Выберите вашу копию magent.exe для установки патча сервера Renaissance";

			TextOutW(DC, 8, 3, Heading, wcslen(Heading));

			DeleteObject(HeadingFont);

			SelectObject(DC, BodyFont);
			TextOutW(DC, 8, 60, Body, wcslen(Body));

			EndPaint(Hwnd, &Ps);
			DeleteObject(BodyFont);
			break;
		}

		case WM_COMMAND: {
			switch(LOWORD(wParam)) {
				case BUTTON_1: {
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
			}
			break;
		}

        default: {
            return DefWindowProc(Hwnd, Msg, wParam, lParam);
        }
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE OldInst, LPSTR CmdLine, INT CmdShow) {
	UNREFERENCED_PARAMETER(OldInst);
	UNREFERENCED_PARAMETER(CmdLine);

	//this part is intended for silent install (online installer etc)
	INT Argc;
	PWSTR *Argv = CommandLineToArgvW(GetCommandLineW(), &Argc);
	
	if (Argc > 1) {
		if (!MainRoutine(NULL, Argv[1])) return 1;
		return 0;
	}

    PCWSTR ClassName = L"Renaissance001"; //source engine nerds should guess the reference :D

    WNDCLASSW WndClass;
	ZeroMemory(&WndClass, sizeof(WNDCLASSW));

    WndClass.hInstance = Instance;
    WndClass.hCursor = LoadCursorW(Instance, IDC_ARROW);
    WndClass.hbrBackground = (HBRUSH)COLOR_WINDOW;
    WndClass.hIcon = LoadIconW(Instance, IDI_APPLICATION);
    WndClass.lpszClassName = ClassName;
    WndClass.lpfnWndProc = WindowProc;
    WndClass.style = 0;

    RegisterClass(&WndClass);

    HWND Wnd = CreateWindowW(ClassName, L"Renaissance Patcher", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 640, 200, NULL, NULL, Instance, NULL),
		PatchButton = CreateWindowW(L"BUTTON", L"Пропатчить", WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON, 520, 131, 100, 30, Wnd, (HMENU)BUTTON_1, (HINSTANCE)GetWindowLongPtr(Wnd, GWLP_HINSTANCE), NULL);

	SendMessage(PatchButton, WM_SETFONT, (LPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    ShowWindow(Wnd, CmdShow);
    UpdateWindow(Wnd);

    MSG Msg;

    while (GetMessage(&Msg, NULL, 0, 0)) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }
    return 0;
}

BOOL WINAPI PatchPE(PWSTR InputFile, PWSTR OutFile) {
	HANDLE MagentPe = CreateFileW(InputFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (!MagentPe) return FALSE;

	HANDLE FileMap = CreateFileMappingW(MagentPe, NULL, PAGE_READONLY, 0, 0, NULL);

	SIZE_T PeSize = GetFileSize(MagentPe, 0);

	LPVOID MapBuffer = MapViewOfFile(FileMap, FILE_MAP_READ, 0, 0, PeSize);
	
	if (!MapBuffer) return FALSE;

	PeHeaders PeHdrs;
	PeHdrs.DosHeader = (PIMAGE_DOS_HEADER)MapBuffer;

	if (PeHdrs.DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		UnmapViewOfFile(MapBuffer);
		CloseHandle(MagentPe);
		CloseHandle(FileMap);
		return FALSE;
	}

	PeHdrs.Nt32 = (PIMAGE_NT_HEADERS32)((PBYTE)PeHdrs.DosHeader + PeHdrs.DosHeader->e_lfanew);
	PeHdrs.ImportDesc = (PIMAGE_IMPORT_DESCRIPTOR) ImageRvaToVa(PeHdrs.Nt32, (PBYTE)PeHdrs.DosHeader, PeHdrs.Nt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress, NULL);
	PeHdrs.ImageSection = IMAGE_FIRST_SECTION(PeHdrs.Nt32);

	if (PeHdrs.Nt32->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
		UnmapViewOfFile(MapBuffer);
		CloseHandle(MagentPe);
		CloseHandle(FileMap);
		return FALSE;
	}

	if (PeHdrs.Nt32->FileHeader.Characteristics & IMAGE_FILE_DLL) {
		UnmapViewOfFile(MapBuffer);
		CloseHandle(MagentPe);
		CloseHandle(FileMap);
		return FALSE;
	}

	PIMAGE_SECTION_HEADER NewImageSection = (PIMAGE_SECTION_HEADER) GlobalAlloc(GMEM_ZEROINIT, sizeof(IMAGE_SECTION_HEADER) * (PeHdrs.Nt32->FileHeader.NumberOfSections + 1));

	if (!NewImageSection) {
		UnmapViewOfFile(MapBuffer);
		CloseHandle(MagentPe);
		CloseHandle(FileMap);
		return FALSE;
	}

	IMAGE_NT_HEADERS32 NewNt32;

	CopyMemory(NewImageSection, PeHdrs.ImageSection, PeHdrs.Nt32->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER));
	CopyMemory(&NewNt32, PeHdrs.Nt32, sizeof(IMAGE_NT_HEADERS32));
	
	Metadata *SectionData = (Metadata *) GlobalAlloc(GMEM_ZEROINIT, (PeHdrs.Nt32->FileHeader.NumberOfSections + 1) * sizeof(Metadata));

	EnumSections(SectionData, &PeHdrs);

	if (!AddImport(&PeHdrs, SectionData, &NewNt32, NewImageSection)) {
		GlobalFree(NewImageSection);
		GlobalFree(SectionData);
		UnmapViewOfFile(MapBuffer);
		CloseHandle(MagentPe);
		CloseHandle(FileMap);
		return FALSE;
	}

	HANDLE Output = CreateFileW(OutFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (!Output) {
		GlobalFree(NewImageSection);
		GlobalFree(SectionData[PeHdrs.Nt32->FileHeader.NumberOfSections].DataPointer);
		GlobalFree(SectionData);
		UnmapViewOfFile(MapBuffer);
		CloseHandle(MagentPe);
		CloseHandle(FileMap);
		return FALSE;
	}

	WritePE(&PeHdrs, NewImageSection, &NewNt32, SectionData, Output);
	
	GlobalFree(NewImageSection);
	GlobalFree(SectionData[PeHdrs.Nt32->FileHeader.NumberOfSections].DataPointer);
	GlobalFree(SectionData);
	UnmapViewOfFile(MapBuffer);
	CloseHandle(MagentPe);
	CloseHandle(FileMap);
	CloseHandle(Output);
	return TRUE;
}

BOOL WINAPI SetStrictServer(VOID) {
	HKEY Hkey;
    RegCreateKeyW(HKEY_CURRENT_USER, L"Software\\Mail.ru\\Agent", &Hkey);
    
    PCWSTR Value = L"proto.mrim.su:2041"; 

    if (RegSetValueExW(Hkey, L"strict_server", 0, REG_SZ, (PBYTE)Value, wcslen(Value) * sizeof(WCHAR)) != ERROR_SUCCESS) {
        RegCloseKey(Hkey);
        return FALSE;
    }
    RegCloseKey(Hkey);

    return TRUE;
}

BOOL WINAPI FileExists(PWSTR FilePath) {
	DWORD dwAttrib = GetFileAttributes(FilePath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY) == 0);
}

BOOL WINAPI MainRoutine(HWND Hwnd, PWSTR FilePath) {
	if (!FileExists(FilePath)) return FALSE;

	WCHAR OrigPath[MAX_PATH];

	ZeroMemory(OrigPath, sizeof(WCHAR) * MAX_PATH);

	GetFullPathNameW(FilePath, MAX_PATH, OrigPath, NULL);
	PathRemoveFileSpecW(OrigPath);

	WCHAR DestPath[MAX_PATH];
	ZeroMemory(DestPath, sizeof(WCHAR) * MAX_PATH);

	wcscpy(DestPath, OrigPath);

	PWSTR TmpFilePath = wcscat(OrigPath, L"\\magent.exe.tmp\0");

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

		if (!SetStrictServer())
			MessageBoxW(
				Hwnd, 
				L"Не удалось задать значение HKCU\\Software\\Mail.ru\\Agent | strict_server. \
				Необходимо ввести значение proto.mrim.su:2041", 
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