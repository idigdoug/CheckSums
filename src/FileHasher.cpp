// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "FileHasher.h"

#include "Hasher.h"
#include "Utility.h"

static constexpr ULONG FileDataSize = 256 * 1024;

void
FileHasher::VirtualFree_delete::operator()(void* ptr) const noexcept
{
    VirtualFree(ptr, 0, MEM_RELEASE);
}

FileHasher::FileHasher() noexcept
    : m_fileData()
{
    return;
}

HRESULT
FileHasher::HashFile(_In_z_ PCWSTR fileName, bool unbufferedIO, _Inout_ Hasher* pHasher)
{
    Hasher* const hashers[] = { pHasher };
    return HashFile(fileName, unbufferedIO, hashers);
}

HRESULT
FileHasher::HashFile(_In_z_ PCWSTR fileName, bool unbufferedIO, std::span<Hasher* const> hashers)
{
    auto const flagsAndAttributes =
        FILE_ATTRIBUTE_NORMAL |
        FILE_FLAG_SEQUENTIAL_SCAN |
        (unbufferedIO ? FILE_FLAG_NO_BUFFERING : 0);

    auto file = CreateFileUnique(
        fileName,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        flagsAndAttributes);
    if (!file)
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return HashHandle(file.get(), unbufferedIO, hashers);
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
        return HashHandle(handle, false, hashers);
    }
}

HRESULT
FileHasher::HashHandle(HANDLE handle, bool unbufferedIO, std::span<Hasher* const> hashers)
{
    if (!m_fileData)
    {
        // Use VirtualAlloc to get a buffer that is (hopefully) suitably aligned for unbuffered I/O.
        m_fileData.reset(static_cast<BYTE*>(VirtualAlloc(nullptr, FileDataSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)));
        if (!m_fileData)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
    }

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
            if (!ReadFile(handle, m_fileData.get() + pos, FileDataSize - pos, &bytesRead, nullptr))
            {
                auto const lastError = GetLastError();
                if (unbufferedIO && lastError == ERROR_INVALID_PARAMETER)
                {
                    // Assume we're at EOF with unbuffered I/O.
                    break;
                }

                hr = HRESULT_FROM_WIN32(lastError);
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
            hasher->Append(m_fileData.get(), pos);
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
