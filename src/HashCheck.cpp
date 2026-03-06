// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "HashCheck.h"

#include "Hasher.h"
#include "FileHasher.h"
#include "LineReader.h"
#include "ProgramOptions.h"
#include "Output.h"
#include "Utility.h"

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

class HashChecker
{
    ProgramOptions const& m_options;
    Output& m_output;
    std::wstring m_path;
    size_t const m_pathBaseSize; // The part of m_path that came from a -d option.
    FileHasher m_fileHasher;
    LineReader m_lineReader;
    std::vector<BYTE> m_expectedHashValue;
    HRESULT m_hr;

public:

    explicit
    HashChecker(ProgramOptions const& options, Output& output);

    HRESULT
    Run();

private:

    void
    CheckStdin();

    void
    CheckListFile(PCWSTR listFileName, FILE* listFile);
};

HashChecker::HashChecker(ProgramOptions const& options, Output& output)
    : m_options(options)
    , m_output(output)
    , m_path(EnsureEmptyOrEndsWithSlash(m_options.directory))
    , m_pathBaseSize(m_path.size())
    , m_fileHasher()
    , m_lineReader()
    , m_expectedHashValue()
    , m_hr(S_OK)
{
    return;
}

HRESULT
HashChecker::Run()
{
    m_hr = S_OK;

    if (m_options.fileNames.empty())
    {
        LogVerbose(m_options, "No file names specified. Reading list from standard input.");
        CheckStdin();
    }
    else
    {
        for (auto const listFileName : m_options.fileNames)
        {
            if (0 == wcscmp(listFileName, L"-"))
            {
                CheckStdin();
            }
            else
            {
                unique_FILE listFile = FopenTextInputWithLogging(listFileName);
                if (!listFile)
                {
                    m_hr = HRESULT_FROM_WIN32(GetLastError());
                    assert(FAILED(m_hr));
                    continue;
                }

                CheckListFile(listFileName, listFile.get());
            }
        }
    }

    return m_hr;
}

void
HashChecker::CheckStdin()
{
    CheckListFile(L"standard input", stdin);
}

void
HashChecker::CheckListFile(PCWSTR listFileName, FILE* listFile)
{
    size_t fileCount = 0;
    size_t mismatchCount = 0;

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

        if (oddHashLength)
        {
            LogInvalidFormat(m_options, listFileName, m_lineReader.LineNumber(), "odd checksum length");

            if (m_options.strict)
            {
                m_hr = E_INVALIDARG;
            }
            continue;
        }
        else if (m_expectedHashValue.empty())
        {
            LogInvalidFormat(m_options, listFileName, m_lineReader.LineNumber(), "missing checksum");

            if (m_options.strict)
            {
                m_hr = E_INVALIDARG;
            }
            continue;
        }
        else if (pos == lineSize)
        {
            LogInvalidFormat(m_options, listFileName, m_lineReader.LineNumber(), "missing filename");

            if (m_options.strict)
            {
                m_hr = E_INVALIDARG;
            }
            continue;
        }

        filename = line.data() + pos;

        m_path.resize(m_pathBaseSize);
        m_path += filename;

        fileCount += 1;

        LogVerbose(m_options, "Hashing '%ls'", filename);
        if (auto const localHr = m_fileHasher.HashFile(m_path.c_str(), m_options.unbuffered, m_options.pHasher);
            FAILED(localHr))
        {
            if (m_options.ignoreMissing && localHr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
            {
                fileCount -= 1;
                continue;
            }

            LogError(listFileName, m_lineReader.LineNumber(), "Could not hash file '%ls' (HR=0x%X).",
                m_path.c_str(),
                localHr);

            m_hr = localHr;
            goto Mismatch;
        }

        actualHashValue = m_options.pHasher->HashBuffer();

        if (!ByteSpansEqual(actualHashValue, m_expectedHashValue))
        {
            goto Mismatch;
        }

        if (!m_options.quiet)
        {
            m_output.WriteHashOK(filename, actualHashValue);
        }

        continue;

    Mismatch:

        mismatchCount += 1;
        m_output.WriteHashMismatch(
            filename,
            actualHashValue,
            m_expectedHashValue);
    }

    if (mismatchCount != 0)
    {
        if (!m_output.StatusCodeOnly())
        {
            LogWarning(listFileName, 0, "%zu of %zu checksums did NOT match.",
                mismatchCount,
                fileCount);
        }

        if (SUCCEEDED(m_hr))
        {
            m_hr = S_FALSE;
        }
    }

    if (FAILED(m_lineReader.Result()))
    {
        m_hr = m_lineReader.Result();
    }
}

HRESULT
HashCheck(ProgramOptions const& options, Output& output)
{
    return HashChecker(options, output).Run();
}
