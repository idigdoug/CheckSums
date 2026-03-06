// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once
#include "Utility.h"

struct OutputOptions
{
    PCWSTR Out = nullptr;
    bool Status = false;
    bool Binary = false;
    bool Utf8Bom = false;
    bool Append = false;
};

class Output
{
    unique_FILE m_outputFile;
    std::string m_actualHashString;
    std::string m_expectedHashString;
    FILE* m_output;
    bool m_status;
    bool m_binary;

public:

    Output() noexcept;

    // Returns false Write is a no-op, e.g. due to --status or because
    // output has not been successfully opened.
    explicit
    operator bool() const noexcept;

    // True if the --status option was set.
    bool
    StatusCodeOnly() const noexcept;

    HRESULT
    Open(OutputOptions const& options) noexcept;

    void
    WriteHash(
        _In_ PCWSTR fileName,
        std::span<BYTE const> actualHash);

    void
    WriteHashOK(
        _In_ PCWSTR fileName,
        std::span<BYTE const> actualHash);

    void
    WriteHashMismatch(
        _In_opt_ PCWSTR fileName,
        std::span<BYTE const> actualHash,
        std::span<BYTE const> expectedHash);

private:

    PCSTR
    Separator() const noexcept;
};
