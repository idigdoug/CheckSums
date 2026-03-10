// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include <HrException.h>
#include <Hasher.h>
#include <LineReader.h>
#include <Utility.h>

static volatile void const* g_pCopyByte;

struct UnmapViewOfFile_delete
{
    void operator()(void* address) const noexcept
    {
        UnmapViewOfFile(address);
    }
};
using unique_ViewOfFile = std::unique_ptr<void, UnmapViewOfFile_delete>;

unique_ViewOfFile
MapViewOfFileUnique(
    _In_ HANDLE fileMappingObject,
    DWORD desiredAccess,
    UINT64 fileOffset,
    _In_ SIZE_T numberOfBytesToMap) noexcept
{
    return unique_ViewOfFile(MapViewOfFile(
        fileMappingObject,
        desiredAccess,
        static_cast<DWORD>(fileOffset >> 32),
        static_cast<DWORD>(fileOffset & 0xFFFFFFFF),
        numberOfBytesToMap));
}

unique_HANDLE
CreateFileMappingUnique(
    _In_     HANDLE file,
    _In_opt_ PSECURITY_ATTRIBUTES pFileMappingAttributes,
    _In_     DWORD protect,
    _In_     UINT64 maximumSize,
    _In_opt_ PCWSTR pName = nullptr) noexcept
{
    return unique_HANDLE(CreateFileMappingW(
        file,
        pFileMappingAttributes,
        protect,
        static_cast<DWORD>(maximumSize >> 32),
        static_cast<DWORD>(maximumSize & 0xFFFFFFFF),
        pName));
}

struct MappedFile
{
    unique_ViewOfFile View;
    SIZE_T Size;
};

HRESULT
MapFile(PCWSTR name, _Out_ MappedFile* pMappedFile)
{
    pMappedFile->View.reset();
    pMappedFile->Size = 0;

    auto file = CreateFileUnique(name,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN);
    if (!file)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(file.get(), &fileSize))
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if (fileSize.QuadPart == 0)
    {
        return S_OK;
    }

    auto fileMapping = CreateFileMappingUnique(
        file.get(),
        nullptr,
        PAGE_READONLY,
        fileSize.QuadPart);
    if (!fileMapping)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    auto view = MapViewOfFileUnique(
        fileMapping.get(),
        FILE_MAP_READ,
        0,
        static_cast<SIZE_T>(fileSize.QuadPart));
    if (!view)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    pMappedFile->View = std::move(view);
    pMappedFile->Size = static_cast<SIZE_T>(fileSize.QuadPart);
    return S_OK;
}

// Returns 256 if the character is not a valid hex character.
static unsigned
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

