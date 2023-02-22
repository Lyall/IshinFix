#include "stdafx.h"
#include "helper.hpp"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

// INI Variables
bool bAspectFix;
bool bFOVFix;
bool bMaxFPS;
int iAspectFix = 0;
int iFOVFix = 0;
int iCustomResX;
int iCustomResY;
int iInjectionDelay;
float fAdditionalFOV;

// Variables
float fNewX;
float fNewY;
float fNativeAspect = 1.777777791f;
float fPi = 3.14159265358979323846f;
float fNewAspect;
string sExeName;
string sGameName;
string sExePath;

// CurrResolution Hook
DWORD64 CurrResolutionReturnJMP;
void __declspec(naked) CurrResolution_CC()
{
    __asm
    {
        mov[iCustomResX], r15d
        mov[iCustomResY], r12d
        cvtsi2ss xmm14, r15d
        cvtsi2ss xmm15, r12d
        divss xmm14,xmm15
        movss [fNewAspect], xmm14
        xorps xmm14,xmm14
        xorps xmm15,xmm15
        mov[rbx + 0x0000009C], r12d
        mov[rbx + 0x000000A4], r12d
        jmp[CurrResolutionReturnJMP]
    }
}

// Aspect Ratio Hook
DWORD64 AspectFixReturnJMP;
void __declspec(naked) AspectFix_CC()
{
    __asm
    {
        movss[rdi + 0x18], xmm0                // Original code
        cmp[iAspectFix], 1                     // Check if aspect ratio fix enabled
        je aspectratio                         // Jump to aspect ratio fix if enabled
        mov eax, [rbx + 0x208]                 // Original code
        mov [fNativeAspect], eax               // Grab native aspect
        mov[rdi + 0x2C], eax                   // Original code
        jmp[AspectFixReturnJMP]                // Jump back to game code

        // Adjust aspect ratio
        aspectratio:
            mov eax, [rbx + 0x208]             // Original code
            mov[fNativeAspect], eax            // Grab native aspect
            mov eax, fNewAspect                // Copy new aspect ratio to eax
            mov[rdi + 0x2C], eax               // Original code
            jmp [AspectFixReturnJMP]           // Jump back to game code
    }
}

// Letterbox Hook
DWORD64 LetterboxReturnJMP;
int iLetterboxCounter = 4; // First 4 are top, bottom, left, right borders
void __declspec(naked) Letterbox_CC()
{
    __asm
    {
        cmp [iLetterboxCounter], 0
        je originalCode
        cmp byte ptr[r14 + 0x6C], 03
        je disableLetterbox
        jmp originalCode

        disableLetterbox:
            dec [iLetterboxCounter]
            mov byte ptr[r14 + 0x6C], 00    // Disable draw
            jmp originalCode

        originalCode:
            mov r13d, [rbp + 0x00000088]
            mov[rsp + 0x00000118], r12
            jmp[LetterboxReturnJMP]
    }
}

// FOV Hook
DWORD64 FOVFixReturnJMP;
float FOVPiDiv;
float FOVDivPi;
float FOVFinalValue;
void __declspec(naked) FOVFix_CC()
{
    __asm
    {
        fld dword ptr[rdx + 0x18]          // Push original FOV to FPU register st(0)
        fmul[FOVPiDiv]                     // Multiply st(0) by Pi/360
        fptan                              // Get partial tangent. Store result in st(1). Store 1.0 in st(0)
        fxch st(1)                         // Swap st(1) to st(0)
        fdiv[fNativeAspect]                // Divide st(0) by 1.778~
        fmul[fNewAspect]                   // Multiply st(0) by new aspect ratio
        fxch st(1)                         // Swap st(1) to st(0)
        fpatan                             // Get partial arc tangent from st(0), st(1)
        fmul[FOVDivPi]                     // Multiply st(0) by 360/Pi
        fstp[FOVFinalValue]                // Store st(0) 
        mov eax, [FOVFinalValue]           // Copy final FOV value to eax register
        mov[rcx + 0x18], eax
        mov eax, [rdx + 0x1C]
        mov[rcx + 0x1C], eax
        mov eax, [rdx + 0x20]
        mov[rcx + 0x20], eax
        jmp[FOVFixReturnJMP]
    }
}

// FOV Culling Hook
DWORD64 FOVCullingReturnJMP;
float fOne = (float)1;
void __declspec(naked) FOVCulling_CC()
{
    __asm
    {
        movss xmm1, [fOne]
        movss[rdx + 0x00000310], xmm1
        movsd xmm0, [rbp + 0x000000E0]
        jmp[FOVCullingReturnJMP]
    }
}

void Logging()
{
    loguru::add_file("IshinFix.log", loguru::Truncate, loguru::Verbosity_MAX);
    loguru::set_thread_name("Main");
}

