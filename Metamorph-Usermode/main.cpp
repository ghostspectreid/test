#include "spoofer.h"
#include <windows.h>
#include <stdio.h>

int main(void) {

    SetTitle(XOR("[Metamorph]"));

    Success(XOR("Kernelmode"));
    if (!NT_SUCCESS(Spoofer::LoadKernelmode())) {
        Error(XOR("Failed to load Kernelmode"));
        Newline; SystemPause;
        return STATUS_UNSUCCESSFUL;
    }
    Success(XOR("Done")); Newline;

    Success(XOR("Usermode"));
    if (!NT_SUCCESS(Spoofer::LoadUsermode())) {
        Error(XOR("Failed to load Usermode"));
        Newline; SystemPause;
        return STATUS_UNSUCCESSFUL;
    }
    Success(XOR("Done")); Newline;

    Success(XOR("Metamorph has successfully loaded"));
    Newline;

    Warning(XOR("Make sure all your network adapters are already loaded"));
    Warning(XOR("You must create a new user for a full unban"));
    Newline;

    Success(XOR("Launching RF_RETURN_RPG.exe with parent spoofing..."));
    Sleep(1500);

    char currentDir[MAX_PATH] = { 0 };
    GetModuleFileNameA(NULL, currentDir, MAX_PATH);
    char* lastSlash = strrchr(currentDir, '\\');
    if (lastSlash) *lastSlash = '\0';

    char gamePath[MAX_PATH] = { 0 };
    sprintf_s(gamePath, MAX_PATH, "%s\\RF_RETURN_RPG.exe", currentDir);

    if (GetFileAttributesA(gamePath) == INVALID_FILE_ATTRIBUTES) {
        Error(XOR("RF_RETURN_RPG.exe not found!"));
        Warning(XOR("Put Metamorph.exe inside the game folder"));
        SystemPause;
        return STATUS_UNSUCCESSFUL;
    }

    if (Spoofer::LaunchWithParent(gamePath, currentDir, "explorer.exe")) {
        Success(XOR("Game launched successfully with spoofed parent!"));
    }
    else {
        Error(XOR("Parent spoofing failed. Running as Admin?"));
        SystemPause;
        return STATUS_UNSUCCESSFUL;
    }

    Sleep(3000);
    return STATUS_SUCCESS;
}