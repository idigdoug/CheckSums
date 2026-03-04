// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

struct fclose_delete
{
    void operator()(FILE* file) const noexcept;
};
using unique_FILE = std::unique_ptr<FILE, fclose_delete>;

// _wfsopen. On error: logs error, calls SetLastError, returns nullptr.
unique_FILE
FopenWithLogging(
    _In_z_ PCWSTR filename,
    _In_z_ PCWSTR mode,
    int shFlag = _SH_DENYWR) noexcept;

struct CloseFile_delete
{
    void operator()(HANDLE handle) const noexcept;
};
using unique_HANDLE = std::unique_ptr<void, CloseFile_delete>;

// CreateFileW
unique_HANDLE
CreateFileUnique(
    _In_z_ PCWSTR fileName,
    DWORD desiredAccess,
    DWORD shareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD creationDisposition,
    DWORD flagsAndAttributes,
    _In_opt_ HANDLE templateFile = nullptr) noexcept;

struct FindClose_delete
{
    void operator()(HANDLE handle) const noexcept;
};

using unique_FIND = std::unique_ptr<void, FindClose_delete>;

// FindFirstFileExW(Basic).
unique_FIND
FindFirstFileUnique(
    _In_ LPCWSTR pFileName,
    _Out_ WIN32_FIND_DATAW* pFindFileData,
    _In_ FINDEX_SEARCH_OPS searchOp,
    _In_ DWORD additionalFlags = 0) noexcept;

// CompareStringOrdinal
bool
StrEqualIgnoreCase(_In_z_ PCWSTR str1, _In_z_ PCWSTR str2) noexcept;

// memcmp
bool
ByteSpansEqual(std::span<BYTE const> left, std::span<BYTE const> right) noexcept;

// Returns 256 if the character is not a valid hex character.
unsigned
HexCharToValue(wchar_t ch);

// reserve and push_back.
void 
AssignHexUpper(_Inout_ std::string* pDest, _In_ std::span<BYTE const> bytes);

bool
IsEmptyOrEndsWithSlash(std::wstring_view path) noexcept;

// If directory is empty, returns "".
// Otherwise, guarantees that the returned string ends with a slash.
std::wstring
EnsureEmptyOrEndsWithSlash(std::wstring_view directory);

// fprintf(stderr, format, ...).
void
Log(_Printf_format_string_ PCSTR format, ...);
