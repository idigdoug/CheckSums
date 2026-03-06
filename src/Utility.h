// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

struct ProgramOptions;

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

// If file starts with a BOM: _wfopen(filename, "rt, ccs=...", _SH_DENYWR).
// If file has no BOM: _wfopen(filename, "rt", _SH_DENYWR).
// On error: logs error, calls SetLastError, returns nullptr.
unique_FILE
FopenTextInputWithLogging(_In_z_ PCWSTR filename) noexcept;

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

bool
IsEmptyOrEndsWithSlash(std::wstring_view path) noexcept;

// If directory is empty, returns "".
// Otherwise, guarantees that the returned string ends with a slash.
std::wstring
EnsureEmptyOrEndsWithSlash(std::wstring_view directory);

// fputs(message, LOG);
void
LogRaw(PCSTR message);

// fprintf(LOG, "filename(lineNumber) : level : Invalid format (" + detail + ")\n").
void
LogInvalidFormat(ProgramOptions const& options, _In_z_ PCWSTR listFileName, size_t lineNumber, PCSTR detail);

// fprintf(LOG, "error : " + format + "\n", ...).
void
LogError(_Printf_format_string_ PCSTR format, ...);

// fprintf(LOG, "error : " + format + "\n", ...).
void
LogError(PCWSTR filename, size_t lineNumber, _Printf_format_string_ PCSTR format, ...);

// fprintf(LOG, "warning : " + format + "\n", ...).
void
LogWarning(_Printf_format_string_ PCSTR format, ...);

// fprintf(LOG, "warning : " + format + "\n", ...).
void
LogWarning(PCWSTR filename, size_t lineNumber, _Printf_format_string_ PCSTR format, ...);

// fprintf(LOG, "verbose : " + format + "\n", ...).
void
LogVerbose(ProgramOptions const& options, _Printf_format_string_ PCSTR format, ...);
