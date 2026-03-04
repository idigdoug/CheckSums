// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "FileHasher.h"

#include "Hasher.h"
#include "Utility.h"

static constexpr ULONG FileDataSize = 256 * 1024;

HRESULT
FileHasher::HashFile(_In_z_ PCWSTR fileName, _Inout_ Hasher* pHasher)
{
    Hasher* const hashers[] = { pHasher };
    return HashFile(fileName, hashers);
}

HRESULT
FileHasher::HashFile(_In_z_ PCWSTR fileName, std::span<Hasher* const> hashers)
{
    auto file = CreateFileUnique(
        fileName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN);
    if (!file)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return HashHandle(file.get(), hashers);
}

HRESULT
FileHasher::HashStdin(_Inout_ Hasher* pHasher)
{
    Hasher* const hashers[] = { pHasher };
    return HashStdin(hashers);
}

HRESULT
FileHasher::HashStdin(std::span<Hasher* const> hashers)
{
    auto handle = GetStdHandle(STD_INPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    else
    {
        return HashHandle(handle, hashers);
    }
}

HRESULT
FileHasher::HashHandle(HANDLE file, std::span<Hasher* const> hashers)
{
    m_fileData.resize(FileDataSize);

    // After calling Reset, we must call Finalize before returning.
    // (Exceptions don't count.)
    for (auto& hasher : hashers)
    {
        hasher->Reset();
    }

    HRESULT hr;

    for (;;)
    {
        ULONG pos = 0;
        for (;;)
        {
            ULONG bytesRead = 0;
            if (!ReadFile(file, m_fileData.data() + pos, FileDataSize - pos, &bytesRead, nullptr))
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Done;
            }

            pos += bytesRead;

            if (bytesRead == 0)
            {
                // EOF before filling buffer.
                break;
            }

            if (pos == FileDataSize)
            {
                // Buffer full.
                break;
            }
        }

        for (auto& hasher : hashers)
        {
            hasher->Append(m_fileData.data(), pos);
        }

        if (pos != FileDataSize)
        {
            break;
        }
    }

    hr = S_OK;

Done:

    auto const succeeded = SUCCEEDED(hr);
    for (auto& hasher : hashers)
    {
        hasher->Finalize(succeeded);
    }

    return hr;
}
