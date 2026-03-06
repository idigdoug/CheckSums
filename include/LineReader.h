// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

class LineReader
{
    static unsigned constexpr LineSizeIncrement = 256;

    PCWSTR m_filename;
    FILE* m_file;
    size_t m_lineNumber;
    HRESULT m_hr;
    std::vector<wchar_t> m_line;

public:

    LineReader(LineReader const&) = delete;
    LineReader& operator=(LineReader const&) = delete;

    LineReader() noexcept;

    void
    SetFile(_In_ PCWSTR filename, _In_ FILE* file);

    // Returns true if a line was read, false if no more lines.
    // Guarantees that there is a null terminator AFTER the end of the line.
    // When no more lines, use Result() to determine if EOF or error.
    bool
    ReadLine(_Out_ std::wstring_view* pLine);

    size_t
    LineNumber() const noexcept;

    // Valid after ReadLine() returns false. Returns S_OK on EOF, HRESULT otherwise.
    HRESULT
    Result() const noexcept;
};
