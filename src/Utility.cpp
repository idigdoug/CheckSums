// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "Utility.h"

static char const HexDigitsUpper[] = "0123456789ABCDEF";

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
        Log("error : Could not open '%ls' (error %u).\n",
            filename,
            doserror);
        SetLastError(doserror ? doserror : ERROR_OPEN_FAILED);
    }
    return file;
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

unsigned
HexCharToValue(wchar_t ch)
{
    unsigned hexVal;
    unsigned index = ch - 48u;
    if (index < 10u)
    {
        hexVal = index;
    }
    else
    {
        index = (ch | 32u) - 97u;
        if (index < 6u)
        {
            hexVal = index + 10u;
        }
        else
        {
            hexVal = 256;
        }
    }
    return hexVal;
}

void
AssignHexUpper(_Inout_ std::string* pDest, _In_ std::span<BYTE const> bytes)
{
    pDest->clear();
    pDest->reserve(pDest->size() + bytes.size() * 2);
    for (auto const byte : bytes)
    {
        pDest->push_back(static_cast<wchar_t>(HexDigitsUpper[(byte >> 4) & 0xF]));
        pDest->push_back(static_cast<wchar_t>(HexDigitsUpper[byte & 0xF]));
    }
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
Log(_Printf_format_string_ PCSTR format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}
