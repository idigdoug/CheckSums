// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "ProgramOptions.h"
#include "Output.h"
#include "HrException.h"
#include "HashCompute.h"
#include "HashCheck.h"
#include "Hasher.h"
#include "Utility.h"
#include "Version.h"

static bool
HandleFileOption(
    PCSTR option,
    int argc,
    _In_count_(argc) PWSTR argv[],
    _Inout_ int* pArgi,
    _Inout_ PCWSTR* pTarget)
{
    auto const argIndex = *pArgi;

    if (argIndex + 1 == argc)
    {
        LogError("Missing value for option %ls.",
            option);
        return false;
    }
    
    auto const value = argv[argIndex + 1];
    *pArgi = argIndex + 1; // Consume value.

    if (value[0] == 0)
    {
        LogError("Missing value for option %ls (\"\" not allowed).",
            option);
        return false;
    }    
    else if (*pTarget)
    {
        LogError("Repeated option %ls \"%ls\" (originally \"%ls\").",
            option,
            value,
            *pTarget);
        return false;
    }
    else
    {
        *pTarget = value;
        return true;
    }
}

static void
WarnInvalidOptionForCheck(bool value, PCSTR option)
{
    if (value)
    {
        LogWarning("Ignoring option '%hs' (not meaningful when checking).", option);
    }
}


static void
WarnInvalidOptionForCompute(bool value, PCSTR option)
{
    if (value)
    {
        LogWarning("Ignoring option '%hs' (not meaningful when computing).", option);
    }
}

static bool
ErrorInvalidOptionForCompute(bool value, PCSTR option)
{
    if (value)
    {
        LogWarning("Option '%hs' is not allowed when computing.", option);
        return false;
    }

    return true;
}

static void
ShowHelp()
{
/*
Options are based on those of the md5sum, sha1sum, etc. utilities.

Not implemented:

  --tag
  -z, --zero

Common:

  -b, --binary
  -t, --text
  --help
  --version

Check-only:

  -c, --check
  --ignore-missing
  --quiet
  --status
  --strict
  -w, --warn

Enhancements:

  -r, --recurse (compute-only)
  -a <algorithm>
  -d <directory>
  -o, --out <file>
  --append
  --utf8bom
  --verbose
  --unbuffered
*/

    fprintf(stdout, R"(
CheckSums: Compute or check checksums of files.

Computing checksums:

  CheckSums <common and compute options...> <FileSpecs...>

  Writes checksum values and file names to output.

  Each FileSpec is either a file name or a wildcard pattern (e.g. "*.txt").

  If no FileSpecs are given or if FileSpec is "-", reads standard input.

  If the -r or --recurse option is given, FileSpecs will be evaluated
  recursively with behavior like "dir /s /b FileSpec":

  - If FileSpec has no slashes, the base directory (see the -d option) will be
    the search starting point. Otherwise, the base directory will be combined
    with the part of FileSpec before the last slash to form the search
    starting point.
  - The rest of FileSpec (the part after the last slash, or all of FileSpec if
    it has no slashes) will be used as the search pattern. It may contain
    wildcards '*' and '?'.
  - The search pattern will be evaluated in every directory at or below the
    search starting point.

  For example "CheckSums -d C:\Data -r Files\*.txt" will compute checksums for
  all ".txt" files in "C:\Data\Files" and its subdirectories.

Compute options:

  -r, --recurse    Recursively process files in subdirectories.

Checking checksums:

  CheckSums -c      <common and check options...> <ListFiles...>
  CheckSums --check <common and check options...> <ListFiles...>

  If no ListFiles are given or if ListFile is "-", reads standard input.

  Each ListFile should contain lines of the form "<checksum>  <filename>" or
  "<checksum> *<filename>".

Check options:

  --ignore-missing Don't fail or report status for missing files.
  --quiet          Don't write output for successfully-verified files.
  --status         Don't write any output. Status code indicates success.
  --strict         Error for improperly-formatted lines in ListFiles.
  -w, --warn       Warn for improperly-formatted lines in ListFiles.
 
Common options:

  -a <algorithm>   Use the specified checksum algorithm (e.g. -a sha256).
  -d <directory>   Set the base directory (default is current directory).
  -o, --out <file> Write output to the specified file instead of stdout.
  -b, --binary     Use " *" instead of "  " as the separator between checksum
                   and file name in output.
  -t, --text       Use "  " instead of " *" as the separator between checksum
                   and file name in output (default).
  --append         Append to the -o file instead of overwriting it.
  --utf8bom        Use UTF-8-BOM encoding for the -o file (default is ANSI).
  --unbuffered     Read file data using unbuffered I/O (usually slower).
  --verbose        Verbose logging on stderr.
  --version        Show version information and exit.
  -h, -?, --help   Show this help message and exit.

  The -d directory is used when locating files but is not included in the
  output. For example, "CheckSums -d C:\Data Files\Test.txt" would read from
  "C:\Data\Files\Test.txt" but the output would be "<SUM>  Files\Test.txt".

  The -b and -t options only affect the output format. Files to be checksummed
  are always read as binary (no newline normalization).

Checksum algorithm options, along with a benchmark time (smaller is faster):

)");

    auto hasherInfoDefault = *HasherInfoDefault();
    for (auto& current : HasherInfos())
    {
        fprintf(stdout, "    -a %-16ls %3u%hs\n",
            current.Name,
            current.BenchmarkTime,
            current.Id == hasherInfoDefault.Id ? " (default)" : "");
    }
}

