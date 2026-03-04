// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

class Hasher;

struct ProgramOptions
{
    std::span<PCWSTR const> fileNames = {};
    FILE* output = nullptr;
    Hasher* pHasher = nullptr;
    PCWSTR directory = nullptr;

    bool verbose = false;
    bool unbufferedIO = false;
    bool recursive = false; // Compute-only.
    bool silent = false; // Check-only.
    bool warn = false; // Check-only.
    bool quiet = false; // Check-only.

    // fwprintf(output, format, ...) unless silent.
    void
    Output(_Printf_format_string_ PCWSTR format, ...) const;

    // fprintf(stderr, format, ...) if verbose.
    void
    LogVerbose(_Printf_format_string_ PCSTR format, ...) const;

    // fprintf(stderr, format, ...) if warn.
    void
    LogWarn(_Printf_format_string_ PCSTR format, ...) const;
};