HRESULT
CheckListFile(
    PCWSTR listFileName,
    FILE* listFile,
    PCWSTR pKnownGoodDir,
    PCWSTR pCopyDir)
{
    HRESULT hr = S_OK;
    HRESULT localHr;

    auto hasher = HasherCreateMurmur3x64_128();
    auto knownGoodPath = EnsureEmptyOrEndsWithSlash(pKnownGoodDir);
    auto const knownGoodPathBaseSize = knownGoodPath.size();
    auto copyPath = EnsureEmptyOrEndsWithSlash(pCopyDir);
    auto const copyPathBaseSize = copyPath.size();

    std::vector<BYTE> expectedHashValue;
    LineReader lineReader;
    size_t fileCount = 0;
    size_t mismatchCount = 0;

    lineReader.SetFile(listFileName, listFile);

    std::wstring_view line;
    while (lineReader.ReadLine(&line))
    {
        PCWSTR filename = nullptr;
        bool oddHashLength = false;

        assert(line.data()[line.size()] == 0);
        size_t pos = 0;
        auto const lineSize = line.size();

        while (pos != lineSize && iswspace(line[pos]))
        {
            pos += 1;
        }

        if (pos == lineSize)
        {
            // Blank line.
            continue;
        }

        if (pos != lineSize &&
            (line[pos] == L';' || line[pos] == L'#'))
        {
            // Comment.
            continue;
        }

        expectedHashValue.clear();
        for (;;)
        {
            unsigned high;
            if (pos == lineSize ||
                0xF < (high = HexCharToValue(line[pos])))
            {
                // Not a valid hexadecimal character. Might be ok.
                break;
            }

            pos += 1;

            unsigned low;
            if (pos == lineSize ||
                0xF < (low = HexCharToValue(line[pos])))
            {
                oddHashLength = true;
                break;
            }

            pos += 1;

            expectedHashValue.push_back(static_cast<BYTE>((high << 4) | low));
        }

        while (pos != lineSize && iswspace(line[pos]))
        {
            pos += 1;
        }

        if (pos != lineSize && line[pos] == L'*')
        {
            pos += 1;
        }

        if (oddHashLength)
        {
            throw HrException(E_INVALIDARG, "Odd checksum length.");
        }
        else if (expectedHashValue.empty())
        {
            throw HrException(E_INVALIDARG, "Missing checksum.");
        }
        else if (pos == lineSize)
        {
            throw HrException(E_INVALIDARG, "Missing filename.");
        }

        fileCount += 1;
        filename = line.data() + pos;

        MappedFile copyFile;
        copyPath.resize(copyPathBaseSize);
        copyPath += filename;
        localHr = MapFile(copyPath.c_str(), &copyFile);
        if (FAILED(localHr))
        {
            LogError(listFileName, lineReader.LineNumber(), "Could not open file '%ls' (HR=0x%X).",
                copyPath.c_str(),
                localHr);
            hr = localHr;
        }

        auto const BlockMax = 0x10000000;
        hasher->Reset();
        for (size_t offset = 0; offset != copyFile.Size;)
        {
            auto const remaining = copyFile.Size - offset;
            unsigned blockSize = remaining > BlockMax ? BlockMax : static_cast<unsigned>(remaining);
            hasher->Append(static_cast<BYTE const*>(copyFile.View.get()) + offset, blockSize);
            offset += blockSize;
        }
        hasher->Finalize(true);
        auto const actualHashValue = hasher->HashBuffer();

        if (ByteSpansEqual(actualHashValue, expectedHashValue))
        {
            // Hashes match.
            continue;
        }

        LogWarning(listFileName, lineReader.LineNumber(), "Checksum mismatch for file '%ls'.",
            copyPath.c_str());
        mismatchCount += 1;

        MappedFile knownGoodFile;
        knownGoodPath.resize(knownGoodPathBaseSize);
        knownGoodPath += filename;
        localHr = MapFile(knownGoodPath.c_str(), &knownGoodFile);
        if (FAILED(localHr))
        {
            LogError(listFileName, lineReader.LineNumber(), "Could not open file '%ls' (HR=0x%X).",
                knownGoodPath.c_str(),
                localHr);
            hr = localHr;
            continue;
        }

        if (knownGoodFile.Size != copyFile.Size)
        {
            LogWarning(listFileName, lineReader.LineNumber(), "Size mismatch for file '%ls' (knownGood %zu bytes, copy %zu bytes).",
                filename,
                knownGoodFile.Size,
                copyFile.Size);
            continue;
        }

        bool foundMismatch = false;
        for (size_t offset = 0; offset != copyFile.Size; offset += 1)
        {
            auto const pKnownGoodByte = &static_cast<BYTE const*>(knownGoodFile.View.get())[offset];
            auto const pCopyByte = &static_cast<BYTE const*>(copyFile.View.get())[offset];
            if (*pKnownGoodByte != *pCopyByte)
            {
                g_pCopyByte = pCopyByte;
                // TODO: Put a breakpoint here and find the physical address of the
                // g_pCopyByte.
                LogWarning(listFileName, lineReader.LineNumber(),
                    "Content mismatch for file '%ls' at offset 0x%zX (%X != %X), user-mode va 0x%zX.",
                    filename,
                    offset,
                    *pKnownGoodByte,
                    *pCopyByte,
                    pCopyByte);
                foundMismatch = true;
            }
        }

        if (!foundMismatch)
        {
            LogWarning(listFileName, lineReader.LineNumber(),
                "Hash mismatch but content matches for file '%ls' ???",
                filename);
        }
    }

    if (mismatchCount != 0)
    {
        LogWarning(listFileName, 0, "%zu of %zu checksums did NOT match.",
            mismatchCount,
            fileCount);

        if (SUCCEEDED(hr))
        {
            hr = S_FALSE;
        }
    }

    if (FAILED(lineReader.Result()))
    {
        hr = lineReader.Result();
    }

    return hr;
}

int
wmain(int argc, _In_count_(argc) PWSTR argv[])
{
    HRESULT hr;

    try
    {
        if (argc != 4)
        {
            fprintf(stderr, "Usage: %ls ListFile.txt KnownGoodDir CopyDir\n", argv[0]);
            hr = 1;
            goto Done;
        }

        auto const hashListFileName = argv[1];
        auto const hashListFile = FopenTextInputWithLogging(hashListFileName);
        if (!hashListFile)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            goto Done;
        }

        hr = CheckListFile(
            hashListFileName,
            hashListFile.get(),
            argv[2],
            argv[3]);
    }
    catch (HrException const& ex)
    {
        LogError("Exception (%hs), HR=0x%X.", ex.what(), ex.hr);
        hr = ex.hr;
    }
    catch (std::exception const& ex)
    {
        LogError("Exception (%hs).", ex.what());
        hr = E_OUTOFMEMORY;
    }
    catch (...)
    {
        LogError("Exception (unknown).");
        hr = E_UNEXPECTED;
    }

Done:

    return hr;
}
