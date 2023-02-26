#include "stdafx.h"
#include "helper.hpp"

using namespace std;

HMODULE baseModule = GetModuleHandle(NULL);

inipp::Ini<char> ini;

// INI Variables
bool bAspectFix;
bool bFOVFix;
bool bCutsceneFPS;
bool bControllerType;
int iControllerType;
int iCustomResX;
int iCustomResY;
int iInjectionDelay;
float fAdditionalFOV;
int iAspectFix;
int iFOVFix;

// Variables
float fNewX;
float fNewY;
float fNativeAspect = (float)16/9;
float fPi = 3.14159265358979323846f;
float fNewAspect;
string sExeName;
string sGameName;
string sExePath;
string sGameVersion;
string sFixVer = "1.0.4";

// CurrResolution Hook
DWORD64 CurrResolutionReturnJMP;
void __declspec(naked) CurrResolution_CC()
{
    __asm
    {
        mov r12d, r9d                          // Original code
        mov rbx, rdx                           // Original code
        mov rdi, [rax + r8 * 0x8]              // Original code
        add rdi, rcx                           // Original code
        mov eax, [rdi]                         // Original code

        mov[iCustomResX], r15d                 // Grab current resX
        mov[iCustomResY], r12d                 // Grab current resY
        cvtsi2ss xmm14, r15d
        cvtsi2ss xmm15, r12d
        divss xmm14,xmm15
        movss [fNewAspect], xmm14              // Grab current aspect ratio
        xorps xmm14,xmm14
        xorps xmm15,xmm15
        jmp[CurrResolutionReturnJMP]
    }
}

// Aspect Ratio/FOV Hook
DWORD64 AspectFOVFixReturnJMP;
float FOVPiDiv;
float FOVDivPi;
float FOVFinalValue;
void __declspec(naked) AspectFOVFix_CC()
{
    __asm
    {
        mov eax, [fNewAspect]                  // Move new aspect to eax
        cmp eax, [fNativeAspect]               // Compare new aspect to native
        jle originalCode                       // Skip FOV fix if fNewAspect<=fNativeAspect
        cmp [iFOVFix], 1                       // Check if FOVFix is enabled
        je modifyFOV                           // jmp to FOV fix
        jmp originalCode                       // jmp to originalCode

        modifyFOV:
            fld dword ptr[rbx + 0x1F8]         // Push original FOV to FPU register st(0)
            fmul[FOVPiDiv]                     // Multiply st(0) by Pi/360
            fptan                              // Get partial tangent. Store result in st(1). Store 1.0 in st(0)
            fxch st(1)                         // Swap st(1) to st(0)
            fdiv[fNativeAspect]                // Divide st(0) by 1.778~
            fmul[fNewAspect]                   // Multiply st(0) by new aspect ratio
            fxch st(1)                         // Swap st(1) to st(0)
            fpatan                             // Get partial arc tangent from st(0), st(1)
            fmul[FOVDivPi]                     // Multiply st(0) by 360/Pi
            fadd[fAdditionalFOV]               // Add additional FOV
            fstp[FOVFinalValue]                // Store st(0) 
            movss xmm0, [FOVFinalValue]        // Copy final FOV value to xmm0
            jmp originalCode

        originalCode:
            movss[rdi + 0x18], xmm0            // Original code
            cmp [iAspectFix], 1
            je modifyAspect
            mov eax, [rbx + 0x00000208]        // Original code
            mov[rdi + 0x2C], eax               // Original code
            jmp [AspectFOVFixReturnJMP]

        modifyAspect:
            mov eax, [fNewAspect]
            mov[rdi + 0x2C], eax               // Original code
            jmp[AspectFOVFixReturnJMP]                         
    }
}

// Minigame Aspect Ratio Hook
DWORD64 MinigameAspectFixReturnJMP;
float fGreaterAspect = 1.7f;
void __declspec(naked) MinigameAspectFix_CC()
{
    __asm
    {
        cmp [iAspectFix], 1
        jne originalCode
        cmp eax, [fGreaterAspect]                     
        jge modifyMinigameAspect                // Jump if aspect >1.7
        jmp originalCode                        // Jump back to game code

        // Adjust aspect ratio
        modifyMinigameAspect:
            mov eax, [fNewAspect]               // Move new aspect ratio to eax
            jmp originalCode                    // Jump back to game code

        originalCode:
            mov[rdi + 0x3C], eax                // Original code
            mov ecx, [rbx + 0x00002810]         // Original code
            mov eax, [rdi + 0x40]               // Original code
            and ecx, 02                         // Original code
            jmp[MinigameAspectFixReturnJMP]
    }
}

