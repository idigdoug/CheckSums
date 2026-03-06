// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include <Hasher.h>
#include <HrException.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CheckSumsTest
{
    TEST_CLASS(HasherInfosTests)
    {
    public:

        TEST_METHOD(HasherInfosContainsAllHashers)
        {
            auto infos = HasherInfos();
            Assert::AreEqual(static_cast<size_t>(HasherId::Max), infos.size());
        }

        TEST_METHOD(HasherInfosIdsMatch)
        {
            auto infos = HasherInfos();
            for (unsigned i = 0; i < infos.size(); ++i)
            {
                Assert::AreEqual(i, static_cast<unsigned>(infos[i].Id));
            }
        }

        TEST_METHOD(HasherInfosNamesNonNull)
        {
            auto infos = HasherInfos();
            for (auto const& info : infos)
            {
                Assert::IsNotNull(info.Name);
                Assert::AreNotEqual(L"", info.Name);
            }
        }
    };

    TEST_CLASS(HasherInfoByNameTests)
    {
    public:

        TEST_METHOD(FindByExactName)
        {
            auto info = HasherInfoByName(L"SHA256");
            Assert::IsNotNull(info);
            Assert::AreEqual(static_cast<unsigned>(HasherId::SHA256), static_cast<unsigned>(info->Id));
        }

        TEST_METHOD(FindByNameCaseInsensitive)
        {
            auto info = HasherInfoByName(L"sha256");
            Assert::IsNotNull(info);
            Assert::AreEqual(static_cast<unsigned>(HasherId::SHA256), static_cast<unsigned>(info->Id));
        }

        TEST_METHOD(AllHashersFindable)
        {
            auto infos = HasherInfos();
            for (auto const& info : infos)
            {
                auto found = HasherInfoByName(info.Name);
                Assert::IsNotNull(found, info.Name);
                Assert::AreEqual(static_cast<unsigned>(info.Id), static_cast<unsigned>(found->Id));
            }
        }

        TEST_METHOD(UnknownNameReturnsNull)
        {
            Assert::IsNull(HasherInfoByName(L"NonExistent"));
        }

        TEST_METHOD(EmptyNameReturnsNull)
        {
            Assert::IsNull(HasherInfoByName(L""));
        }
    };

    TEST_CLASS(HasherInfoDefaultTests)
    {
    public:

        TEST_METHOD(DefaultIsNotNull)
        {
            Assert::IsNotNull(HasherInfoDefault());
        }

        TEST_METHOD(DefaultIsMurmur3)
        {
            auto info = HasherInfoDefault();
            Assert::AreEqual(static_cast<unsigned>(HasherId::Murmur3x64_128), static_cast<unsigned>(info->Id));
        }
    };

    TEST_CLASS(HasherCreateByIdTests)
    {
    public:

        TEST_METHOD(CreateAllById)
        {
            for (unsigned i = 0; i < static_cast<unsigned>(HasherId::Max); ++i)
            {
                auto hasher = HasherCreateById(static_cast<HasherId>(i));
                Assert::IsNotNull(hasher.get());
            }
        }

        TEST_METHOD(CreateByIdMatchesInfo)
        {
            auto infos = HasherInfos();
            for (auto const& info : infos)
            {
                auto hasher = HasherCreateById(info.Id);
                Assert::AreEqual(info.Name, hasher->Name());
            }
        }

        TEST_METHOD(CreateInvalidIdThrows)
        {
            Assert::ExpectException<HrException>([&]
            {
                HasherCreateById(HasherId::Max);
            });
        }
    };
}
