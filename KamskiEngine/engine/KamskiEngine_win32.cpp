#include "../KamskiEngine.h"
#define NOMINMAX
#include <Windows.h>
#include <cstdio>

struct Win32State
{
    HWND window;
};

Win32State* win32State = nullptr;

// PLATFORM

void kamskiPlatformReadWholeFile(void* buffer, const u64 bufferSize, const char* filePath)
{
    const HANDLE fileHandle = CreateFileA(filePath,
                                          GENERIC_READ,
                                          FILE_SHARE_READ,
                                          nullptr,
                                          OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL,
                                          nullptr);

    DWORD bytesRead = 0;
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        if (!ReadFile(fileHandle,
                      buffer,
                      bufferSize,
                      &bytesRead,
                      nullptr))
        {
            logError("Could not read file %s", filePath);
        }
    }
    else
    {
        logError("File not found %s", filePath);
    }

    CloseHandle(fileHandle);
}

void kamskiPlatformWriteFile(const void* const buffer, u64 bufferSize, const char* filePath)
{
    const HANDLE fileHandle = CreateFileA(filePath,
                                          GENERIC_WRITE,
                                          0,
                                          nullptr,
                                          CREATE_ALWAYS,
                                          FILE_ATTRIBUTE_NORMAL,
                                          nullptr);
    assert(fileHandle != INVALID_HANDLE_VALUE);
    if (!WriteFile(fileHandle,
                   buffer,
                   bufferSize,
                   0,
                   0))
    {
        logError("Could not write to file %s!", filePath);
    }
    CloseHandle(fileHandle);
}

u64 kamskiPlatformGetFileSize(const char* filePath)
{
    const HANDLE fileHandle = CreateFileA(filePath,
                                          GENERIC_READ,
                                          FILE_SHARE_READ,
                                          nullptr,
                                          OPEN_EXISTING,
                                          FILE_ATTRIBUTE_NORMAL,
                                          nullptr);

    LARGE_INTEGER fileSize;
    const bool gotFile = GetFileSizeEx(fileHandle, &fileSize);
    assert(gotFile);
    CloseHandle(fileHandle);
    return fileSize.QuadPart;
}

// LOGGER

void kamskiLog(const char* const format,
               const char* fileName,
               const int lineNumber,
               const char* funcName,
               KamskiLogLevel logLevel,
               ...)
{
    const HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    const char* logLevelPrefixes[] = {
        "[ERROR]",
        "[WARNING]",
        "[INFO]",
        "[DEBUG]"
    };

    char buff[4096] = {};
    va_list args;
    va_start(args, logLevel);

    vsprintf(buff, format, args);

    va_end(args);

    char finalMessage[4096] = {};

    const i32 charsWritten = sprintf(finalMessage,
                                     "%s [%s][%s()]:%d : %s\n",
                                     logLevelPrefixes[(u32)logLevel],
                                     fileName,
                                     funcName,
                                     lineNumber,
                                     buff);

    switch (logLevel)
    {

        case KamskiLogLevel::Error:
        {
            SetConsoleTextAttribute(stdoutHandle, FOREGROUND_RED);
        }
        break;

        case KamskiLogLevel::Warning:
        {
            SetConsoleTextAttribute(stdoutHandle, FOREGROUND_RED | FOREGROUND_GREEN);
        }
        break;

        case KamskiLogLevel::Info:
        {
            SetConsoleTextAttribute(stdoutHandle, FOREGROUND_GREEN);
        }
        break;

        default:
        {
        }
    }

    WriteConsole(stdoutHandle, finalMessage, charsWritten, nullptr, nullptr);
    SetConsoleTextAttribute(stdoutHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

// TIME

f64 kamskiPlatformGetTime()
{
    static LARGE_INTEGER freq = {};

    if (freq.QuadPart == 0)
        QueryPerformanceFrequency(&freq);

    LARGE_INTEGER perf;
    QueryPerformanceCounter(&perf);
    return (f64)perf.QuadPart / (f64)freq.QuadPart;
}


// OTHER

void exit(u32 code)
{
    ShowWindow(win32State->window, SW_HIDE);
    ExitProcess(code);
}
