// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include <Utility.h>
#include <HrException.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CheckSumsTest
{
    TEST_CLASS(StrEqualIgnoreCaseTests)
    {
    public:

        TEST_METHOD(SameCase)
        {
            Assert::IsTrue(StrEqualIgnoreCase(L"hello", L"hello"));
        }

        TEST_METHOD(DifferentCase)
        {
            Assert::IsTrue(StrEqualIgnoreCase(L"Hello", L"hello"));
            Assert::IsTrue(StrEqualIgnoreCase(L"HELLO", L"hello"));
            Assert::IsTrue(StrEqualIgnoreCase(L"HeLLo", L"hEllO"));
        }

        TEST_METHOD(NotEqual)
        {
            Assert::IsFalse(StrEqualIgnoreCase(L"hello", L"world"));
        }

        TEST_METHOD(DifferentLength)
        {
            Assert::IsFalse(StrEqualIgnoreCase(L"hello", L"hello!"));
            Assert::IsFalse(StrEqualIgnoreCase(L"hello!", L"hello"));
        }

        TEST_METHOD(EmptyStrings)
        {
            Assert::IsTrue(StrEqualIgnoreCase(L"", L""));
        }

        TEST_METHOD(EmptyVsNonEmpty)
        {
            Assert::IsFalse(StrEqualIgnoreCase(L"", L"a"));
            Assert::IsFalse(StrEqualIgnoreCase(L"a", L""));
        }
    };

    TEST_CLASS(ByteSpansEqualTests)
    {
    public:

        TEST_METHOD(EqualSpans)
        {
            BYTE a[] = { 1, 2, 3, 4 };
            BYTE b[] = { 1, 2, 3, 4 };
            Assert::IsTrue(ByteSpansEqual(a, b));
        }

        TEST_METHOD(NotEqualSpans)
        {
            BYTE a[] = { 1, 2, 3, 4 };
            BYTE b[] = { 1, 2, 3, 5 };
            Assert::IsFalse(ByteSpansEqual(a, b));
        }

        TEST_METHOD(DifferentSizeSpans)
        {
            BYTE a[] = { 1, 2, 3 };
            BYTE b[] = { 1, 2, 3, 4 };
            Assert::IsFalse(ByteSpansEqual(a, b));
        }

        TEST_METHOD(EmptySpans)
        {
            Assert::IsTrue(ByteSpansEqual(std::span<BYTE const>(), std::span<BYTE const>()));
        }
    };

    TEST_CLASS(IsEmptyOrEndsWithSlashTests)
    {
    public:

        TEST_METHOD(EmptyString)
        {
            Assert::IsTrue(IsEmptyOrEndsWithSlash(L""));
        }

        TEST_METHOD(EndsWithBackslash)
        {
            Assert::IsTrue(IsEmptyOrEndsWithSlash(L"dir\\"));
        }

        TEST_METHOD(EndsWithForwardSlash)
        {
            Assert::IsTrue(IsEmptyOrEndsWithSlash(L"dir/"));
        }

        TEST_METHOD(DoesNotEndWithSlash)
        {
            Assert::IsFalse(IsEmptyOrEndsWithSlash(L"dir"));
            Assert::IsFalse(IsEmptyOrEndsWithSlash(L"file.txt"));
        }
    };

    TEST_CLASS(EnsureEmptyOrEndsWithSlashTests)
    {
    public:

        TEST_METHOD(EmptyStaysEmpty)
        {
            auto result = EnsureEmptyOrEndsWithSlash(L"");
            Assert::AreEqual(std::wstring(L""), result);
        }

        TEST_METHOD(AlreadyEndsWithBackslash)
        {
            auto result = EnsureEmptyOrEndsWithSlash(L"dir\\");
            Assert::AreEqual(std::wstring(L"dir\\"), result);
        }

        TEST_METHOD(AlreadyEndsWithForwardSlash)
        {
            auto result = EnsureEmptyOrEndsWithSlash(L"dir/");
            Assert::AreEqual(std::wstring(L"dir/"), result);
        }

        TEST_METHOD(AddsBackslash)
        {
            auto result = EnsureEmptyOrEndsWithSlash(L"dir");
            Assert::AreEqual(std::wstring(L"dir\\"), result);
        }
    };

    TEST_CLASS(HrExceptionTests)
    {
    public:

        TEST_METHOD(ConstructionAndProperties)
        {
            HrException ex(E_FAIL, "test error");
            Assert::AreEqual(static_cast<HRESULT>(E_FAIL), ex.hr);
            Assert::AreEqual("test error", ex.what());
        }

        TEST_METHOD(IsStdException)
        {
            HrException ex(E_INVALIDARG, "invalid arg");
            std::exception const& base = ex;
            Assert::AreEqual("invalid arg", base.what());
        }
    };
}
