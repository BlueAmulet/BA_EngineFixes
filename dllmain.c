#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Version
#define MAJOR 1
#define MINOR 0
#define BUILD 0

#define _str(s) #s
#define str(s) _str(s)

// Quick logging routines
static HANDLE hLogFile = INVALID_HANDLE_VALUE;

static void lputs(const char* msg)
{
    if (hLogFile != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        WriteFile(hLogFile, msg, strlen(msg), &bytesWritten, NULL);
        FlushFileBuffers(hLogFile);
    }
}

// Minimal OBSE Plugin structures
#define OBL_1_2_0_416 0x10201A0

typedef DWORD UInt32;

typedef struct _OBSEInterface
{
    UInt32 obseVersion;
    UInt32 runtimeVersion;
    UInt32 editorVersion;
    UInt32 isEditor;
} OBSEInterface;

enum
{
    kInfoVersion = 2
};

typedef struct _PluginInfo
{
    UInt32 infoVersion;
    const char* name;
    UInt32 version;
} PluginInfo;

// Patches
#define JMPA(addr) push addr __asm ret

// Improper strrchr return check
__declspec(naked) int patch1(void)
{
    __asm
    {
        mov   esi, eax
        sub   esi, ebx
        add   esp, 8
        test  eax, eax
        jnz   skip1
        JMPA (4A27A1h)
        skip1:
        JMPA (4A26FEh)
    }
}

// The patcher
static void WriteBuffer(void* addr, void* data, size_t len)
{
    DWORD oldProtect;
    VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(addr, data, len);
    VirtualProtect(addr, len, oldProtect, &oldProtect);
}

static void WriteJMP(char* dst, char* src, size_t len)
{
    // Make memory writable
    DWORD oldProtect;
    VirtualProtect(src, len, PAGE_EXECUTE_READWRITE, &oldProtect);

    // Write JMP
    src[0] = 0xE9;

    // Calculate relative offset
    unsigned char* address = dst - src - 5;
    *(void**)(src + 1) = address;

    // Fill rest of space with nops
    for (size_t i = 5; i < len; i++)
    {
        src[i] = 0x90;
    }

    // Restore memory protections
    VirtualProtect(src, len, oldProtect, &oldProtect);
}

// OBSE Plugin interface
__declspec(dllexport) int OBSEPlugin_Query(const OBSEInterface* obse, PluginInfo* info)
{
    info->infoVersion = kInfoVersion;
    info->name = "BA_EngineFixes";
    info->version = MAJOR << 16 | MINOR << 8 | BUILD << 0;

    if (obse->isEditor)
    {
        lputs("Invalid environment, loaded in editor\n");
        return FALSE;
    }
    else if (obse->runtimeVersion != OBL_1_2_0_416)
    {
        lputs("Invalid environment, wrong oblivion version\n");
        return FALSE;
    }
    return TRUE;
}

__declspec(dllexport) int OBSEPlugin_Load(const OBSEInterface* obse)
{
    if (obse->isEditor)
    {
        lputs("Invalid environment, loaded in editor\n");
        return FALSE;
    }
    else if (obse->runtimeVersion != OBL_1_2_0_416)
    {
        lputs("Invalid environment, wrong oblivion version\n");
        return FALSE;
    }

    // Apply patches
    lputs("Patch1: Improper strrchr return check\n");
    WriteJMP(&patch1, 0x4A26F7, 7);

    // Done applying patches
    lputs("Done applying patches\n");
    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // For optimization
        DisableThreadLibraryCalls(hModule);

        // Open Log file
        hLogFile = CreateFileW(L"BA_EngineFixes.log", GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        lputs("Blue's Engine Fixes v" str(MAJOR) "." str(MINOR) "." str(BUILD) " Initializing\n\n");
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        if (hLogFile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hLogFile);
            hLogFile = INVALID_HANDLE_VALUE;
        }
        break;
    }
    return TRUE;
}
