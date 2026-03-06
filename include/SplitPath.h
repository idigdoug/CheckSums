// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

/*
Splits a path (e.g. "c:\dir\subdir\filename.extension" into components:

- Drive (e.g. "c:").
- Directory (e.g. "\dir\subdir\")
- Filename (e.g. "filename")
- Extension (e.g. ".extension")

This accepts any string as input and does not attempt to validate it.
Concatenating the components together will always yield the original string.

This accepts both '/' and '\' as path separators.

Drive: If the path starts with a drive letter followed by a colon, then the drive is
       that portion of the path. Otherwise, the drive is empty. Always either empty or 2
       characters.

Directory: The portion of the path after the drive up to and including the last slash.
           This may be empty if there is no directory. This may or may not start with a slash.
           If non-empty, it will always end with a slash.

Filename: The portion of the path after the last slash up to but not including the last '.'.
          This may be empty if there is no filename or if the filename starts with a '.'.

Extension: The portion of the path starting at the last '.'. This may be empty if there is no
           extension. Always either empty or starts with a '.'.
*/
template<class CH>
class SplitPath
{
    std::basic_string_view<CH> m_path;
    size_t m_directoryBegin;
    size_t m_filenameBegin;
    size_t m_extensionBegin;

public:

    using ch_string_view = std::basic_string_view<CH>;

    constexpr
    SplitPath()
        : m_path()
        , m_directoryBegin(0)
        , m_filenameBegin(0)
        , m_extensionBegin(0)
    {
        return;
    }

    constexpr
    SplitPath(ch_string_view path)
        : m_path(path)
        , m_directoryBegin(0)
        , m_filenameBegin(0)
        , m_extensionBegin(0)
    {
        size_t const endPos = m_path.size();

        if (endPos >= 2 && m_path[1] == L':')
        {
            // Drive letter. Normalize to lowercase for simplicity.
            unsigned const driveLetter = static_cast<unsigned>(m_path[0]) | 32;
            if (driveLetter >= 'a' && driveLetter <= 'z')
            {
                m_directoryBegin = 2;
            }
        }

        // If no dots, there is no extension.

        m_extensionBegin = endPos;

        // Scan backwards for filename and extension.
        size_t curPos = endPos;
        while (curPos != m_directoryBegin)
        {
            curPos -= 1;

            auto const ch = m_path[curPos];
            if (ch == '.')
            {
                // Found the extension.
                m_extensionBegin = curPos;

                // Keep looking for the start of the filename.
                while (curPos != m_directoryBegin)
                {
                    curPos -= 1;
                    auto const ch2 = m_path[curPos];
                    if (ch2 == '\\' || ch2 == '/')
                    {
                        // Found the end of the directory.
                        curPos += 1; // Include the slash in the directory.
                        break;
                    }
                }

                break;
            }

            if (ch == '\\' || ch == '/')
            {
                // Found the end of the directory. No extension.
                curPos += 1; // Include the slash in the directory.
                break;
            }
        }

        m_filenameBegin = curPos;
    }

    // Returns the entire path: drive + directory + filename + extension.
    constexpr ch_string_view
    Path() const noexcept
    {
        return m_path;
    }

    // Returns the drive (no directory, filename, or extension), e.g. "" or "c:" or "D:".
    // Always either 0 characters or 2 characters (drive letter followed by colon).
    constexpr ch_string_view
    Drive() const noexcept
    {
        return m_path.substr(0, m_directoryBegin);
    }

    // Returns the path without "filename.extension", e.g. "" or "c:\" or "D:dir\".
    constexpr ch_string_view
    DriveDirectory() const noexcept
    {
        return m_path.substr(0, m_filenameBegin);
    }

    // Returns the path without the ".extension", e.g. "" or "c:\dir\filename".
    constexpr ch_string_view
    DriveDirectoryFilename() const noexcept
    {
        return m_path.substr(0, m_extensionBegin);
    }

    // Returns the entire path, same as Path().
    constexpr ch_string_view
    DriveDirectoryFilenameExtension() const noexcept
    {
        return m_path;
    }

    // Returns the directory portion of the path, not including the drive.
    // May or may not start with a slash (e.g. path "d:dir\filename.extension").
    // May be empty if there is no directory (e.g. path "d:filename.extension").
    constexpr ch_string_view
    Directory() const noexcept
    {
        return m_path.substr(m_directoryBegin, m_filenameBegin - m_directoryBegin);
    }

    // Returns the path without drive or extension, e.g. "" or "dir\" or "dir\filename".
    constexpr ch_string_view
    DirectoryFilename() const noexcept
    {
        return m_path.substr(m_directoryBegin, m_extensionBegin - m_directoryBegin);
    }

    // Returns the path without drive, e.g. "" or "dir\filename.extension".
    constexpr ch_string_view
    DirectoryFilenameExtension() const noexcept
    {
        return m_path.substr(m_directoryBegin);
    }

    // Returns the filename portion of the path, not including the directory or extension.
    // May be empty if there is no filename or if the filename starts with a '.' (e.g. path "d:\dir\.extension").
    constexpr ch_string_view
    Filename() const noexcept
    {
        return m_path.substr(m_filenameBegin, m_extensionBegin - m_filenameBegin);
    }

    // Returns the path without drive or directory, e.g. "" or "filename.extension".
    constexpr ch_string_view
    FilenameExtension() const noexcept
    {
        return m_path.substr(m_filenameBegin);
    }

    // Returns the extension portion of the path, not including the directory or filename.
    // May be empty if there is no extension (e.g. path "d:\dir\filename").
    // Always either empty or starts with a '.' (e.g. path "d:\dir\filename.").
    constexpr ch_string_view
    Extension() const noexcept
    {
        return m_path.substr(m_extensionBegin);
    }
};
