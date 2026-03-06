// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include <LineReader.h>
#include <Utility.h>

LineReader::LineReader() noexcept
    : m_filename(L"")
    , m_file(nullptr)
    , m_lineNumber(0)
    , m_hr(S_OK)
    , m_line()
{
}

void
LineReader::SetFile(_In_ PCWSTR  filename, _In_ FILE* file)
{
    m_filename = filename;
    m_file = file;
    m_lineNumber = 0;
    m_hr = S_OK;
}

bool
LineReader::ReadLine(_Out_ std::wstring_view* pLine)
{
    bool lineWasRead;

    m_lineNumber += 1;

    size_t pos = 0;
    for (;;)
    {
        if (m_line.size() - pos < LineSizeIncrement)
        {
            m_line.resize(m_line.size() + LineSizeIncrement);
        }

        _doserrno = 0;
        if (!fgetws(m_line.data() + pos, static_cast<int>(m_line.size() - pos), m_file))
        {
            auto const doserror = _doserrno;
            if (!feof(m_file))
            {
                LogError("Could not read from '%ls' (error %u).",
                    m_filename,
                    doserror);
                m_hr = doserror ? HRESULT_FROM_WIN32(doserror) : E_FAIL;
                pos = 0;
                lineWasRead = false;
                break;
            }
            else if (pos != 0)
            {
                // EOF after reading some characters. Return the last line.
                lineWasRead = true;
                break;
            }
            else
            {
                // EOF before reading any characters. No more lines.
                pos = 0;
                lineWasRead = false;
                break;
            }
        }

        pos += wcslen(m_line.data() + pos);
        if (m_line.at(pos - 1) == L'\n')
        {
            pos -= 1;
            lineWasRead = true;
            break;
        }
    }

    m_line[pos] = 0;
    *pLine = std::wstring_view{ m_line.data(), pos };
    return lineWasRead;
}

size_t
LineReader::LineNumber() const noexcept
{
    return m_lineNumber;
}

HRESULT
LineReader::Result() const noexcept
{
    return m_hr;
}