void ReadConfig()
{
    // Initialize config
    // UE4 games use launchers so config path is relative to launcher
    INIReader config(".\\LikeaDragonIshin\\Binaries\\Win64\\IshinFix.ini");

    // Check if config failed to load
    if (config.ParseError() != 0) {
        LOG_F(ERROR, "Can't load config file");
        LOG_F(ERROR, "Parse error: %d", config.ParseError());
    }

    iInjectionDelay = config.GetInteger("IshinFix Parameters", "InjectionDelay", 2000);
    //iCustomResX = config.GetInteger("Custom Resolution", "Width", 0);
    //iCustomResY = config.GetInteger("Custom Resolution", "Height", 0);
    bAspectFix = config.GetBoolean("Fix Aspect Ratio", "Enabled", true);
    bFOVFix = config.GetBoolean("Fix FOV", "Enabled", true);
    fAdditionalFOV = config.GetFloat("Fix FOV", "AdditionalFOV", (float)0);
    bMaxFPS = config.GetBoolean("Remove Framerate Cap", "Enabled", false);

    // Get game name and exe path
    LPWSTR exePath = new WCHAR[_MAX_PATH];
    GetModuleFileNameW(baseModule, exePath, _MAX_PATH);
    wstring exePathWString(exePath);
    sExePath = string(exePathWString.begin(), exePathWString.end());
    wstring wsGameName = Memory::GetVersionProductName();
    sExeName = sExePath.substr(sExePath.find_last_of("/\\") + 1);
    sGameName = string(wsGameName.begin(), wsGameName.end());

    LOG_F(INFO, "Game Name: %s", sGameName.c_str());
    LOG_F(INFO, "Game Path: %s", sExePath.c_str());

    // Custom resolution
    if (iCustomResX > 0 && iCustomResY > 0)
    {
        fNewX = (float)iCustomResX;
        fNewY = (float)iCustomResY;
        fNewAspect = (float)iCustomResX / (float)iCustomResY;
    }
    else
    {
        // Grab desktop resolution
        RECT desktop;
        GetWindowRect(GetDesktopWindow(), &desktop);
        fNewX = (float)desktop.right;
        fNewY = (float)desktop.bottom;
        iCustomResX = (int)desktop.right;
        iCustomResY = (int)desktop.bottom;
        fNewAspect = (float)desktop.right / (float)desktop.bottom;
    }

    // Log config parse
    LOG_F(INFO, "Config Parse: iInjectionDelay: %dms", iInjectionDelay);
    LOG_F(INFO, "Config Parse: bAspectFix: %d", bAspectFix);
    LOG_F(INFO, "Config Parse: bFOVFix: %d", bFOVFix);
    LOG_F(INFO, "Config Parse: fAdditionalFOV: %.2f", fAdditionalFOV);
    LOG_F(INFO, "Config Parse: bMaxFPS: %d", bMaxFPS);
    LOG_F(INFO, "Config Parse: iCustomResX: %d", iCustomResX);
    LOG_F(INFO, "Config Parse: iCustomResY: %d", iCustomResY);
    LOG_F(INFO, "Config Parse: fNewX: %.2f", fNewX);
    LOG_F(INFO, "Config Parse: fNewY: %.2f", fNewY);
    LOG_F(INFO, "Config Parse: fNewAspect: %.4f", fNewAspect);
}