// Letterbox Hook
DWORD64 LetterboxReturnJMP;
int iLetterboxCounter = 4;                      // First 4 are bottom, left, right, top borders
void __declspec(naked) Letterbox_CC()
{
    __asm
    {
        cmp [iLetterboxCounter], 0              // Compare loop counter
        je originalCode                         // jmp to original code if loop done
        cmp byte ptr[r14 + 0x6C], 03            // compare draw flag
        je disableLetterbox                     // jmp to disable letterbox loop
        jmp originalCode                        // jmp just in case

        disableLetterbox:
            dec [iLetterboxCounter]             // Decrease loop count down from 4
            mov byte ptr[r14 + 0x6C], 00        // Set draw flag to 0
            jmp originalCode                    // jmp and carry on

        originalCode:
            mov r13d, [rbp + 0x00000088]        // Original code
            mov[rsp + 0x00000118], r12          // Original code
            jmp[LetterboxReturnJMP]
    }
}

// Letterbox Hook (Microsoft Store)
void __declspec(naked) LetterboxMS_CC()
{
    __asm
    {
        cmp[iLetterboxCounter], 0               // Compare loop counter
        je originalCode                         // jmp to original code if loop done
        cmp byte ptr[r15 + 0x6C], 03            // compare draw flag
        je disableLetterbox                     // jmp to disable letterbox loop
        jmp originalCode                        // jmp just in case

        disableLetterbox :
        dec[iLetterboxCounter]                  // Decrease loop count down from 4
            mov byte ptr[r15 + 0x6C], 00        // Set draw flag to 0
            jmp originalCode                    // jmp and carry on

        originalCode :
            mov r13d, [rbp + 0x000000e8]        // Original code
            mov[rsp + 0x00000178], r12          // Original code
            jmp[LetterboxReturnJMP]
    }
}

// FOV Culling Hook
DWORD64 FOVCullingReturnJMP;
float fOne = (float)1;
void __declspec(naked) FOVCulling_CC()
{
    __asm
    {
        movss xmm1, [fOne]                      // 90/90, there is undoubtedly a smarter way of doing this
        movss[rdx + 0x00000310], xmm1           // Original code
        movsd xmm0, [rbp + 0x000000E0]          // Original code
        jmp[FOVCullingReturnJMP]
    }
}

// FOV Culling Hook (Microsoft Store)
void __declspec(naked) FOVCullingMS_CC()
{
    __asm
    {
        movss xmm1, [fOne]                      // 90/90, there is undoubtedly a smarter way of doing this
        movss[rdx + 0x00000310], xmm1           // Original code
        xor r8d,r8d                             // Original code
        movsd xmm0, [rbp+0xD0]                  // Original code
        jmp[FOVCullingReturnJMP]
    }
}

// Controller Glyph Hook
DWORD64 ControllerTypeReturnJMP;
uint8_t iControllerGlyph;
void __declspec(naked) ControllerType_CC()
{
    __asm
    {
        mov rdx, rdi                            // Original code
        lea rcx, [rsp + 0x00000080]             // Original code
        cmovne rdx, [rsp + 0x30]                // Original code
        mov bl, iControllerGlyph
        jmp [ControllerTypeReturnJMP]
    }
}

void Logging()
{
    loguru::add_file("IshinFix.log", loguru::Truncate, loguru::Verbosity_MAX);
    loguru::set_thread_name("Main");

    LOG_F(INFO, "IshinFix v%s loaded", sFixVer.c_str());
}

