#include "peutils.h"

VOID WINAPI WritePE(PeHeaders *PeHdrs, PIMAGE_SECTION_HEADER NewImportSection, PIMAGE_NT_HEADERS32 NewNt32, Metadata *SectionData, HANDLE Output) {
	DWORD Written;

	WriteFile(Output, PeHdrs->DosHeader, PeHdrs->DosHeader->e_lfanew, &Written, NULL);
	WriteFile(Output, NewNt32, sizeof(IMAGE_NT_HEADERS32), &Written, NULL);
	WriteFile(Output, NewImportSection, NewNt32->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER), &Written, NULL);

	for (WORD a = 0; a < NewNt32->FileHeader.NumberOfSections; a++) {
		SetFilePointer(Output, SectionData[a].Offset, NULL, FILE_BEGIN);
        WriteFile(Output, SectionData[a].DataPointer, SectionData[a].Size, &Written, NULL);
	}
}

BOOL WINAPI AddImport(PeHeaders *PeHdr, Metadata *SectionData, PIMAGE_NT_HEADERS32 NewNt32, PIMAGE_SECTION_HEADER NewSectionTable) {
	CONST DWORD OldImportTableSize = PeHdr->Nt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size,
		*OldImportTable = (PDWORD)PeHdr->ImportDesc;

	for (PIMAGE_IMPORT_DESCRIPTOR ImpDesc = PeHdr->ImportDesc; ImpDesc->Name; ImpDesc++) {
		PSTR ImportName = (PSTR)ImageRvaToVa(PeHdr->Nt32, (PBYTE)PeHdr->DosHeader, ImpDesc->Name, NULL);
		if (strcmp(ImportName, IMPDLL) == 0) return FALSE;
	}

	CONST DWORD SectionSize = ALIGN(OldImportTableSize, 512) + 0x200;

	if (!CreateNewSection(IMPSEC, SectionSize, PeHdr, SectionData, NewNt32, NewSectionTable)) return FALSE;

	NewNt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = NewSectionTable[PeHdr->Nt32->FileHeader.NumberOfSections].VirtualAddress;
	NewNt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size += sizeof(IMAGE_IMPORT_DESCRIPTOR);

	PIMAGE_IMPORT_DESCRIPTOR NewDescEntry = (PIMAGE_IMPORT_DESCRIPTOR)SectionData[PeHdr->Nt32->FileHeader.NumberOfSections].DataPointer;

	CopyMemory(NewDescEntry, OldImportTable, OldImportTableSize);
	
	CONST PBYTE PrologueOffset = (PBYTE)NewDescEntry + NewNt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;

	CONST DWORD PrologueRva = NewSectionTable[PeHdr->Nt32->FileHeader.NumberOfSections].VirtualAddress + NewNt32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size;

	CopyMemory(PrologueOffset, IMPDLL, strlen(IMPDLL));
	CopyMemory(PrologueOffset + strlen(IMPDLL) + 3, RENAISIMPORT, strlen(RENAISIMPORT));

	DWORD ImportCount = 0;
	for(; NewDescEntry->Name; NewDescEntry++) ImportCount++;

	NewDescEntry->Name = PrologueRva;
	NewDescEntry->OriginalFirstThunk = PrologueRva + strlen(IMPDLL) + strlen(RENAISIMPORT) + 6;
	NewDescEntry->FirstThunk = PrologueRva + strlen(IMPDLL) + strlen(RENAISIMPORT) + 6;

	IMAGE_THUNK_DATA32 PatchThunk;
	ZeroMemory(&PatchThunk, sizeof(IMAGE_THUNK_DATA32));
	PatchThunk.u1.AddressOfData = PrologueRva + strlen(IMPDLL) + 1;

	CopyMemory(PrologueOffset + strlen(IMPDLL) + strlen(RENAISIMPORT) + 6, &PatchThunk, sizeof(IMAGE_THUNK_DATA32));
	CopyMemory(PrologueOffset + strlen(IMPDLL) + strlen(RENAISIMPORT) + 6 + sizeof(IMAGE_THUNK_DATA32) + 10, EASTER_EGG, strlen(EASTER_EGG));
	return TRUE;
}

BOOL WINAPI CreateNewSection(PSTR SectionName, DWORD Size, PeHeaders *PeHdr, Metadata *SectionData, PIMAGE_NT_HEADERS32 NewNt32, PIMAGE_SECTION_HEADER NewSectionTable) {
	IMAGE_SECTION_HEADER *LastSection = &NewSectionTable[PeHdr->Nt32->FileHeader.NumberOfSections - 1];
	IMAGE_SECTION_HEADER *NewSection = &NewSectionTable[PeHdr->Nt32->FileHeader.NumberOfSections];

	CopyMemory(NewSection->Name, SectionName, 8);

	DWORD FileAlign = PeHdr->Nt32->OptionalHeader.FileAlignment,
		SecAlign = PeHdr->Nt32->OptionalHeader.SectionAlignment;

	DWORD SecOffset = ALIGN(LastSection->PointerToRawData + LastSection->SizeOfRawData, FileAlign),
		SecVA = ALIGN(LastSection->VirtualAddress + LastSection->Misc.VirtualSize, SecAlign),
		SecSize = ALIGN(Size, FileAlign),
		SecVSize = Size;
		
	NewSection->VirtualAddress = SecVA;
	NewSection->SizeOfRawData = SecSize;
	NewSection->PointerToRawData = SecOffset;
	NewSection->Misc.VirtualSize = SecVSize;
		
	NewSection->Characteristics = IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE | IMAGE_SCN_CNT_INITIALIZED_DATA;

	NewNt32->OptionalHeader.SizeOfImage = ALIGN(NewNt32->OptionalHeader.SizeOfImage + NewSection->Misc.VirtualSize, NewNt32->OptionalHeader.SectionAlignment);
	NewNt32->FileHeader.NumberOfSections++;

	SectionData[PeHdr->Nt32->FileHeader.NumberOfSections].DataPointer = GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, Size);

	if (!SectionData[PeHdr->Nt32->FileHeader.NumberOfSections].DataPointer) return FALSE;

	SectionData[PeHdr->Nt32->FileHeader.NumberOfSections].Size = Size;
	SectionData[PeHdr->Nt32->FileHeader.NumberOfSections].Offset = SecOffset;

	return TRUE;
}

VOID WINAPI EnumSections(Metadata *SectionData, PeHeaders *PeHdrs) {
	for (WORD a = 0; a < PeHdrs->Nt32->FileHeader.NumberOfSections; a++) {
		SectionData[a].DataPointer = (PBYTE)PeHdrs->DosHeader + PeHdrs->ImageSection[a].PointerToRawData;
		SectionData[a].Offset = PeHdrs->ImageSection[a].PointerToRawData;
		SectionData[a].Size = PeHdrs->ImageSection[a].SizeOfRawData;
	}
}