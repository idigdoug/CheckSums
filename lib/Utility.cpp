// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include <Utility.h>
#include <ProgramOptions.h>

void
fclose_delete::operator()(FILE* file) const noexcept
{
    fclose(file);
}

unique_FILE
FopenWithLogging(
    _In_z_ PCWSTR filename,
    _In_z_ PCWSTR mode,
    int shFlag) noexcept
{
    _doserrno = 0;
    auto file = unique_FILE(_wfsopen(filename, mode, shFlag));
    if (!file)
    {
        auto const doserror = _doserrno;
        LogError("Could not open '%ls' (error %u).",
            filename,
            doserror);
        SetLastError(doserror ? doserror : ERROR_OPEN_FAILED);
    }
    return file;
}

unique_FILE
FopenTextInputWithLogging(
    _In_z_ PCWSTR filename,
    int shFlag) noexcept
{
    auto listFileBinary = FopenWithLogging(filename, L"rb", _SH_DENYWR);
    if (!listFileBinary)
    {
        return nullptr;
    }

    UINT8 bom[3];
    auto cb = fread(bom, 1, sizeof(bom), listFileBinary.get());
    PCWSTR mode;
    if (cb == sizeof(bom) && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)
    {
        mode = L"rt, ccs=UTF-8";
    }
    else if (cb >= 2 && bom[0] == 0xFF && bom[1] == 0xFE)
    {
        mode = L"rt, ccs=UTF-16LE";
    }
    else
    {
        mode = L"rt";
    }

    return FopenWithLogging(filename, mode, shFlag);
}

void
CloseFile_delete::operator()(HANDLE handle) const noexcept
{
    CloseHandle(handle);
}

unique_HANDLE
CreateFileUnique(
    _In_z_ PCWSTR fileName,
    DWORD desiredAccess,
    DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD creationDisposition,
    DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile) noexcept
{
    auto const handle = CreateFileW(
        fileName,
        desiredAccess,
        shareMode,
        pSecurityAttributes,
        creationDisposition,
        flagsAndAttributes,
        templateFile);
    return unique_HANDLE(handle == INVALID_HANDLE_VALUE ? nullptr : handle);
}

void
FindClose_delete::operator()(HANDLE handle) const noexcept
{
    FindClose(handle);
}

unique_FIND
FindFirstFileUnique(
    _In_ LPCWSTR pFileName,
    _Out_ WIN32_FIND_DATAW* pFindFileData,
    _In_ FINDEX_SEARCH_OPS searchOp,
    _In_ DWORD additionalFlags) noexcept
{
    auto handle = FindFirstFileExW(
        pFileName,
        FindExInfoBasic,
        pFindFileData,
        searchOp,
        nullptr,
        additionalFlags);
    if (handle == INVALID_HANDLE_VALUE)
    {
        handle = nullptr;
    }
    return unique_FIND(handle);
}

bool
StrEqualIgnoreCase(_In_z_ PCWSTR str1, _In_z_ PCWSTR str2) noexcept
{
    return CSTR_EQUAL == CompareStringOrdinal(str1, -1, str2, -1, TRUE);
}

bool
ByteSpansEqual(std::span<BYTE const> left, std::span<BYTE const> right) noexcept
{
    return
        left.size() == right.size() &&
        0 == memcmp(left.data(), right.data(), left.size());
}

bool
IsEmptyOrEndsWithSlash(std::wstring_view directory) noexcept
{
    if (directory.empty())
    {
        return true;
    }
    else
    {
        auto const lastChar = directory.back();
        return lastChar == L'/' || lastChar == L'\\';
    }
}

std::wstring
EnsureEmptyOrEndsWithSlash(std::wstring_view directory)
{
    std::wstring path;
    if (IsEmptyOrEndsWithSlash(directory))
    {
        path = directory;
    }
    else
    {
        path.reserve(directory.size() + 1);
        path = directory;
        path.push_back(L'\\');
    }

    return path;
}

void
LogRaw(PCSTR message)
{
    fputs(message, stderr);
}

static void
LogImpl(PCWSTR filename, size_t lineNumber, PCSTR level, _Printf_format_string_ PCSTR format, va_list args)
{
    if (lineNumber != 0)
    {
        fprintf(stderr, "%ls(%zu) : %hs : ",
            filename,
            lineNumber,
            level);
    }
    else if (filename != nullptr)
    {
        fprintf(stderr, "%ls : %hs : ",
            filename,
            level);
    }
    else
    {
        fprintf(stderr, "%hs : ",
            level);
    }

    vfprintf(stderr, format, args);
    fputc('\n', stderr);
}

void
LogInvalidFormat(ProgramOptions const& options, _In_z_ PCWSTR listFileName, size_t lineNumber, PCSTR detail)
{
    PCSTR level;
    if (options.strict)
    {
        level = "error";
    }
    else if (options.warn)
    {
        level = "warning";
    }
    else
    {
        return;
    }

    fprintf(stderr, "%ls(%zu) : %hs : Invalid format (%hs).\n",
        listFileName,
        lineNumber,
        level,
        detail);
}

void
LogError(_Printf_format_string_ PCSTR format, ...)
{
    va_list args;
    va_start(args, format);
    LogImpl(nullptr, 0, "error", format, args);
    va_end(args);
}

void
LogError(PCWSTR filename, size_t lineNumber, _Printf_format_string_ PCSTR format, ...)
{
    va_list args;
    va_start(args, format);
    LogImpl(filename, lineNumber, "error", format, args);
    va_end(args);
}

void
LogWarning(_Printf_format_string_ PCSTR format, ...)
{
    va_list args;
    va_start(args, format);
    LogImpl(nullptr, 0, "warning", format, args);
    va_end(args);
}

void
LogWarning(PCWSTR filename, size_t lineNumber, _Printf_format_string_ PCSTR format, ...)
{
    va_list args;
    va_start(args, format);
    LogImpl(filename, lineNumber, "warning", format, args);
    va_end(args);
}

void
LogVerbose(ProgramOptions const& options, _Printf_format_string_ PCSTR format, ...)
{
    if (options.verbose)
    {
        va_list args;
        va_start(args, format);
        LogImpl(nullptr, 0, "verbose", format, args);
        va_end(args);
    }
}