void ReadConfig()
{
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

    // Initialize config
    // UE4 games use launchers so config path is relative to launcher
    if (sExePath.find("WinGDK") != string::npos)
    {
        sGameVersion = "Microsoft Store";
        std::ifstream iniFile(".\\LikeaDragonIshin\\Binaries\\WinGDK\\IshinFix.ini");
        if (!iniFile)
        {
            LOG_F(ERROR, "Failed to load config file. (Microsoft Store)");
        }
        else
        {
            ini.parse(iniFile);
        } 
    }
    else
    {
        sGameVersion = "Steam";
        std::ifstream iniFile(".\\LikeaDragonIshin\\Binaries\\Win64\\IshinFix.ini");
        if (!iniFile)
        {
            LOG_F(ERROR, "Failed to load config file. (Steam)");
        }
        else
        {
            ini.parse(iniFile);
        }
    }
    LOG_F(INFO, "Game Version: %s", sGameVersion.c_str());

    inipp::get_value(ini.sections["IshinFix Parameters"], "InjectionDelay", iInjectionDelay);
    inipp::get_value(ini.sections["Fix Aspect Ratio"], "Enabled", bAspectFix);
    iAspectFix = (int)bAspectFix;
    inipp::get_value(ini.sections["Fix FOV"], "Enabled", bFOVFix);
    iFOVFix = (int)bFOVFix;
    inipp::get_value(ini.sections["Fix FOV"], "AdditionalFOV", fAdditionalFOV);
    inipp::get_value(ini.sections["Remove Cutscene FPS Cap"], "Enabled", bCutsceneFPS);
    inipp::get_value(ini.sections["Override Controller Icons"], "Enabled", bControllerType);
    inipp::get_value(ini.sections["Override Controller Icons"], "Type", iControllerType);

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
    LOG_F(INFO, "Config Parse: bCutsceneFPS: %d", bCutsceneFPS);
    LOG_F(INFO, "Config Parse: bControllerType: %d", bControllerType);
    LOG_F(INFO, "Config Parse: iControllerType: %d", iControllerType);
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
        uint8_t* CurrResolutionScanResult = Memory::PatternScan(baseModule, "33 ?? B9 ?? ?? ?? ?? 45 ?? ?? 48 ?? ?? 4A ?? ?? ?? 48 ?? ?? 8B ??");
        if (CurrResolutionScanResult)
        {
            DWORD64 CurrResolutionAddress = (uintptr_t)CurrResolutionScanResult + 0x7;
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

        uint8_t* AspectFOVFixScanResult = Memory::PatternScan(baseModule, "F3 0F ?? ?? ?? ?? ?? ?? F3 0F ?? ?? ?? 8B ?? ?? ?? ?? ?? 89 ?? ?? 0F ?? ?? ?? ?? ?? ?? 33 ?? ?? 83 ?? ??");
        if (AspectFOVFixScanResult)
        {
            FOVPiDiv = fPi / 360;
            FOVDivPi = 360 / fPi;

            DWORD64 AspectFOVFixAddress = (uintptr_t)AspectFOVFixScanResult + 0x8;
            int AspectFOVFixHookLength = Memory::GetHookLength((char*)AspectFOVFixAddress, 13);
            AspectFOVFixReturnJMP = AspectFOVFixAddress + AspectFOVFixHookLength;
            Memory::DetourFunction64((void*)AspectFOVFixAddress, AspectFOVFix_CC, AspectFOVFixHookLength);

            LOG_F(INFO, "Aspect Ratio/FOV: Hook length is %d bytes", AspectFOVFixHookLength);
            LOG_F(INFO, "Aspect Ratio/FOV: Hook address is 0x%" PRIxPTR, (uintptr_t)AspectFOVFixAddress);
        }
        else if (!AspectFOVFixScanResult)
        {
            LOG_F(INFO, "Aspect Ratio/FOV: Pattern scan failed.");
        }

        uint8_t* MinigameAspectFixScanResult = Memory::PatternScan(baseModule, "89 47 ?? 8B 8B ?? ?? ?? ?? 8B 47");
        if (MinigameAspectFixScanResult)
        {
            DWORD64 MinigameAspectFixAddress = (uintptr_t)MinigameAspectFixScanResult;
            int MinigameAspectFixHookLength = Memory::GetHookLength((char*)MinigameAspectFixAddress, 13);
            MinigameAspectFixReturnJMP = MinigameAspectFixAddress + MinigameAspectFixHookLength;
            Memory::DetourFunction64((void*)MinigameAspectFixAddress, MinigameAspectFix_CC, MinigameAspectFixHookLength);

            LOG_F(INFO, "Minigame Aspect Ratio: Hook length is %d bytes", MinigameAspectFixHookLength);
            LOG_F(INFO, "Minigame Aspect Ratio: Hook address is 0x%" PRIxPTR, (uintptr_t)MinigameAspectFixAddress);
        }
        else if (!MinigameAspectFixScanResult)
        {
            LOG_F(INFO, "Minigame Aspect Ratio: Pattern scan failed.");
        }

        uint8_t* LetterboxScanResult = Memory::PatternScan(baseModule, "44 8B ?? ?? ?? ?? ?? 4C 89 ?? ?? ?? ?? ?? ?? 4D 85 ?? 0F 84 ?? ?? ?? ??");
        if (LetterboxScanResult)
        {
            DWORD64 LetterboxAddress = (uintptr_t)LetterboxScanResult;
            int LetterboxHookLength = Memory::GetHookLength((char*)LetterboxAddress, 13);
            LetterboxReturnJMP = LetterboxAddress + LetterboxHookLength;

            if (sGameVersion == "Steam")
            {
                Memory::DetourFunction64((void*)LetterboxAddress, Letterbox_CC, LetterboxHookLength);
            }
            else if (sGameVersion == "Microsoft Store")
            {
                Memory::DetourFunction64((void*)LetterboxAddress, LetterboxMS_CC, LetterboxHookLength);
            }

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
        uint8_t* FOVCullingScanResult = Memory::PatternScan(baseModule, "8B ?? ?? ?? ?? ?? F2 ?? ?? ?? ?? ?? 89 ?? ?? ?? 84 ?? 75 ??");
        if (FOVCullingScanResult)
        {
            if (sGameVersion == "Steam")
            {
                DWORD64 FOVCullingAddress = (uintptr_t)FOVCullingScanResult - 0x10;
                int FOVCullingHookLength = Memory::GetHookLength((char*)FOVCullingAddress, 13);
                FOVCullingReturnJMP = FOVCullingAddress + FOVCullingHookLength;
                Memory::DetourFunction64((void*)FOVCullingAddress, FOVCulling_CC, FOVCullingHookLength);

                LOG_F(INFO, "FOV Culling: Hook length is %d bytes", FOVCullingHookLength);
                LOG_F(INFO, "FOV Culling: Hook address is 0x%" PRIxPTR, (uintptr_t)FOVCullingAddress);
            }
            else if (sGameVersion == "Microsoft Store")
            {
                DWORD64 FOVCullingAddress = (uintptr_t)FOVCullingScanResult - 0x13;
                int FOVCullingHookLength = Memory::GetHookLength((char*)FOVCullingAddress, 13);
                FOVCullingReturnJMP = FOVCullingAddress + FOVCullingHookLength;
                Memory::DetourFunction64((void*)FOVCullingAddress, FOVCullingMS_CC, FOVCullingHookLength);

                LOG_F(INFO, "FOV Culling: Hook length is %d bytes", FOVCullingHookLength);
                LOG_F(INFO, "FOV Culling: Hook address is 0x%" PRIxPTR, (uintptr_t)FOVCullingAddress);
            }
        }
        else if (!FOVCullingScanResult)
        {
            LOG_F(INFO, "FOV Culling: Pattern scan failed.");
        }
    }
}

void CutsceneFPS()
{
    if (bCutsceneFPS)
    {
        uint8_t* CutsceneFPSScanResult = Memory::PatternScan(baseModule, "C7 ?? ?? ?? ?? ?? ?? 02 ?? ?? ?? FF ?? ?? ?? ?? ?? C6 ?? ?? ?? ?? ?? 01 48");
        if (CutsceneFPSScanResult)
        {
            DWORD64 CutsceneFPSAddress = (uintptr_t)CutsceneFPSScanResult + 0x7;
            Memory::PatchBytes(CutsceneFPSAddress, "\x01", 1);
            LOG_F(INFO, "CutsceneFPS: Patched byte(s) at 0x%" PRIxPTR, (uintptr_t)CutsceneFPSAddress);
        }
        else if (!CutsceneFPSScanResult)
        {
            LOG_F(INFO, "CutsceneFPS: Pattern scan failed.");
        }
    }
}

void ControllerType()
{
    if (bControllerType)
    {
        uint8_t* ControllerTypeScanResult = Memory::PatternScan(baseModule, "48 ?? ?? 48 8D ?? ?? ?? ?? ?? ?? 48 0F ?? ?? ?? ?? 45 ?? ?? E8 ?? ?? ?? ??");
        if (ControllerTypeScanResult)
        {
            iControllerGlyph = (uint8_t)iControllerType;
            DWORD64 ControllerTypeAddress = (uintptr_t)ControllerTypeScanResult;
            int ControllerTypeHookLength = Memory::GetHookLength((char*)ControllerTypeAddress, 13);
            ControllerTypeReturnJMP = ControllerTypeAddress + ControllerTypeHookLength;
            Memory::DetourFunction64((void*)ControllerTypeAddress, ControllerType_CC, ControllerTypeHookLength);

            LOG_F(INFO, "Controller Icons: Hook length is %d bytes", ControllerTypeHookLength);
            LOG_F(INFO, "Controller Icons: Hook address is 0x%" PRIxPTR, (uintptr_t)ControllerTypeAddress);
        }
        else if (!ControllerTypeScanResult)
        {
            LOG_F(INFO, "Controller Icons: Pattern scan failed.");
        }
    }
}

DWORD __stdcall Main(void*)
{
    Logging();
    ReadConfig();
    Sleep(iInjectionDelay);
    AspectFOVFix();
    CutsceneFPS();
    ControllerType();
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
        HANDLE mainHandle = CreateThread(NULL, 0, Main, 0, NULL, 0);

        if (mainHandle)
        {
            CloseHandle(mainHandle);
        }
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

