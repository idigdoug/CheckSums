// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "ProgramOptions.h"
#include "HrException.h"
#include "HashCompute.h"
#include "HashCheck.h"
#include "Hasher.h"
#include "Utility.h"

static bool
HandleFileOption(
    int argc,
    _In_count_(argc) PWSTR argv[],
    _Inout_ int* pArgi,
    _Inout_ PCWSTR* pTarget)
{
    if (*pArgi + 1 == argc)
    {
        Log("error : Missing value for option '%ls'.\n", argv[*pArgi]);
        return false;
    }
    else if (argv[*pArgi + 1][0] == 0)
    {
        Log("error : Missing value for option '%ls' (\"\" not allowed).\n", argv[*pArgi]);
        return false;
    }    
    else if (*pTarget)
    {
        Log("error : Invalid argument %ls '%ls', previously specified as '%ls'.\n",
            argv[*pArgi],
            argv[*pArgi + 1],
            *pTarget);
        return false;
    }
    else
    {
        *pTarget = argv[*pArgi + 1];
        *pArgi += 1;
        return true;
    }
}

static void
ShowHelp()
{
    fprintf(stdout, R"(
CheckSums: Compute or check hash values of files.

To compute hashes: CheckSums [-r] <FileSpecs...>

    Writes hash values and file names to output, one per line.
    Each filespec is either a file name or a wildcard pattern (e.g. "*.txt").
    If no FileSpecs are given, hashes stdin.
    FileSpecs may not contain slashes. Use -d to specify a base directory.

    -r             Recursively process files in subdirectories.

To check hashes: CheckSums <options...> -c <ListFiles...>

    Each ListFile should contain lines of the form "<hash> <filename>".
    For each file that doesn't match the expected hash, write the actual hash
    value and file name to output, one per line.

    -s             Don't write results. Status code indicates success/failure.
    -w             Warn about improperly formatted lines in ListFiles.
    -q             Write output only for failures.

Common options:

    -a <algorithm> Use the specified hash algorithm (e.g. -a sha256).
    -d <directory> Set the base directory (default is current directory).
    -o <file>      Write to the specified file instead of stdout.
    -+             Append to the -o file instead of overwriting it.
    -u             UTF-8-BOM encoding for the -o file (default is ANSI).
    -v             Verbose output on stderr.
    --unbuffered   Use unbuffered I/O (not recommended, usually slower).

Hash algorithm options, along with a benchmark time (smaller is faster):

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
        std::vector<PCWSTR> fileNamesVec;
        PCWSTR outputFilename = nullptr;
        HasherInfo const* hasherInfo = nullptr;
        bool showHelp = false;
        bool checkHashes = false;
        bool utf8Output = false;
        bool appendOutput = false;

        for (int argi = 1; argi != argc; argi += 1)
        {
            auto const arg = argv[argi];
            if (StrEqualIgnoreCase(arg, L"--help"))
            {
                showHelp = true;
                goto ArgsDone;
            }
            else if (StrEqualIgnoreCase(arg, L"--unbuffered"))
            {
                options.unbufferedIO = true;
            }
            else if (arg[0] == L'-' || arg[0] == L'/')
            {
                for (auto argPos = &arg[1]; *argPos != 0; argPos += 1)
                {
                    switch (*argPos)
                    {
                    case L'?':
                    case L'h':
                        showHelp = true;
                        goto ArgsDone;
                    case L'c':
                        checkHashes = true;
                        break;
                    case L'a':
                        if (argi + 1 == argc)
                        {
                            Log("error : Missing value for option '-a'.\n");
                            hr = E_INVALIDARG;
                        }
                        else
                        {
                            argi += 1;
                            auto const argHasherInfo = HasherInfoByName(argv[argi]);
                            if (!argHasherInfo)
                            {
                                Log("error : Unknown hash algorithm '%ls'.\n", argv[argi]);
                                hr = E_INVALIDARG;
                            }
                            else if (hasherInfo)
                            {
                                Log("error : Invalid argument '-a %ls': hash algorithm '%ls' already selected.\n",
                                    argv[argi],
                                    hasherInfo->Name);
                                hr = E_INVALIDARG;
                            }
                            else
                            {
                                hasherInfo = argHasherInfo;
                            }
                        }
                        break;
                    case L'o':
                        if (!HandleFileOption(argc, argv, &argi, &outputFilename))
                        {
                            hr = E_INVALIDARG;
                        }
                        break;
                    case L'+':
                        appendOutput = true;
                        break;
                    case L'u':
                        utf8Output = true;
                        break;
                    case L'd':
                        if (!HandleFileOption(argc, argv, &argi, &options.directory))
                        {
                            hr = E_INVALIDARG;
                        }
                        break;
                    case L'v':
                        options.verbose = true;
                        break;
                    case L'r':
                        options.recursive = true;
                        break;
                    case L's':
                        options.silent = true;
                        break;
                    case L'w':
                        options.warn = true;
                        break;
                    case L'q':
                        options.quiet = true;
                        break;
                    }
                }
            }
            else if (arg[0] == 0)
            {
                Log("warning : Ignoring argument \"\".\n");
            }
            else
            {
                fileNamesVec.push_back(arg);
            }
        }

    ArgsDone:
        
        if (showHelp)
        {
            ShowHelp();
            hr = E_ABORT;
            goto Done;
        }

        if (FAILED(hr))
        {
            Log("\nUse '--help' for usage.\n");
            goto Done;
        }

        // Fill in options:

        options.fileNames = fileNamesVec;

        unique_FILE outputFile;
        if (!outputFilename)
        {
            options.output = stdout;
        }
        else
        {
            auto const mode = utf8Output
                ? (appendOutput ? L"a, ccs=UTF-8" : L"w, ccs=UTF-8")
                : (appendOutput ? L"a" : L"w");
            outputFile = FopenWithLogging(outputFilename, mode);
            if (!outputFile)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
                goto Done;
            }

            options.output = outputFile.get();
        }

        auto const hasher = HasherCreateById(hasherInfo ? hasherInfo->Id : HasherId::Default);
        options.pHasher = hasher.get();

        if (!options.directory)
        {
            options.directory = L"";
        }

        // Perform the requested operation:

        if (checkHashes)
        {
            if (options.recursive)
            {
                Log("warning : Ignoring option '-r' (not meaningful when checking hashes).\n");
            }

            hr = HashCheck(options);
        }
        else
        {
            if (options.silent)
            {
                Log("error : Option '-s' is not allowed when computing hashes.\n");
                hr = E_INVALIDARG;
                goto Done;
            }

            if (options.warn)
            {
                Log("warning : Ignoring option '-w' (not meaningful when computing hashes).\n");
            }

            hr = HashCompute(options);
        }
    }
    catch (HrException const& ex)
    {
        Log("error : Exception %hs, HR=0x%X\n", ex.what(), ex.hr);
        hr = ex.hr;
    }
    catch (std::exception const& ex)
    {
        Log("error : Exception %hs\n", ex.what());
        hr = E_OUTOFMEMORY;
    }
    catch (...)
    {
        Log("error : Exception (unknown)\n");
        hr = E_UNEXPECTED;
    }

Done:

    return hr;
}