void AspectFOVFix()
{
    if (bAspectFix)
    {
        iAspectFix = 1;

        uint8_t* CurrResolutionScanResult = Memory::PatternScan(baseModule, "44 89 ?? ?? ?? ?? ?? 44 89 ?? ?? ?? ?? ?? E8 ?? ?? ?? ?? 0F 28 ?? 0F 11 ?? ?? 0F 28 ?? ??");
        if (CurrResolutionScanResult)
        {
            DWORD64 CurrResolutionAddress = (uintptr_t)CurrResolutionScanResult;
            int CurrResolutionHookLength = Memory::GetHookLength((char*)CurrResolutionAddress, 13);
            CurrResolutionReturnJMP = CurrResolutionAddress + CurrResolutionHookLength;
            Memory::DetourFunction64((void*)CurrResolutionAddress, CurrResolution_CC, CurrResolutionHookLength);

            LOG_F(INFO, "Current Resolution: Hook length is %d bytes", CurrResolutionHookLength);
            LOG_F(INFO, "Current Resolution: Hook address is 0x%" PRIxPTR, (uintptr_t)CurrResolutionAddress);
        }
        else if (!CurrResolutionScanResult)
        {
            LOG_F(INFO, "Current Resolution: Pattern scan failed.");
        }


        uint8_t* AspectFixScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? 8B ?? ?? ?? ?? ?? 89 ?? ?? 0F ?? ?? ?? ?? ?? ?? 33 ?? ?? 83 ?? ??");
        if (AspectFixScanResult)
        {
            DWORD64 AspectFixAddress = ((uintptr_t)AspectFixScanResult + 0x8);
            int AspectFixHookLength = Memory::GetHookLength((char*)AspectFixAddress, 13);
            AspectFixReturnJMP = AspectFixAddress + AspectFixHookLength;
            Memory::DetourFunction64((void*)AspectFixAddress, AspectFix_CC, AspectFixHookLength);

            LOG_F(INFO, "Aspect Ratio: Hook length is %d bytes", AspectFixHookLength);
            LOG_F(INFO, "Aspect Ratio: Hook address is 0x%" PRIxPTR, (uintptr_t)AspectFixAddress);
        }
        else if (!AspectFixScanResult)
        {
            LOG_F(INFO, "Aspect Ratio: Pattern scan failed.");
        }

        uint8_t* LetterboxScanResult = Memory::PatternScan(baseModule, "44 8B ?? ?? ?? ?? ?? 4C 89 ?? ?? ?? ?? ?? ?? 4D 85 ?? 0F 84 ?? ?? ?? ??");
        if (LetterboxScanResult)
        {
            DWORD64 LetterboxAddress = (uintptr_t)LetterboxScanResult;
            int LetterboxHookLength = Memory::GetHookLength((char*)LetterboxAddress, 13);
            LetterboxReturnJMP = LetterboxAddress + LetterboxHookLength;
            Memory::DetourFunction64((void*)LetterboxAddress, Letterbox_CC, LetterboxHookLength);
            LOG_F(INFO, "Letterbox: Hook length is %d bytes", LetterboxHookLength);
            LOG_F(INFO, "Letterbox: Hook address is 0x%" PRIxPTR, (uintptr_t)LetterboxAddress);
        }
        else if (!LetterboxScanResult)
        {
            LOG_F(INFO, "Letterbox: Pattern scan failed.");
        }
    }

    if (bFOVFix)
    {
        iFOVFix = 1;
        uint8_t* FOVFixScanResult = Memory::PatternScan(baseModule, "83 E0 ?? 31 41 ?? 8B 49 ?? 33 4A ?? 83 E1 ?? 31 4B");
        if (FOVFixScanResult)
        {
            FOVPiDiv = fPi / 360;
            FOVDivPi = 360 / fPi;

            DWORD64 FOVFixAddress = ((uintptr_t)FOVFixScanResult - 0x27);
            int FOVFixHookLength = Memory::GetHookLength((char*)FOVFixAddress, 13);
            FOVFixReturnJMP = FOVFixAddress + FOVFixHookLength;
            Memory::DetourFunction64((void*)FOVFixAddress, FOVFix_CC, FOVFixHookLength);

            LOG_F(INFO, "FOV: Hook length is %d bytes", FOVFixHookLength);
            LOG_F(INFO, "FOV: Hook address is 0x%" PRIxPTR, (uintptr_t)FOVFixAddress);
        }
        else if (!FOVFixScanResult)
        {
            LOG_F(INFO, "FOV: Pattern scan failed.");
        }

        uint8_t* FOVCullingScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? F2 0F ?? ?? ?? ?? ?? ?? 8B ?? ?? ?? ?? ?? F2 0F ?? ?? ?? ?? 89 ?? ?? ?? 84 ?? 75 ??");
        if (FOVCullingScanResult)
        {
            DWORD64 FOVCullingAddress = (uintptr_t)FOVCullingScanResult;
            int FOVCullingHookLength = Memory::GetHookLength((char*)FOVCullingAddress, 13);
            FOVCullingReturnJMP = FOVCullingAddress + FOVCullingHookLength;
            Memory::DetourFunction64((void*)FOVCullingAddress, FOVCulling_CC, FOVCullingHookLength);
            LOG_F(INFO, "FOV Culling: Hook length is %d bytes", FOVCullingHookLength);
            LOG_F(INFO, "FOV Culling: Hook address is 0x%" PRIxPTR, (uintptr_t)FOVCullingAddress);
        }
        else if (!FOVCullingScanResult)
        {
            LOG_F(INFO, "FOV Culling: Pattern scan failed.");
        }
    }
}

void MaxFPS()
{
    if (bMaxFPS)
    {
        uint8_t* MaxFPSScanResult = Memory::PatternScan(baseModule, "EB ?? 48 ?? ?? 44 0F ?? ?? ?? 48 ?? ?? ?? ?? 73 ?? 80 ?? ?? ?? ?? ?? 00 74 ??");
        if (MaxFPSScanResult)
        {
            DWORD64 MaxFPSAddress = (uintptr_t)(MaxFPSScanResult + 0x5);
            Memory::PatchBytes(MaxFPSAddress, "\x90\x90\x90\x90\x90", 5);
            LOG_F(INFO, "MaxFPS: Patched byte(s) at 0x%" PRIxPTR, (uintptr_t)MaxFPSAddress);
        }
        else if (!MaxFPSScanResult)
        {
            LOG_F(INFO, "MaxFPS: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    AspectFOVFix();
    MaxFPS();
    return true; // end thread
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        CreateThread(NULL, 0, Main, 0, NULL, 0);
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

