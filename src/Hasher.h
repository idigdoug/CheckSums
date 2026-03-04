// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

enum class HasherState : unsigned
{
    // After construction.
    None,

    // After Reset.
    Appending,

    // After Append with a size that is not a multiple of Hasher::AppendBlockMultiple.
    AppendingComplete,

    // After Finalize.
    Finalized,
};

class __declspec(novtable) Hasher
{
    unsigned const m_hashSize;
    HasherState m_state;
    std::vector<BYTE> m_hashBuffer;

public:

    static unsigned constexpr AppendBlockMultiple = 256;

    Hasher(Hasher const&) = delete;
    Hasher& operator=(Hasher const&) = delete;

    virtual
    ~Hasher() noexcept;

    // Initial state is None.
    explicit
    Hasher(unsigned hashSize) noexcept;

    virtual PCWSTR
    Name() const noexcept = 0;

    HasherState
    State() const noexcept;

    unsigned
    HashSize() const noexcept;

    // If State == Finalized, returns the hash value. Otherwise, returns an empty span.
    std::span<BYTE const>
    HashBuffer() const noexcept;

    // Requires: State == None or State == Finalized.
    // Sets State to Appending.
    void
    Reset();

    // Requires: State == Appending.
    // If contentSize is not a multiple of Hasher::AppendBlockMultiple, sets State to AppendingComplete.
    void
    Append(_In_reads_bytes_(contentSize) void const* content, unsigned contentSize);

    // Requires: State == Appending or State == AppendingComplete.
    // Sets State to Finalized (successful) or None (!successful).
    void
    Finalize(bool successful);

protected:

    // Caller (Reset) guarantees that State == Finalized.
    virtual void
    ResetImpl() = 0;

    // Caller (Append) guarantees that State == Appending.
    virtual void
    AppendImpl(_In_reads_bytes_(contentSize) void const* content, unsigned contentSize) = 0;

    // Caller (Finalize) guarantees that State == Appending or State == AppendingComplete.
    // Caller (Finalize) guarantees that hashSize == HashSize().
    // Implementation may start with FINALIZE_CHECK_HASHSIZE(hashSize, expectedHashSize),
    // which will verify that hashSize == expectedHashSize and throw if not.
    virtual void
    FinalizeImpl(_Out_writes_bytes_(hashSize) void* hash, unsigned hashSize) = 0;
};

std::unique_ptr<Hasher>
HasherCreateBcrypt(BCRYPT_ALG_HANDLE bcryptAlgHandle);

// Support for well-known hashers:

std::unique_ptr<Hasher>
HasherCreateAdler32();

std::unique_ptr<Hasher>
HasherCreateCrc32();

std::unique_ptr<Hasher>
HasherCreateFnv1a32();

std::unique_ptr<Hasher>
HasherCreateFnv1a64();

std::unique_ptr<Hasher>
HasherCreateMD4();

std::unique_ptr<Hasher>
HasherCreateMD5();

std::unique_ptr<Hasher>
HasherCreateMurmur3x64_128();

std::unique_ptr<Hasher>
HasherCreateSHA1();

std::unique_ptr<Hasher>
HasherCreateSHA256();

std::unique_ptr<Hasher>
HasherCreateSHA384();

std::unique_ptr<Hasher>
HasherCreateSHA512();

std::unique_ptr<Hasher>
HasherCreateXor64();

// Unique IDs for each of the well-known hashers.
enum class HasherId : unsigned
{
    Adler32,
    Crc32,
    Fnv1a32,
    Fnv1a64,
    MD4,
    MD5,
    Murmur3x64_128,
    SHA1,
    SHA256,
    SHA384,
    SHA512,
    Xor64,
    Max,
    Default = Murmur3x64_128,
};

// Information about a hasher.
struct HasherInfo
{
    PCWSTR Name;
    unsigned BenchmarkTime; // Arbitrary relative units (think "seconds needed to hash a large file").
    HasherId Id;
};

// Returns a span of HasherInfo for all well-known hashers.
std::span<HasherInfo const>
HasherInfos() noexcept;

// If name matches a well-known hasher, returns a pointer to the corresponding HasherInfo.
// Otherwise, returns nullptr.
_Ret_opt_ HasherInfo const*
HasherInfoByName(_In_z_ PCWSTR name) noexcept;

// Returns a pointer to the HasherInfo for the Default hasher.
_Ret_ HasherInfo const*
HasherInfoDefault() noexcept;

// Creates a hasher by ID. If id is not recognized, throws an exception.
std::unique_ptr<Hasher>
HasherCreateById(HasherId id);
