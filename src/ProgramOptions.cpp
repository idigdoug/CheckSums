// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "ProgramOptions.h"

void
ProgramOptions::Output(_Printf_format_string_ PCWSTR format, ...) const
{
    if (!silent)
    {
        va_list args;
        va_start(args, format);
        vfwprintf(output, format, args);
        va_end(args);
    }
}

void
ProgramOptions::LogVerbose(_Printf_format_string_ PCSTR format, ...) const
{
    if (verbose)
    {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }
}

void
ProgramOptions::LogWarn(_Printf_format_string_ PCSTR format, ...) const
{
    if (warn)
    {
        va_list args;
        va_start(args, format);
        vfprintf(stderr, format, args);
        va_end(args);
    }
}
