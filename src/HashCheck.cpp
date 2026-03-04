// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "HashCheck.h"

#include "Hasher.h"
#include "FileHasher.h"
#include "LineReader.h"
#include "ProgramOptions.h"
#include "Utility.h"

class HashChecker
{
    ProgramOptions const& m_options;
    std::wstring m_path;
    size_t const m_pathBaseSize;
    LineReader m_lineReader;
    FileHasher m_fileHasher;
    std::vector<BYTE> m_expectedHashValue;
    std::string m_expectedHashString;
    std::string m_actualHashString;
    size_t m_fileCount;
    size_t m_mismatchCount;

public:

    explicit
    HashChecker(ProgramOptions const& options);

    HRESULT
    Run();

private:

    HRESULT
    CheckFile(PCWSTR listFileName, FILE* listFile);
};

HashChecker::HashChecker(ProgramOptions const& options)
    : m_options(options)
    , m_path(EnsureEmptyOrEndsWithSlash(m_options.directory))
    , m_pathBaseSize(m_path.size())
    , m_lineReader()
    , m_fileHasher()
    , m_expectedHashValue()
    , m_expectedHashString()
    , m_actualHashString()
    , m_fileCount(0)
    , m_mismatchCount(0)
{
    return;
}

HRESULT
HashChecker::Run()
{
    HRESULT hr = S_OK;
    if (m_options.fileNames.empty())
    {
        m_options.LogVerbose("verbose : No file names specified. Reading list from stdin.\n");
        hr = CheckFile(L"stdin", stdin);
    }
    else
    {
        for (auto const listFileName : m_options.fileNames)
        {
            unique_FILE listFile;
            {
                auto listFileBinary = FopenWithLogging(listFileName, L"rb");
                if (!listFileBinary)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    continue;
                }

                UINT8 bom[3];
                auto cb = fread(bom, 1, sizeof(bom), listFileBinary.get());
                PCWSTR mode;
                if (cb == sizeof(bom) && bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF)
                {
                    mode = L"r, ccs=UTF-8";
                }
                else if (cb >= 2 && bom[0] == 0xFF && bom[1] == 0xFE)
                {
                    mode = L"r, ccs=UTF-16LE";
                }
                else
                {
                    mode = L"r";
                }

                listFile = FopenWithLogging(listFileName, mode);
                if (!listFile)
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                    continue;
                }
            }

            auto fileResult = CheckFile(listFileName, listFile.get());

            // Results could be S_OK, S_FALSE (mismatch), or failure.
            // Keep the most-severe result.
            if (hr == S_OK || FAILED(fileResult))
            {
                hr = fileResult;
            }
        }
    }

    return hr;
}

HRESULT
HashChecker::CheckFile(PCWSTR listFileName, FILE* listFile)
{
    m_fileCount = 0;
    m_mismatchCount = 0;

    HRESULT hr = S_OK;
    m_lineReader.SetFile(listFileName, listFile);

    std::wstring_view line;
    while (m_lineReader.ReadLine(&line))
    {
        std::span<BYTE const> actualHashValue;
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

        // From here on, we either match or we increment the mismatch count.
        m_fileCount += 1;

        m_expectedHashValue.clear();
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

            m_expectedHashValue.push_back(static_cast<BYTE>((high << 4) | low));
        }

        while (pos != lineSize && iswspace(line[pos]))
        {
            pos += 1;
        }

        if (pos != lineSize && line[pos] == L'*')
        {
            pos += 1;
        }

        if (pos == lineSize)
        {
            m_options.LogWarn("%ls(%zu) : error : Invalid format (missing filename).\n",
                listFileName,
                m_lineReader.LineNumber());
            goto Mismatch;
        }

        filename = line.data() + pos;

        m_path.resize(m_pathBaseSize);
        m_path += filename;

        m_options.LogVerbose("verbose : Hashing '%ls'\n", filename);
        if (auto const localHr = m_fileHasher.HashFile(m_path.c_str(), m_options.unbufferedIO, m_options.pHasher);
            FAILED(localHr))
        {
            hr = localHr;
            Log("%ls(%zu) : error : Could not hash file '%ls' (HR=0x%X).\n",
                listFileName,
                m_lineReader.LineNumber(),
                m_path.c_str(),
                hr);
            goto Mismatch;
        }

        actualHashValue = m_options.pHasher->HashBuffer();

        if (oddHashLength)
        {
            m_options.LogWarn("%ls(%zu) : warning : Invalid format (odd hash length).\n",
                listFileName,
                m_lineReader.LineNumber());
            goto Mismatch;
        }
        else if (m_expectedHashValue.empty())
        {
            m_options.LogWarn("%ls(%zu) : warning : Invalid format (missing hash).\n",
                listFileName,
                m_lineReader.LineNumber());
            goto Mismatch;
        }
        else
        {
            auto const matched = ByteSpansEqual(actualHashValue, m_expectedHashValue);
            if (!matched)
            {
                goto Mismatch;
            }

            if (!m_options.silent && !m_options.quiet)
            {
                AssignHexUpper(&m_actualHashString, actualHashValue);
                m_options.Output(L"%hs  %ls\n",
                    m_actualHashString.c_str(),
                    filename);
            }
        }

        continue;

    Mismatch:

        m_mismatchCount += 1;

        if (!m_options.silent)
        {
            if (m_expectedHashValue.empty())
            {
                m_expectedHashString = "???";
            }
            else
            {
                AssignHexUpper(&m_expectedHashString, m_expectedHashValue);
            }

            if (filename == nullptr)
            {
                filename = L"???";
            }

            if (actualHashValue.empty())
            {
                m_actualHashString = "???";
            }
            else
            {
                AssignHexUpper(&m_actualHashString, actualHashValue);
            }

            m_options.Output(L"%hs  %ls* FAILED: expected %hs\n",
                m_actualHashString.c_str(),
                filename,
                m_expectedHashString.c_str());
        }
    }

    if (m_mismatchCount != 0)
    {
        if (!m_options.silent)
        {
            Log("%ls : warning : %zu of %zu checksums did NOT match.\n",
                listFileName,
                m_mismatchCount,
                m_fileCount);
        }

        if (SUCCEEDED(hr))
        {
            hr = S_FALSE;
        }
    }

    if (FAILED(m_lineReader.Result()))
    {
        hr = m_lineReader.Result();
    }

    return hr;
}

HRESULT
HashCheck(ProgramOptions const& options)
{
    return HashChecker(options).Run();
}
