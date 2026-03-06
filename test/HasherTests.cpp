// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include <Hasher.h>
#include <HrException.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace CheckSumsTest
{
    // Helper: compute hash of a byte buffer, return hash as vector.
    static std::vector<BYTE>
    ComputeHash(Hasher& hasher, void const* data, unsigned size)
    {
        hasher.Reset();
        hasher.Append(data, size);
        hasher.Finalize(true);
        auto span = hasher.HashBuffer();
        return std::vector<BYTE>(span.begin(), span.end());
    }

    // Helper: compute hash of a string (excluding null terminator).
    static std::vector<BYTE>
    ComputeHashString(Hasher& hasher, char const* str)
    {
        return ComputeHash(hasher, str, static_cast<unsigned>(strlen(str)));
    }

    // Helper: convert hex string to byte vector.
    static std::vector<BYTE>
    HexToBytes(char const* hex)
    {
        std::vector<BYTE> bytes;
        while (*hex)
        {
            unsigned high = 0, low = 0;
            auto ch = *hex++;
            high = (ch >= 'a') ? (ch - 'a' + 10) : (ch - '0');
            ch = *hex++;
            low = (ch >= 'a') ? (ch - 'a' + 10) : (ch - '0');
            bytes.push_back(static_cast<BYTE>((high << 4) | low));
        }
        return bytes;
    }

    static void
    AssertHashEqual(std::vector<BYTE> const& actual, char const* expectedHex, wchar_t const* message = L"")
    {
        auto expected = HexToBytes(expectedHex);
        Assert::AreEqual(expected.size(), actual.size(), message);
        for (size_t i = 0; i < expected.size(); ++i)
        {
            Assert::AreEqual(expected[i], actual[i], message);
        }
    }

    TEST_CLASS(HasherStateTests)
    {
    public:

        TEST_METHOD(InitialStateIsNone)
        {
            auto hasher = HasherCreateCrc32();
            Assert::AreEqual(static_cast<unsigned>(HasherState::None), static_cast<unsigned>(hasher->State()));
        }

        TEST_METHOD(ResetSetsStateToAppending)
        {
            auto hasher = HasherCreateCrc32();
            hasher->Reset();
            Assert::AreEqual(static_cast<unsigned>(HasherState::Appending), static_cast<unsigned>(hasher->State()));
        }

        TEST_METHOD(AppendNonAlignedSetsAppendingComplete)
        {
            auto hasher = HasherCreateCrc32();
            hasher->Reset();
            BYTE data[3] = { 1, 2, 3 };
            hasher->Append(data, 3);
            Assert::AreEqual(static_cast<unsigned>(HasherState::AppendingComplete), static_cast<unsigned>(hasher->State()));
        }

        TEST_METHOD(AppendAlignedKeepsAppending)
        {
            auto hasher = HasherCreateCrc32();
            hasher->Reset();
            BYTE data[Hasher::AppendBlockMultiple] = {};
            hasher->Append(data, Hasher::AppendBlockMultiple);
            Assert::AreEqual(static_cast<unsigned>(HasherState::Appending), static_cast<unsigned>(hasher->State()));
        }

        TEST_METHOD(FinalizeSuccessfulSetsFinalized)
        {
            auto hasher = HasherCreateCrc32();
            hasher->Reset();
            BYTE data[1] = { 0 };
            hasher->Append(data, 1);
            hasher->Finalize(true);
            Assert::AreEqual(static_cast<unsigned>(HasherState::Finalized), static_cast<unsigned>(hasher->State()));
        }

        TEST_METHOD(FinalizeUnsuccessfulSetsNone)
        {
            auto hasher = HasherCreateCrc32();
            hasher->Reset();
            BYTE data[1] = { 0 };
            hasher->Append(data, 1);
            hasher->Finalize(false);
            Assert::AreEqual(static_cast<unsigned>(HasherState::None), static_cast<unsigned>(hasher->State()));
        }

        TEST_METHOD(HashBufferEmptyBeforeFinalize)
        {
            auto hasher = HasherCreateCrc32();
            Assert::IsTrue(hasher->HashBuffer().empty());
            hasher->Reset();
            Assert::IsTrue(hasher->HashBuffer().empty());
        }

        TEST_METHOD(HashBufferAvailableAfterFinalize)
        {
            auto hasher = HasherCreateCrc32();
            hasher->Reset();
            BYTE data[1] = { 0 };
            hasher->Append(data, 1);
            hasher->Finalize(true);
            Assert::IsFalse(hasher->HashBuffer().empty());
            Assert::AreEqual(hasher->HashSize(), static_cast<unsigned>(hasher->HashBuffer().size()));
        }

        TEST_METHOD(ResetFromFinalizedWorks)
        {
            auto hasher = HasherCreateCrc32();
            hasher->Reset();
            BYTE data[1] = { 0 };
            hasher->Append(data, 1);
            hasher->Finalize(true);
            hasher->Reset();
            Assert::AreEqual(static_cast<unsigned>(HasherState::Appending), static_cast<unsigned>(hasher->State()));
        }

        TEST_METHOD(ResetFromAppendingThrows)
        {
            auto hasher = HasherCreateCrc32();
            hasher->Reset();
            Assert::ExpectException<HrException>([&] { hasher->Reset(); });
        }

        TEST_METHOD(AppendFromNoneThrows)
        {
            auto hasher = HasherCreateCrc32();
            BYTE data[1] = { 0 };
            Assert::ExpectException<HrException>([&] { hasher->Append(data, 1); });
        }

        TEST_METHOD(FinalizeFromNoneThrows)
        {
            auto hasher = HasherCreateCrc32();
            Assert::ExpectException<HrException>([&] { hasher->Finalize(true); });
        }
    };

    TEST_CLASS(HasherHashSizeTests)
    {
    public:

        TEST_METHOD(Adler32HashSizeIs4)
        {
            Assert::AreEqual(4u, HasherCreateAdler32()->HashSize());
        }

        TEST_METHOD(Crc32HashSizeIs4)
        {
            Assert::AreEqual(4u, HasherCreateCrc32()->HashSize());
        }

        TEST_METHOD(Fnv1a32HashSizeIs4)
        {
            Assert::AreEqual(4u, HasherCreateFnv1a32()->HashSize());
        }

        TEST_METHOD(Fnv1a64HashSizeIs8)
        {
            Assert::AreEqual(8u, HasherCreateFnv1a64()->HashSize());
        }

        TEST_METHOD(MD4HashSizeIs16)
        {
            Assert::AreEqual(16u, HasherCreateMD4()->HashSize());
        }

        TEST_METHOD(MD5HashSizeIs16)
        {
            Assert::AreEqual(16u, HasherCreateMD5()->HashSize());
        }

        TEST_METHOD(Murmur3HashSizeIs16)
        {
            Assert::AreEqual(16u, HasherCreateMurmur3x64_128()->HashSize());
        }

        TEST_METHOD(SHA1HashSizeIs20)
        {
            Assert::AreEqual(20u, HasherCreateSHA1()->HashSize());
        }

        TEST_METHOD(SHA256HashSizeIs32)
        {
            Assert::AreEqual(32u, HasherCreateSHA256()->HashSize());
        }

        TEST_METHOD(SHA384HashSizeIs48)
        {
            Assert::AreEqual(48u, HasherCreateSHA384()->HashSize());
        }

        TEST_METHOD(SHA512HashSizeIs64)
        {
            Assert::AreEqual(64u, HasherCreateSHA512()->HashSize());
        }

        TEST_METHOD(Xor64HashSizeIs8)
        {
            Assert::AreEqual(8u, HasherCreateXor64()->HashSize());
        }
    };

    TEST_CLASS(HasherNameTests)
    {
    public:

        TEST_METHOD(Adler32Name)
        {
            Assert::AreEqual(L"Adler32", HasherCreateAdler32()->Name());
        }

        TEST_METHOD(Crc32Name)
        {
            Assert::AreEqual(L"Crc32", HasherCreateCrc32()->Name());
        }

        TEST_METHOD(Fnv1a32Name)
        {
            Assert::AreEqual(L"Fnv1a32", HasherCreateFnv1a32()->Name());
        }

        TEST_METHOD(Fnv1a64Name)
        {
            Assert::AreEqual(L"Fnv1a64", HasherCreateFnv1a64()->Name());
        }

        TEST_METHOD(Murmur3Name)
        {
            Assert::AreEqual(L"Murmur3x64_128", HasherCreateMurmur3x64_128()->Name());
        }

        TEST_METHOD(Xor64Name)
        {
            Assert::AreEqual(L"Xor64", HasherCreateXor64()->Name());
        }
    };

    TEST_CLASS(HasherDeterministicTests)
    {
    public:

        TEST_METHOD(SameInputProducesSameHash)
        {
            auto hasher = HasherCreateSHA256();
            auto hash1 = ComputeHashString(*hasher, "test data");
            auto hash2 = ComputeHashString(*hasher, "test data");
            Assert::IsTrue(hash1 == hash2);
        }

        TEST_METHOD(DifferentInputProducesDifferentHash)
        {
            auto hasher = HasherCreateSHA256();
            auto hash1 = ComputeHashString(*hasher, "test data 1");
            auto hash2 = ComputeHashString(*hasher, "test data 2");
            Assert::IsFalse(hash1 == hash2);
        }

        TEST_METHOD(AllHashersDeterministic)
        {
            for (unsigned i = 0; i < static_cast<unsigned>(HasherId::Max); ++i)
            {
                auto hasher = HasherCreateById(static_cast<HasherId>(i));
                auto hash1 = ComputeHashString(*hasher, "deterministic");
                auto hash2 = ComputeHashString(*hasher, "deterministic");
                Assert::IsTrue(hash1 == hash2, hasher->Name());
            }
        }
    };

    TEST_CLASS(HasherKnownVectorTests)
    {
    public:

        // MD5 test vectors (RFC 1321)

        TEST_METHOD(MD5_Empty)
        {
            auto hasher = HasherCreateMD5();
            auto hash = ComputeHashString(*hasher, "");
            AssertHashEqual(hash, "d41d8cd98f00b204e9800998ecf8427e");
        }

        TEST_METHOD(MD5_abc)
        {
            auto hasher = HasherCreateMD5();
            auto hash = ComputeHashString(*hasher, "abc");
            AssertHashEqual(hash, "900150983cd24fb0d6963f7d28e17f72");
        }

        // SHA-1 test vectors (FIPS 180-4)

        TEST_METHOD(SHA1_Empty)
        {
            auto hasher = HasherCreateSHA1();
            auto hash = ComputeHashString(*hasher, "");
            AssertHashEqual(hash, "da39a3ee5e6b4b0d3255bfef95601890afd80709");
        }

        TEST_METHOD(SHA1_abc)
        {
            auto hasher = HasherCreateSHA1();
            auto hash = ComputeHashString(*hasher, "abc");
            AssertHashEqual(hash, "a9993e364706816aba3e25717850c26c9cd0d89d");
        }

        // SHA-256 test vectors (FIPS 180-4)

        TEST_METHOD(SHA256_Empty)
        {
            auto hasher = HasherCreateSHA256();
            auto hash = ComputeHashString(*hasher, "");
            AssertHashEqual(hash, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        }

        TEST_METHOD(SHA256_abc)
        {
            auto hasher = HasherCreateSHA256();
            auto hash = ComputeHashString(*hasher, "abc");
            AssertHashEqual(hash, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        }

        // SHA-384 test vectors (FIPS 180-4)

        TEST_METHOD(SHA384_Empty)
        {
            auto hasher = HasherCreateSHA384();
            auto hash = ComputeHashString(*hasher, "");
            AssertHashEqual(hash, "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b");
        }

        TEST_METHOD(SHA384_abc)
        {
            auto hasher = HasherCreateSHA384();
            auto hash = ComputeHashString(*hasher, "abc");
            AssertHashEqual(hash, "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7");
        }

        // SHA-512 test vectors (FIPS 180-4)

        TEST_METHOD(SHA512_Empty)
        {
            auto hasher = HasherCreateSHA512();
            auto hash = ComputeHashString(*hasher, "");
            AssertHashEqual(hash, "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");
        }

        TEST_METHOD(SHA512_abc)
        {
            auto hasher = HasherCreateSHA512();
            auto hash = ComputeHashString(*hasher, "abc");
            AssertHashEqual(hash, "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
        }

        // CRC32 standard check value: CRC32("123456789") = 0xCBF43926

        TEST_METHOD(Crc32_123456789)
        {
            auto hasher = HasherCreateCrc32();
            auto hash = ComputeHashString(*hasher, "123456789");
            AssertHashEqual(hash, "cbf43926");
        }

        TEST_METHOD(Crc32_Empty)
        {
            auto hasher = HasherCreateCrc32();
            auto hash = ComputeHashString(*hasher, "");
            AssertHashEqual(hash, "00000000");
        }

        // Adler-32: Adler32("Wikipedia") = 0x11E60398

        TEST_METHOD(Adler32_Wikipedia)
        {
            auto hasher = HasherCreateAdler32();
            auto hash = ComputeHashString(*hasher, "Wikipedia");
            AssertHashEqual(hash, "11e60398");
        }

        TEST_METHOD(Adler32_Empty)
        {
            auto hasher = HasherCreateAdler32();
            auto hash = ComputeHashString(*hasher, "");
            AssertHashEqual(hash, "00000001");
        }

        // FNV-1a-32: FNV1a32("") = offset basis 0x811C9DC5 (stored little-endian)

        TEST_METHOD(Fnv1a32_Empty)
        {
            auto hasher = HasherCreateFnv1a32();
            auto hash = ComputeHashString(*hasher, "");
            AssertHashEqual(hash, "c59d1c81");
        }

        // FNV-1a-64: FNV1a64("") = offset basis 0xCBF29CE484222325 (stored little-endian)

        TEST_METHOD(Fnv1a64_Empty)
        {
            auto hasher = HasherCreateFnv1a64();
            auto hash = ComputeHashString(*hasher, "");
            AssertHashEqual(hash, "25232284e49cf2cb");
        }
    };
}
