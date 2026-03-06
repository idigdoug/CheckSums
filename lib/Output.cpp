// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include <Output.h>

static void
AssignHex(_Inout_ std::string* pDest, _In_ std::span<BYTE const> bytes)
{
    static char const HexDigits[] = "0123456789abcdef";

    pDest->clear();
    pDest->reserve(pDest->size() + bytes.size() * 2);
    for (auto const byte : bytes)
    {
        pDest->push_back(static_cast<wchar_t>(HexDigits[(byte >> 4) & 0xF]));
        pDest->push_back(static_cast<wchar_t>(HexDigits[byte & 0xF]));
    }
}

Output::Output() noexcept
    : m_outputFile()
    , m_output()
    , m_status()
    , m_binary()
{
    return;
}

Output::operator bool() const noexcept
{
    return m_output != nullptr;
}

bool
Output::StatusCodeOnly() const noexcept
{
    return m_status;
}

HRESULT
Output::Open(OutputOptions const& options) noexcept
{
    HRESULT hr;

    m_outputFile.reset();
    m_output = nullptr;
    m_status = options.Status;
    m_binary = options.Binary;

    if (options.Status)
    {
        if (options.Out)
        {
            LogWarning("Ignoring --out \"%ls\" since '--status' is set.",
                options.Out);
        }

        m_output = nullptr;
        hr = S_OK;
    }
    else if (!options.Out)
    {
        if (options.Utf8Bom)
        {
            LogWarning("Ignoring '--utf8bom' since no output file specified.");
        }

        if (options.Append)
        {
            LogWarning("Ignoring '--append' since no output file specified.");
        }

        m_output = stdout;
        hr = S_OK;
    }
    else
    {
        auto const mode = options.Utf8Bom
            ? (options.Append ? L"a, ccs=UTF-8" : L"w, ccs=UTF-8")
            : (options.Append ? L"a" : L"w");
        m_outputFile = FopenWithLogging(options.Out, mode);

        m_output = m_outputFile.get();
        hr = m_output ? S_OK : HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}

void
Output::WriteHash(
    _In_ PCWSTR fileName,
    std::span<BYTE const> actualHash)
{
    if (m_output)
    {
        AssignHex(&m_actualHashString, actualHash);
        fwprintf(m_output, L"%hs%hs%ls\n",
            m_actualHashString.c_str(),
            Separator(),
            fileName);
    }
}

void
Output::WriteHashOK(
    _In_ PCWSTR fileName,
    std::span<BYTE const> actualHash)
{
    if (m_output)
    {
        AssignHex(&m_actualHashString, actualHash);
        fwprintf(m_output, L"%hs%hs%ls* OK\n",
            m_actualHashString.c_str(),
            Separator(),
            fileName);
    }
}

void
Output::WriteHashMismatch(
    _In_opt_ PCWSTR fileName,
    std::span<BYTE const> actualHash,
    std::span<BYTE const> expectedHash)
{
    if (m_output)
    {
        AssignHex(&m_actualHashString, actualHash);
        AssignHex(&m_expectedHashString, expectedHash);
        fwprintf(m_output, L"%hs%hs%ls* FAILED: expected %hs\n",
            m_actualHashString.empty() ? "???" :  m_actualHashString.c_str(),
            Separator(),
            !fileName ? L"???" : fileName,
            m_expectedHashString.empty() ? "???" : m_expectedHashString.c_str());
    }
}

PCSTR
Output::Separator() const noexcept
{
    return m_binary ? " *" : "  ";
}
