// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "HashCompute.h"

#include "Hasher.h"
#include "FileHasher.h"
#include "ProgramOptions.h"
#include "Output.h"
#include "SplitPath.h"
#include "Utility.h"

class HashComputer
{
    ProgramOptions const& m_options;
    Output& m_output;
    std::wstring m_path;
    size_t const m_pathBaseSize; // The part of m_path that came from a -d option.
    FileHasher m_fileHasher;
    WIN32_FIND_DATAW m_findData;
    std::wstring_view m_filePattern;
    HRESULT m_hr;

public:

    explicit
    HashComputer(ProgramOptions const& options, Output& output);

    HRESULT
    Run();

private:

    void
    ComputeStdin();

    // Recursive based on m_path and m_filePattern.
    void
    ComputeRecurse();
};

HashComputer::HashComputer(ProgramOptions const& options, Output& output)
    : m_options(options)
    , m_output(output)
    , m_path(EnsureEmptyOrEndsWithSlash(options.directory))
    , m_pathBaseSize(m_path.size())
    , m_fileHasher()
    , m_findData()
    , m_filePattern()
    , m_hr(S_OK)
{
    return;
}

HRESULT
HashComputer::Run()
{
    m_hr = S_OK;

    if (m_options.fileNames.empty())
    {
        LogVerbose(m_options, "No file names specified. Reading data from standard input.");
        ComputeStdin();
    }
    else
    {
        for (auto const dataFileName : m_options.fileNames)
        {
            if (0 == wcscmp(dataFileName, L"-"))
            {
                ComputeStdin();
            }
            else
            {
                SplitPath<wchar_t> path(dataFileName);

                m_path.resize(m_pathBaseSize);

                auto const driveDirectory = path.DriveDirectory();
                m_path += driveDirectory;
                if (!IsEmptyOrEndsWithSlash(driveDirectory))
                {
                    m_path.push_back(L'\\');
                }

                auto const filenameExtension = path.FilenameExtension();
                m_filePattern = filenameExtension.empty() ? std::wstring_view(L"*") : filenameExtension;
                ComputeRecurse();
            }
        }
    }

    return m_hr;
}

void
HashComputer::ComputeStdin()
{
    auto hr = m_fileHasher.HashStdin(m_options.pHasher);
    if (FAILED(hr))
    {
        LogError("Could not read 'standard input' (HR=0x%X).", hr);
        m_hr = hr;
    }
    else
    {
        m_output.WriteHash(L"-", m_options.pHasher->HashBuffer());
    }
}

void
HashComputer::ComputeRecurse()
{
    assert(m_path.empty() || m_path.back() == L'\\' || m_path.back() == L'/');

    auto const pathSize = m_path.size();

    m_path.resize(pathSize);
    m_path += m_filePattern;
    if (auto find = FindFirstFileUnique(m_path.c_str(), &m_findData, FindExSearchNameMatch);
        find)
    {
        do
        {
            if (0 != (m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                0 != (m_findData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE) ||
                0 != (m_findData.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE))
            {
                continue;
            }

            m_path.resize(pathSize);
            m_path += m_findData.cFileName;

            LogVerbose(m_options, "Hashing '%ls'", m_path.c_str());
            auto hr = m_fileHasher.HashFile(m_path.c_str(), m_options.unbuffered, m_options.pHasher);
            if (FAILED(hr))
            {
                LogError("Could not hash file '%ls' (HR=0x%X).", m_path.c_str(), hr);
                m_hr = hr;
            }
            else
            {
                m_output.WriteHash(m_path.c_str() + m_pathBaseSize, m_options.pHasher->HashBuffer());
            }
        } while (FindNextFileW(find.get(), &m_findData));
    }

    if (m_options.recurse)
    {
        m_path.resize(pathSize);
        m_path += L'*';
        if (auto find = FindFirstFileUnique(m_path.c_str(), &m_findData, FindExSearchLimitToDirectories);
            find)
        {
            do
            {
                if (0 == (m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                {
                    continue;
                }

                auto const filename = m_findData.cFileName;
                if (filename[0] == L'.' && (filename[1] == 0 || (filename[1] == L'.' && filename[2] == 0)))
                {
                    continue;
                }

                m_path.resize(pathSize);
                m_path += filename;
                m_path += L'\\';
                ComputeRecurse();
            } while (FindNextFileW(find.get(), &m_findData));
        }
    }

    m_path.resize(pathSize);
}

HRESULT
HashCompute(ProgramOptions const& options, Output& output)
{
    return HashComputer(options, output).Run();
}
