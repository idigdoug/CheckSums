// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include "HashCompute.h"

#include "Hasher.h"
#include "FileHasher.h"
#include "ProgramOptions.h"
#include "SplitPath.h"
#include "Utility.h"

static void
OutputHash(ProgramOptions const& options, PCSTR hashString, PCWSTR fileName)
{
    options.Output(L"%hs  %ls\n", hashString, fileName);
}

class HashComputer
{
    ProgramOptions const& m_options;
    FileHasher m_fileHasher;
    std::wstring m_path;
    size_t const m_pathBaseSize;

    HRESULT m_hr;
    std::string m_hashString;
    WIN32_FIND_DATAW m_findData;
    std::wstring_view m_filePattern;

public:

    explicit
    HashComputer(ProgramOptions const& options);

    HRESULT
    Run();

private:

    void
    Recurse();
};

HashComputer::HashComputer(ProgramOptions const& options)
    : m_options(options)
    , m_fileHasher()
    , m_path(EnsureEmptyOrEndsWithSlash(options.directory))
    , m_pathBaseSize(m_path.size())
    , m_hr(S_OK)
    , m_hashString()
    , m_findData()
    , m_filePattern()
{
    return;
}

HRESULT
HashComputer::Run()
{
    if (m_options.fileNames.empty())
    {
        m_options.LogVerbose("verbose : No file names specified. Hashing stdin.\n");

        if (m_options.recursive)
        {
            Log("warning : '-r' ignored when hashing stdin.\n");
        }

        auto hr = m_fileHasher.HashStdin(m_options.pHasher);
        if (FAILED(hr))
        {
            Log("error : Could not hash stdin (HR=0x%X).\n", hr);
            return hr;
        }

        AssignHexUpper(&m_hashString, m_options.pHasher->HashBuffer());
        OutputHash(m_options, m_hashString.c_str(), L"-");
    }
    else
    {
        for (auto const fileName : m_options.fileNames)
        {
            SplitPath<wchar_t> path(fileName);

            m_path.resize(m_pathBaseSize);

            auto const driveDirectory = path.DriveDirectory();
            m_path += driveDirectory;
            if (!IsEmptyOrEndsWithSlash(driveDirectory))
            {
                m_path.push_back(L'\\');
            }

            auto const filenameExtension = path.FilenameExtension();
            m_filePattern = filenameExtension.empty() ? std::wstring_view(L"*") : filenameExtension;
            Recurse();
        }
    }

    return m_hr;
}

void
HashComputer::Recurse()
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

            m_options.LogVerbose("verbose : Hashing '%ls'\n", m_path.c_str());
            auto hr = m_fileHasher.HashFile(m_path.c_str(), m_options.pHasher);
            if (FAILED(hr))
            {
                Log("error : Could not hash file '%ls' (HR=0x%X).\n", m_path.c_str(), hr);
                m_hr = hr;
            }
            else
            {
                AssignHexUpper(&m_hashString, m_options.pHasher->HashBuffer());
                OutputHash(m_options, m_hashString.c_str(), m_path.c_str() + m_pathBaseSize);
            }
        } while (FindNextFileW(find.get(), &m_findData));
    }

    if (m_options.recursive)
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
                Recurse();
            } while (FindNextFileW(find.get(), &m_findData));
        }
    }

    m_path.resize(pathSize);
}

HRESULT
HashCompute(ProgramOptions const& options)
{
    return HashComputer(options).Run();
}
