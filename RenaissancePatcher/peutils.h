#pragma once
#include "main.h"

#define ALIGN(n, a) (((n) + (a) - 1) & ~((a) - 1))

typedef struct {
    PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS32 Nt32;
	PIMAGE_IMPORT_DESCRIPTOR ImportDesc;
	PIMAGE_SECTION_HEADER ImageSection;
} PeHeaders;

typedef struct {
    DWORD Size;
    LPVOID DataPointer;
    DWORD Offset;
} Metadata;

VOID WINAPI WritePE(PeHeaders *PeHdrs, PIMAGE_SECTION_HEADER NewImportSection, PIMAGE_NT_HEADERS32 NewNt32, Metadata *SectionData, HANDLE Output);
BOOL WINAPI CreateNewSection(PSTR SectionName, DWORD Size, PeHeaders *PeHdr, Metadata *SectionData, PIMAGE_NT_HEADERS32 NewNt32, PIMAGE_SECTION_HEADER NewSectionTable);
VOID WINAPI EnumSections(Metadata *SectionData, PeHeaders *PeHdrs);
VOID WINAPI EnumImports(PeHeaders *PeHdrs);
BOOL WINAPI AddImport(PeHeaders *PeHdr, Metadata *SectionData, PIMAGE_NT_HEADERS32 NewNt32, PIMAGE_SECTION_HEADER NewSectionTable);