int __cdecl
wmain(int argc, _In_count_(argc) PWSTR argv[])
{
    HRESULT hr = S_OK;

    try
    {
        ProgramOptions options;
        OutputOptions outputOptions;
        bool optionCheck = false;
        HasherInfo const* optionAlgorithm = nullptr;

        bool optionVersion = false;
        bool optionHelp = false;
        std::vector<PCWSTR> optionFiles;

        for (int argi = 1; argi != argc; argi += 1)
        {
            auto const arg = argv[argi];
            if (arg[0] == L'-' && arg[1] == L'-')
            {
                // "--flag"

                auto const flag = &arg[2];
                if (StrEqualIgnoreCase(flag, L"binary"))
                {
                    outputOptions.Binary = true;
                }
                else if (StrEqualIgnoreCase(flag, L"text"))
                {
                    outputOptions.Binary = false;
                }
                else if (StrEqualIgnoreCase(flag, L"recurse"))
                {
                    options.recurse = true;
                }
                else if (StrEqualIgnoreCase(flag, L"check"))
                {
                    optionCheck = true;
                }
                else if (StrEqualIgnoreCase(flag, L"ignore-missing"))
                {
                    options.ignoreMissing = true;
                }
                else if (StrEqualIgnoreCase(flag, L"quiet"))
                {
                    options.quiet = true;
                }
                else if (StrEqualIgnoreCase(flag, L"status"))
                {
                    outputOptions.Status = true;
                }
                else if (StrEqualIgnoreCase(flag, L"strict"))
                {
                    options.strict = true;
                }
                else if (StrEqualIgnoreCase(flag, L"warn"))
                {
                    options.warn = true;
                }
                else if (StrEqualIgnoreCase(flag, L"out"))
                {
                    if (!HandleFileOption("--out", argc, argv, &argi, &outputOptions.Out))
                    {
                        hr = E_INVALIDARG;
                    }
                }
                else if (StrEqualIgnoreCase(flag, L"append"))
                {
                    outputOptions.Append = true;
                }
                else if (StrEqualIgnoreCase(flag, L"utf8bom"))
                {
                    outputOptions.Utf8Bom = true;
                }
                else if (StrEqualIgnoreCase(flag, L"unbuffered"))
                {
                    options.unbuffered = true;
                }
                else if (StrEqualIgnoreCase(flag, L"verbose"))
                {
                    options.verbose = true;
                }
                else if (StrEqualIgnoreCase(flag, L"version"))
                {
                    optionVersion = true;
                }
                else if (StrEqualIgnoreCase(flag, L"help"))
                {
                    optionHelp = true;
                }
                else
                {
                    LogError("Unknown option \"%ls\".", arg);
                    hr = E_INVALIDARG;
                }
            }
            else if (
                (arg[0] == L'/') ||
                (arg[0] == L'-' && arg[1] != 0))
            {
                // "-abc" or "/abc", but not "-"
                for (auto argPos = &arg[1]; *argPos != 0; argPos += 1)
                {
                    switch (*argPos)
                    {
                    case L'b':
                        outputOptions.Binary = true;
                        break;
                    case L't':
                        outputOptions.Binary = false;
                        break;
                    case L'r':
                        options.recurse = true;
                        break;
                    case L'c':
                        optionCheck = true;
                        break;
                    case L'w':
                        options.warn = true;
                        break;
                    case L'a':
                        if (argi + 1 == argc)
                        {
                            LogError("Missing value for option '-a'.");
                            hr = E_INVALIDARG;
                        }
                        else
                        {
                            argi += 1;
                            auto const argHasherInfo = HasherInfoByName(argv[argi]);
                            if (!argHasherInfo)
                            {
                                LogError("Unknown hash algorithm '%ls'.", argv[argi]);
                                hr = E_INVALIDARG;
                            }
                            else if (optionAlgorithm)
                            {
                                LogError("Invalid argument '-a %ls': algorithm '%ls' already selected.",
                                    argv[argi],
                                    optionAlgorithm->Name);
                                hr = E_INVALIDARG;
                            }
                            else
                            {
                                optionAlgorithm = argHasherInfo;
                            }
                        }
                        break;
                    case L'd':
                        if (!HandleFileOption("-d", argc, argv, &argi, &options.directory))
                        {
                            hr = E_INVALIDARG;
                        }
                        break;
                    case L'o':
                        if (!HandleFileOption("-o", argc, argv, &argi, &outputOptions.Out))
                        {
                            hr = E_INVALIDARG;
                        }
                        break;
                    case L'h':
                    case L'?':
                        optionHelp = true;
                        break;
                    default:
                        LogError("Unknown option '-%lc'.", *argPos);
                        hr = E_INVALIDARG;
                        break;
                    }
                }
            }
            else if (arg[0] == 0)
            {
                // ""
                LogWarning("Ignoring argument \"\".");
            }
            else
            {
                // "filename" or "-"
                optionFiles.push_back(arg);
            }
        }

        if (optionHelp)
        {
            ShowHelp();
            hr = E_ABORT;
            goto Done;
        }

        if (optionVersion)
        {
            fprintf(stdout, "CheckSums version %u.%u, build date %hs\n",
                VERSION_MAJOR,
                VERSION_MINOR,
                __DATE__);
            hr = E_ABORT;
            goto Done;
        }

        if (FAILED(hr))
        {
            LogRaw("\nUse '--help' for usage.\n");
            goto Done;
        }

        Output output;
        hr = output.Open(outputOptions);
        if (FAILED(hr))
        {
            goto Done;
        }

        // Fill in options:

        options.fileNames = optionFiles;

        auto const hasher = HasherCreateById(optionAlgorithm ? optionAlgorithm->Id : HasherId::Default);
        options.pHasher = hasher.get();

        if (!options.directory)
        {
            options.directory = L"";
        }

        // Perform the requested operation:

        if (optionCheck)
        {
            // Check mode:

            WarnInvalidOptionForCheck(options.recurse, "--recurse");

            hr = HashCheck(options, output);
        }
        else
        {
            // Compute mode:

            WarnInvalidOptionForCompute(options.ignoreMissing, "--ignore-missing");
            WarnInvalidOptionForCompute(options.quiet, "--quiet");
            WarnInvalidOptionForCompute(options.strict, "--strict");
            WarnInvalidOptionForCompute(options.warn, "--warn");

            if (!ErrorInvalidOptionForCompute(outputOptions.Status, "--status"))
            {
                hr = E_INVALIDARG;
                goto Done;
            }

            hr = HashCompute(options, output);
        }
    }
    catch (HrException const& ex)
    {
        LogError("Exception (%hs), HR=0x%X.", ex.what(), ex.hr);
        hr = ex.hr;
    }
    catch (std::exception const& ex)
    {
        LogError("Exception (%hs).", ex.what());
        hr = E_OUTOFMEMORY;
    }
    catch (...)
    {
        LogError("Exception (unknown).");
        hr = E_UNEXPECTED;
    }

Done:

    return hr;
}
