// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

class Hasher;

struct ProgramOptions
{
    std::span<PCWSTR const> fileNames = {};

    Hasher* pHasher = nullptr;
    PCWSTR directory = nullptr;

    bool verbose = false;
    bool unbuffered = false;
    bool recurse = false; // Compute-only.
    bool ignoreMissing = false; // Check-only.
    bool quiet = false; // Check-only.
    bool strict = false; // Check-only.
    bool warn = false; // Check-only.
};
