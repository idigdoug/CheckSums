// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include <SplitPath.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CheckSumsTest
{
    TEST_CLASS(SplitPathWideTests)
    {
    public:

        TEST_METHOD(EmptyPath)
        {
            SplitPath<wchar_t> sp(L"");
            Assert::AreEqual(std::wstring_view(L""), sp.Path());
            Assert::AreEqual(std::wstring_view(L""), sp.Drive());
            Assert::AreEqual(std::wstring_view(L""), sp.Directory());
            Assert::AreEqual(std::wstring_view(L""), sp.Filename());
            Assert::AreEqual(std::wstring_view(L""), sp.Extension());
        }

        TEST_METHOD(DefaultConstructor)
        {
            SplitPath<wchar_t> sp;
            Assert::AreEqual(std::wstring_view(L""), sp.Path());
            Assert::AreEqual(std::wstring_view(L""), sp.Drive());
            Assert::AreEqual(std::wstring_view(L""), sp.Directory());
            Assert::AreEqual(std::wstring_view(L""), sp.Filename());
            Assert::AreEqual(std::wstring_view(L""), sp.Extension());
        }

        TEST_METHOD(FullPath)
        {
            SplitPath<wchar_t> sp(L"c:\\dir\\subdir\\filename.extension");
            Assert::AreEqual(std::wstring_view(L"c:\\dir\\subdir\\filename.extension"), sp.Path());
            Assert::AreEqual(std::wstring_view(L"c:"), sp.Drive());
            Assert::AreEqual(std::wstring_view(L"\\dir\\subdir\\"), sp.Directory());
            Assert::AreEqual(std::wstring_view(L"filename"), sp.Filename());
            Assert::AreEqual(std::wstring_view(L".extension"), sp.Extension());
        }

        TEST_METHOD(ForwardSlashes)
        {
            SplitPath<wchar_t> sp(L"c:/dir/subdir/filename.ext");
            Assert::AreEqual(std::wstring_view(L"c:"), sp.Drive());
            Assert::AreEqual(std::wstring_view(L"/dir/subdir/"), sp.Directory());
            Assert::AreEqual(std::wstring_view(L"filename"), sp.Filename());
            Assert::AreEqual(std::wstring_view(L".ext"), sp.Extension());
        }

        TEST_METHOD(FilenameOnly)
        {
            SplitPath<wchar_t> sp(L"filename.txt");
            Assert::AreEqual(std::wstring_view(L""), sp.Drive());
            Assert::AreEqual(std::wstring_view(L""), sp.Directory());
            Assert::AreEqual(std::wstring_view(L"filename"), sp.Filename());
            Assert::AreEqual(std::wstring_view(L".txt"), sp.Extension());
        }

        TEST_METHOD(FilenameNoExtension)
        {
            SplitPath<wchar_t> sp(L"filename");
            Assert::AreEqual(std::wstring_view(L""), sp.Drive());
            Assert::AreEqual(std::wstring_view(L""), sp.Directory());
            Assert::AreEqual(std::wstring_view(L"filename"), sp.Filename());
            Assert::AreEqual(std::wstring_view(L""), sp.Extension());
        }

        TEST_METHOD(DriveOnly)
        {
            SplitPath<wchar_t> sp(L"c:");
            Assert::AreEqual(std::wstring_view(L"c:"), sp.Drive());
            Assert::AreEqual(std::wstring_view(L""), sp.Directory());
            Assert::AreEqual(std::wstring_view(L""), sp.Filename());
            Assert::AreEqual(std::wstring_view(L""), sp.Extension());
        }

        TEST_METHOD(DriveAndDirectory)
        {
            SplitPath<wchar_t> sp(L"c:\\dir\\");
            Assert::AreEqual(std::wstring_view(L"c:"), sp.Drive());
            Assert::AreEqual(std::wstring_view(L"\\dir\\"), sp.Directory());
            Assert::AreEqual(std::wstring_view(L""), sp.Filename());
            Assert::AreEqual(std::wstring_view(L""), sp.Extension());
        }

        TEST_METHOD(DriveAndFilename)
        {
            SplitPath<wchar_t> sp(L"c:filename.txt");
            Assert::AreEqual(std::wstring_view(L"c:"), sp.Drive());
            Assert::AreEqual(std::wstring_view(L""), sp.Directory());
            Assert::AreEqual(std::wstring_view(L"filename"), sp.Filename());
            Assert::AreEqual(std::wstring_view(L".txt"), sp.Extension());
        }

        TEST_METHOD(DirectoryOnly)
        {
            SplitPath<wchar_t> sp(L"\\dir\\subdir\\");
            Assert::AreEqual(std::wstring_view(L""), sp.Drive());
            Assert::AreEqual(std::wstring_view(L"\\dir\\subdir\\"), sp.Directory());
            Assert::AreEqual(std::wstring_view(L""), sp.Filename());
            Assert::AreEqual(std::wstring_view(L""), sp.Extension());
        }

        TEST_METHOD(DotFile)
        {
            SplitPath<wchar_t> sp(L".gitignore");
            Assert::AreEqual(std::wstring_view(L""), sp.Drive());
            Assert::AreEqual(std::wstring_view(L""), sp.Directory());
            Assert::AreEqual(std::wstring_view(L""), sp.Filename());
            Assert::AreEqual(std::wstring_view(L".gitignore"), sp.Extension());
        }

        TEST_METHOD(DirectoryWithDotFile)
        {
            SplitPath<wchar_t> sp(L"c:\\dir\\.gitignore");
            Assert::AreEqual(std::wstring_view(L"c:"), sp.Drive());
            Assert::AreEqual(std::wstring_view(L"\\dir\\"), sp.Directory());
            Assert::AreEqual(std::wstring_view(L""), sp.Filename());
            Assert::AreEqual(std::wstring_view(L".gitignore"), sp.Extension());
        }

        TEST_METHOD(MultipleExtensions)
        {
            SplitPath<wchar_t> sp(L"archive.tar.gz");
            Assert::AreEqual(std::wstring_view(L""), sp.Drive());
            Assert::AreEqual(std::wstring_view(L""), sp.Directory());
            Assert::AreEqual(std::wstring_view(L"archive.tar"), sp.Filename());
            Assert::AreEqual(std::wstring_view(L".gz"), sp.Extension());
        }

        TEST_METHOD(UpperCaseDrive)
        {
            SplitPath<wchar_t> sp(L"D:\\folder\\file.dat");
            Assert::AreEqual(std::wstring_view(L"D:"), sp.Drive());
            Assert::AreEqual(std::wstring_view(L"\\folder\\"), sp.Directory());
            Assert::AreEqual(std::wstring_view(L"file"), sp.Filename());
            Assert::AreEqual(std::wstring_view(L".dat"), sp.Extension());
        }

        TEST_METHOD(TrailingDot)
        {
            SplitPath<wchar_t> sp(L"filename.");
            Assert::AreEqual(std::wstring_view(L"filename"), sp.Filename());
            Assert::AreEqual(std::wstring_view(L"."), sp.Extension());
        }

        // Combination accessors

        TEST_METHOD(DriveDirectory)
        {
            SplitPath<wchar_t> sp(L"c:\\dir\\file.txt");
            Assert::AreEqual(std::wstring_view(L"c:\\dir\\"), sp.DriveDirectory());
        }

        TEST_METHOD(DriveDirectoryFilename)
        {
            SplitPath<wchar_t> sp(L"c:\\dir\\file.txt");
            Assert::AreEqual(std::wstring_view(L"c:\\dir\\file"), sp.DriveDirectoryFilename());
        }

        TEST_METHOD(DriveDirectoryFilenameExtension)
        {
            SplitPath<wchar_t> sp(L"c:\\dir\\file.txt");
            Assert::AreEqual(std::wstring_view(L"c:\\dir\\file.txt"), sp.DriveDirectoryFilenameExtension());
        }

        TEST_METHOD(DirectoryFilename)
        {
            SplitPath<wchar_t> sp(L"c:\\dir\\file.txt");
            Assert::AreEqual(std::wstring_view(L"\\dir\\file"), sp.DirectoryFilename());
        }

        TEST_METHOD(DirectoryFilenameExtension)
        {
            SplitPath<wchar_t> sp(L"c:\\dir\\file.txt");
            Assert::AreEqual(std::wstring_view(L"\\dir\\file.txt"), sp.DirectoryFilenameExtension());
        }

        TEST_METHOD(FilenameExtension)
        {
            SplitPath<wchar_t> sp(L"c:\\dir\\file.txt");
            Assert::AreEqual(std::wstring_view(L"file.txt"), sp.FilenameExtension());
        }

        // Concatenation roundtrip: Drive + Directory + Filename + Extension == Path
        TEST_METHOD(ConcatenationRoundtrip)
        {
            auto paths = {
                L"c:\\dir\\subdir\\filename.extension",
                L"D:/a/b/c.txt",
                L"filename.txt",
                L"filename",
                L".gitignore",
                L"c:",
                L"\\dir\\",
                L"",
                L"c:\\dir\\.gitignore",
                L"archive.tar.gz",
            };

            for (auto path : paths)
            {
                SplitPath<wchar_t> sp(path);
                std::wstring rebuilt;
                rebuilt += sp.Drive();
                rebuilt += sp.Directory();
                rebuilt += sp.Filename();
                rebuilt += sp.Extension();
                Assert::AreEqual(std::wstring_view(path), std::wstring_view(rebuilt), path);
            }
        }
    };

    TEST_CLASS(SplitPathCharTests)
    {
    public:

        TEST_METHOD(NarrowFullPath)
        {
            SplitPath<char> sp("c:\\dir\\file.txt");
            Assert::AreEqual(std::string_view("c:"), sp.Drive());
            Assert::AreEqual(std::string_view("\\dir\\"), sp.Directory());
            Assert::AreEqual(std::string_view("file"), sp.Filename());
            Assert::AreEqual(std::string_view(".txt"), sp.Extension());
        }

        TEST_METHOD(NarrowConcatenationRoundtrip)
        {
            SplitPath<char> sp("c:\\dir\\subdir\\filename.extension");
            std::string rebuilt;
            rebuilt += sp.Drive();
            rebuilt += sp.Directory();
            rebuilt += sp.Filename();
            rebuilt += sp.Extension();
            Assert::AreEqual(std::string_view("c:\\dir\\subdir\\filename.extension"), std::string_view(rebuilt));
        }
    };
}
