// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#include "pch.h"
#include <Hasher.h>

#include <HrException.h>
#include <Utility.h>

#define FINALIZE_CHECK_HASHSIZE(hashSize, expectedHashSize) ( \
    (hashSize) == (expectedHashSize) \
    ? (void)0 \
    : ThrowHrException(E_FAIL, "Finalize(): unexpected state (wrong hash size).") \
    )

static constexpr HasherInfo const g_hasherInfos[] =
{
    { L"Adler32",        131,   HasherId::Adler32 },
    { L"Crc32",          779,   HasherId::Crc32 },
    { L"Fnv1a32",        442,   HasherId::Fnv1a32 },
    { L"Fnv1a64",        425,   HasherId::Fnv1a64 },
    { L"MD4",            450,   HasherId::MD4 },
    { L"MD5",            759,   HasherId::MD5 },
    { L"Murmur3x64_128",  68,   HasherId::Murmur3x64_128 },
    { L"SHA1",           575,   HasherId::SHA1   },
    { L"SHA256",         232,   HasherId::SHA256 },
    { L"SHA384",         695,   HasherId::SHA384 },
    { L"SHA512",         685,   HasherId::SHA512 },
    { L"Xor64",           23,   HasherId::Xor64 },
};

static_assert(
    static_cast<unsigned>(HasherId::Max) == ARRAYSIZE(g_hasherInfos),
    "g_hasherInfos must have an entry for each HasherId");

static_assert(
    HasherId::Default == g_hasherInfos[static_cast<unsigned>(HasherId::Default)].Id,
    "HasherId::Default == g_hasherInfos[HasherId::Default].Id");

[[noreturn]] static void
ThrowHrException(HRESULT hr, PCSTR what)
{
    throw HrException(hr, what);
}

//////////////////////////////////////////////////////////////////
// Hasher

Hasher::~Hasher()
{
    return;
}

Hasher::Hasher(unsigned hashSize) noexcept
    : m_hashSize(hashSize)
    , m_state(HasherState::None)
    , m_hashBuffer()
{
    return;
}

HasherState
Hasher::State() const noexcept
{
    return m_state;
}

unsigned
Hasher::HashSize() const noexcept
{
    return m_hashSize;
}

std::span<BYTE const>
Hasher::HashBuffer() const noexcept
{
    return m_state == HasherState::Finalized
        ? std::span<BYTE const>(m_hashBuffer)
        : std::span<BYTE const>();
}

void
Hasher::Reset()
{
    if (m_state != HasherState::None &&
        m_state != HasherState::Finalized)
    {
        ThrowHrException(
            E_NOT_VALID_STATE,
            "Reset() requires State == None or State == Finalized.");
    }

    m_hashBuffer.resize(m_hashSize);
    ResetImpl();
    m_state = HasherState::Appending;
}

void
Hasher::Append(_In_reads_bytes_(contentSize) void const* content, unsigned contentSize)
{
    if (m_state != HasherState::Appending)
    {
        ThrowHrException(
            E_NOT_VALID_STATE,
            "Append() requires State == Appending.");
    }

    AppendImpl(content, contentSize);

    // All but the last block must have a size that is a multiple of Hasher::AppendBlockMultiple.
    // If contentSize is not a multiple of Hasher::AppendBlockMultiple, this must be the last block.
    if (contentSize & (Hasher::AppendBlockMultiple - 1))
    {
        m_state = HasherState::AppendingComplete;
    }
}

void
Hasher::Finalize(bool successful)
{
    if (m_state != HasherState::Appending &&
        m_state != HasherState::AppendingComplete)
    {
        ThrowHrException(
            E_NOT_VALID_STATE,
            "Finalize() requires State == Appending or State == AppendingComplete.");
    }

    assert(m_hashBuffer.size() == m_hashSize);
    FinalizeImpl(m_hashBuffer.data(), static_cast<unsigned>(m_hashBuffer.size()));
    m_state = successful ? HasherState::Finalized : HasherState::None;
}

//////////////////////////////////////////////////////////////////
// Xor64Hasher

class Xor64Hasher final : public Hasher
{
    using ValueType = UINT64;
    ValueType m_hashState = 0;

public:

    Xor64Hasher() noexcept
        : Hasher(sizeof(m_hashState))
    {
        return;
    }

    PCWSTR
    Name() const noexcept override
    {
        return L"Xor64";
    }

private:

    void
    ResetImpl() override
    {
        m_hashState = 0;
    }

    void
    AppendImpl(_In_reads_bytes_(contentSize) void const* content, unsigned contentSize) override
    {
        auto pVals = static_cast<ValueType const*>(content);
        auto const pValsEnd = pVals + (contentSize / sizeof(*pVals));
        while (pVals != pValsEnd)
        {
            m_hashState ^= *pVals;
            pVals += 1;
        }

        // Any remaining bytes that don't fit into a full ValueType are treated as if they were
        // padded with zeros to form a full ValueType.
        ValueType tail = 0;
        memcpy(&tail, pVals, contentSize % sizeof(*pVals));
        m_hashState ^= tail;
    }

    void
    FinalizeImpl(_Out_writes_bytes_(hashSize) void* hash, unsigned hashSize) override
    {
        FINALIZE_CHECK_HASHSIZE(hashSize, sizeof(m_hashState));
        memcpy(hash, &m_hashState, sizeof(m_hashState));
    }
};

//////////////////////////////////////////////////////////////////
// Adler32Hasher

class Adler32Hasher final : public Hasher
{
    static auto constexpr MyHashSize = 4;
    UINT16 m_s1 = 0;
    UINT16 m_s2 = 0;

public:

    Adler32Hasher() noexcept
        : Hasher(MyHashSize)
    {
        return;
    }

    PCWSTR
    Name() const noexcept override
    {
        return L"Adler32";
    }

private:

    void
    ResetImpl() override
    {
        m_s1 = 1;
        m_s2 = 0;
    }

    // Unroll 16 update steps.
#define ADLER32_DO1(buf,i)  {s1 += buf[i]; s2 += s1;}
#define ADLER32_DO2(buf,i)  ADLER32_DO1(buf,i); ADLER32_DO1(buf,(i)+1);
#define ADLER32_DO4(buf,i)  ADLER32_DO2(buf,i); ADLER32_DO2(buf,(i)+2);
#define ADLER32_DO8(buf,i)  ADLER32_DO4(buf,i); ADLER32_DO4(buf,(i)+4);
#define ADLER32_DO16(buf)   ADLER32_DO8(buf,0); ADLER32_DO8(buf,8);

    void
    AppendImpl(_In_reads_bytes_(contentSize) void const* content, unsigned contentSize) override
    {
        UINT16 constexpr BASE = 65521; // largest prime smaller than 65536
        UINT16 constexpr NMAX = 5552; // largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1

        auto buf = static_cast<UINT8 const*>(content);
        auto const bufEnd = buf + contentSize;

        unsigned s1 = m_s1;
        unsigned s2 = m_s2;

        // do length blocks of NMAX bytes -- requires just one modulo operation per block.
        while (bufEnd - buf >= NMAX)
        {
            for (unsigned n = 0; n != NMAX / 16; n += 1)
            {
                ADLER32_DO16(buf);
                buf += 16;
            }

            s1 %= BASE;
            s2 %= BASE;
        }

        // do remaining bytes
        if (bufEnd > buf)
        {
            while (bufEnd - buf >= 16)
            {
                ADLER32_DO16(buf);
                buf += 16;
            }

            while (bufEnd > buf)
            {
                s1 += *buf++;
                s2 += s1;
            }

            s1 %= BASE;
            s2 %= BASE;
        }

        m_s1 = static_cast<UINT16>(s1);
        m_s2 = static_cast<UINT16>(s2);
    }

    void
    FinalizeImpl(_Out_writes_bytes_(hashSize) void* hash, unsigned hashSize) override
    {
        FINALIZE_CHECK_HASHSIZE(hashSize, MyHashSize);
        UINT32 adler32 = _byteswap_ulong((static_cast<UINT32>(m_s2) << 16) | m_s1);
        memcpy(hash, &adler32, MyHashSize);
    }
};

//////////////////////////////////////////////////////////////////
// Crc32Hasher

class Crc32Hasher final : public Hasher
{
    UINT32 m_crcState = 0;

public:

    Crc32Hasher() noexcept
        : Hasher(sizeof(m_crcState))
    {
        return;
    }

    PCWSTR
    Name() const noexcept override
    {
        return L"Crc32";
    }

private:

    void
    ResetImpl() override
    {
        m_crcState = 0xFFFFFFFF;
    }

    void
    AppendImpl(_In_reads_bytes_(contentSize) void const* content, unsigned contentSize) override
    {
        static UINT32 const Crc32Table[] = {
            0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F,
            0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
            0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91, 0x1DB71064, 0x6AB020F2,
            0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
            0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9,
            0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
            0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B, 0x35B5A8FA, 0x42B2986C,
            0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
            0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423,
            0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
            0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D, 0x76DC4190, 0x01DB7106,
            0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
            0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D,
            0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
            0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457, 0x65B0D9C6, 0x12B7E950,
            0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
            0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7,
            0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
            0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9, 0x5005713C, 0x270241AA,
            0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
            0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81,
            0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
            0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683, 0xE3630B12, 0x94643B84,
            0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
            0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB,
            0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
            0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5, 0xD6D6A3E8, 0xA1D1937E,
            0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
            0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55,
            0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
            0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F, 0xC5BA3BBE, 0xB2BD0B28,
            0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
            0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F,
            0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
            0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21, 0x86D3D2D4, 0xF1D4E242,
            0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
            0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69,
            0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
            0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB, 0xAED16A4A, 0xD9D65ADC,
            0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
            0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693,
            0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
            0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
        };

        auto pBytes = static_cast<UINT8 const*>(content);
        auto const pBytesEnd = pBytes + contentSize;
        while (pBytes != pBytesEnd)
        {
            m_crcState = (m_crcState >> 8) ^ Crc32Table[(m_crcState & 0xFF) ^ *pBytes];
            pBytes += 1;
        }
    }

    void
    FinalizeImpl(_Out_writes_bytes_(hashSize) void* hash, unsigned hashSize) override
    {
        FINALIZE_CHECK_HASHSIZE(hashSize, sizeof(m_crcState));
        UINT32 crc32 = _byteswap_ulong(~m_crcState);
        memcpy(hash, &crc32, sizeof(crc32));
    }
};

//////////////////////////////////////////////////////////////////
// Fnv1aHasher

template<class ValueType, ValueType PRIME, ValueType OFFSET_BASIS>
class Fnv1aHasher final : public Hasher
{
    ValueType m_hashState = 0;

public:

    Fnv1aHasher() noexcept
        : Hasher(sizeof(m_hashState))
    {
        return;
    }

    PCWSTR
    Name() const noexcept override
    {
        return
            sizeof(ValueType) == 8 ? L"Fnv1a64" :
            sizeof(ValueType) == 4 ? L"Fnv1a32" :
            L"Fnv1a";
    }

private:

    void
    ResetImpl() override
    {
        m_hashState = OFFSET_BASIS;
    }

    void
    AppendImpl(_In_reads_bytes_(contentSize) void const* content, unsigned contentSize) override
    {
        auto pb = static_cast<UINT8 const*>(content);
        auto const pbEnd = pb + contentSize;
        while (pb != pbEnd)
        {
            m_hashState ^= *pb;
            m_hashState *= PRIME;
            pb += 1;
        }
    }

    void
    FinalizeImpl(_Out_writes_bytes_(hashSize) void* hash, unsigned hashSize) override
    {
        FINALIZE_CHECK_HASHSIZE(hashSize, sizeof(m_hashState));
        memcpy(hash, &m_hashState, sizeof(m_hashState));
    }
};

//////////////////////////////////////////////////////////////////
// MurmurHash3x64Hasher

class Murmur3x64_128Hasher final : public Hasher
{
    static auto constexpr MyHashSize = 16;
    UINT64 m_h1 = 0;
    UINT64 m_h2 = 0;
    UINT64 m_len = 0;

public:

    Murmur3x64_128Hasher() noexcept
        : Hasher(MyHashSize)
    {
        return;
    }

    PCWSTR
    Name() const noexcept override
    {
        return L"Murmur3x64_128";
    }

private:

    void
    ResetImpl() override
    {
        m_h1 = 0;
        m_h2 = 0;
        m_len = 0;
    }

    void
    AppendImpl(_In_reads_bytes_(contentSize) void const* content, unsigned contentSize) override
    {
        const UINT64 c1 = 0x87C37B91114253D5;
        const UINT64 c2 = 0x4CF5AD432745937F;

        //----------
        // body

        auto const blocks = static_cast<UINT64 const*>(content);
        auto const cBlocks = contentSize / 16;

        for (unsigned i = 0; i < cBlocks; i += 1)
        {
            UINT64 k1 = blocks[i * 2 + 0];
            UINT64 k2 = blocks[i * 2 + 1];

            k1 *= c1;
            k1 = _rotl64(k1, 31);
            k1 *= c2;
            m_h1 ^= k1;

            m_h1 = _rotl64(m_h1, 27);
            m_h1 += m_h2;
            m_h1 = m_h1 * 5 + 0x52DCE729;

            k2 *= c2;
            k2 = _rotl64(k2, 33);
            k2 *= c1;
            m_h2 ^= k2;

            m_h2 = _rotl64(m_h2, 31);
            m_h2 += m_h1;
            m_h2 = m_h2 * 5 + 0x38495AB5;
        }

        //----------
        // tail

        auto const tail = static_cast<UINT8 const*>(content) + cBlocks * 16;

        UINT64 k1 = 0;
        UINT64 k2 = 0;

        switch (contentSize & 15)
        {
        case 15:
            k2 ^= ((UINT64)tail[14]) << 48;
            __fallthrough;
        case 14:
            k2 ^= ((UINT64)tail[13]) << 40;
            __fallthrough;
        case 13:
            k2 ^= ((UINT64)tail[12]) << 32;
            __fallthrough;
        case 12:
            k2 ^= ((UINT64)tail[11]) << 24;
            __fallthrough;
        case 11:
            k2 ^= ((UINT64)tail[10]) << 16;
            __fallthrough;
        case 10:
            k2 ^= ((UINT64)tail[9]) << 8;
            __fallthrough;
        case  9:
            k2 ^= ((UINT64)tail[8]) << 0;
            k2 *= c2;
            k2 = _rotl64(k2, 33);
            k2 *= c1;
            m_h2 ^= k2;
            __fallthrough;
        case  8:
            k1 ^= ((UINT64)tail[7]) << 56;
            __fallthrough;
        case  7:
            k1 ^= ((UINT64)tail[6]) << 48;
            __fallthrough;
        case  6:
            k1 ^= ((UINT64)tail[5]) << 40;
            __fallthrough;
        case  5:
            k1 ^= ((UINT64)tail[4]) << 32;
            __fallthrough;
        case  4:
            k1 ^= ((UINT64)tail[3]) << 24;
            __fallthrough;
        case  3:
            k1 ^= ((UINT64)tail[2]) << 16;
            __fallthrough;
        case  2:
            k1 ^= ((UINT64)tail[1]) << 8;
            __fallthrough;
        case  1:
            k1 ^= ((UINT64)tail[0]) << 0;
            k1 *= c1;
            k1 = _rotl64(k1, 31);
            k1 *= c2;
            m_h1 ^= k1;
            break;
        };

        m_len += contentSize;
    }

    void
    FinalizeImpl(_Out_writes_bytes_(hashSize) void* hash, unsigned hashSize) override
    {
        FINALIZE_CHECK_HASHSIZE(hashSize, MyHashSize);

        auto h1 = m_h1;
        auto h2 = m_h2;

        h1 ^= m_len;
        h2 ^= m_len;

        h1 += h2;
        h2 += h1;

        h1 = fmix64(h1);
        h2 = fmix64(h2);

        h1 += h2;
        h2 += h1;

        memcpy(hash, &h1, sizeof(h1));
        memcpy(static_cast<BYTE*>(hash) + sizeof(h1), &h2, sizeof(h2));
    }

private:

    static UINT64
    fmix64(UINT64 k)
    {
        k ^= k >> 33;
        k *= 0xFF51AFD7ED558CCD;
        k ^= k >> 33;
        k *= 0xC4CEB9FE1A85EC53;
        k ^= k >> 33;
        return k;
    }
};

//////////////////////////////////////////////////////////////////
// BcryptHasher

struct BCryptDestroyHash_delete
{
    void operator()(BCRYPT_HASH_HANDLE handle) const { BCryptDestroyHash(handle); }
};

using unique_BCRYPT_HASH_HANDLE = std::unique_ptr<void, BCryptDestroyHash_delete>;

class BcryptHasher final : public Hasher
{
    unique_BCRYPT_HASH_HANDLE const m_hashHandle;
    std::vector<WCHAR> const m_name;

public:

    BcryptHasher(
        unsigned hashSize,
        unique_BCRYPT_HASH_HANDLE hashHandle,
        std::vector<WCHAR>&& name)
        : Hasher(hashSize)
        , m_hashHandle(std::move(hashHandle))
        , m_name(std::move(name))
    {
        return;
    }

    PCWSTR
    Name() const noexcept override
    {
        return m_name.data();
    }

private:

    void
    ResetImpl() override
    {
        // BCrypt hash objects created with BCRYPT_HASH_REUSABLE_FLAG
        // are automatically reset by BCryptFinishHash, so no action needed.
    }

    void
    AppendImpl(_In_reads_bytes_(contentSize) void const* content, unsigned contentSize) override
    {
        auto status = BCryptHashData(m_hashHandle.get(), (PUCHAR)content, contentSize, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            ThrowHrException(HRESULT_FROM_NT(status), "BCryptHashData failure");
        }
    }

    void
    FinalizeImpl(_Out_writes_bytes_(hashSize) void* hash, unsigned hashSize) override
    {
        // No need for FINALIZE_CHECK_HASHSIZE. BCryptFinishHash will fail if hashSize is incorrect.
        auto status = BCryptFinishHash(m_hashHandle.get(), static_cast<UCHAR*>(hash), hashSize, 0);
        if (!BCRYPT_SUCCESS(status))
        {
            ThrowHrException(HRESULT_FROM_NT(status), "BCryptFinishHash failure");
        }
    }
};

std::unique_ptr<Hasher>
HasherCreateBcrypt(BCRYPT_ALG_HANDLE bcryptAlgHandle)
{
    NTSTATUS status;

    BCRYPT_HASH_HANDLE rawHashHandle;
    status = BCryptCreateHash(bcryptAlgHandle, &rawHashHandle, nullptr, 0, nullptr, 0, BCRYPT_HASH_REUSABLE_FLAG);
    if (!BCRYPT_SUCCESS(status))
    {
        ThrowHrException(HRESULT_FROM_NT(status), "BCryptCreateHash failure");
    }

    unique_BCRYPT_HASH_HANDLE hashHandle(rawHashHandle);

    ULONG hashSize = 0;
    ULONG resultLength = 0;
    status = BCryptGetProperty(hashHandle.get(), BCRYPT_HASH_LENGTH, (PUCHAR)&hashSize, sizeof(hashSize), &resultLength, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        ThrowHrException(HRESULT_FROM_NT(status), "BCryptGetProperty(BCRYPT_HASH_LENGTH) failure");
    }

    status = BCryptGetProperty(bcryptAlgHandle, BCRYPT_ALGORITHM_NAME, nullptr, 0, &resultLength, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        ThrowHrException(HRESULT_FROM_NT(status), "BCryptGetPropertyLength(BCRYPT_ALGORITHM_NAME) failure");
    }

    std::vector<WCHAR> name(resultLength / sizeof(WCHAR));
    status = BCryptGetProperty(bcryptAlgHandle, BCRYPT_ALGORITHM_NAME, (PUCHAR)name.data(), resultLength, &resultLength, 0);
    if (!BCRYPT_SUCCESS(status))
    {
        ThrowHrException(HRESULT_FROM_NT(status), "BCryptGetProperty(BCRYPT_ALGORITHM_NAME) failure");
    }

    return std::make_unique<BcryptHasher>(hashSize, std::move(hashHandle), std::move(name));
}

//////////////////////////////////////////////////////////////////
// Well-known hashers

std::unique_ptr<Hasher>
HasherCreateAdler32()
{
    return std::make_unique<Adler32Hasher>();
}

std::unique_ptr<Hasher>
HasherCreateCrc32()
{
    return std::make_unique<Crc32Hasher>();
}

std::unique_ptr<Hasher>
HasherCreateFnv1a32()
{
    return std::make_unique<Fnv1aHasher<UINT32, 0x01000193, 0x811C9DC5>>();
}

std::unique_ptr<Hasher>
HasherCreateFnv1a64()
{
    return std::make_unique<Fnv1aHasher<UINT64, 0x100000001B3, 0xCBF29CE484222325>>();
}

std::unique_ptr<Hasher>
HasherCreateMD4()
{
    return HasherCreateBcrypt(BCRYPT_MD4_ALG_HANDLE);
}

std::unique_ptr<Hasher>
HasherCreateMD5()
{
    return HasherCreateBcrypt(BCRYPT_MD5_ALG_HANDLE);
}

std::unique_ptr<Hasher>
HasherCreateMurmur3x64_128()
{
    return std::make_unique<Murmur3x64_128Hasher>();
}

std::unique_ptr<Hasher>
HasherCreateSHA1()
{
    return HasherCreateBcrypt(BCRYPT_SHA1_ALG_HANDLE);
}

std::unique_ptr<Hasher>
HasherCreateSHA256()
{
    return HasherCreateBcrypt(BCRYPT_SHA256_ALG_HANDLE);
}

std::unique_ptr<Hasher>
HasherCreateSHA384()
{
    return HasherCreateBcrypt(BCRYPT_SHA384_ALG_HANDLE);
}

std::unique_ptr<Hasher>
HasherCreateSHA512()
{
    return HasherCreateBcrypt(BCRYPT_SHA512_ALG_HANDLE);
}

std::unique_ptr<Hasher>
HasherCreateXor64()
{
    return std::make_unique<Xor64Hasher>();
}

std::span<HasherInfo const>
HasherInfos() noexcept
{
    return g_hasherInfos;
}

// If name matches a well-known hasher, returns a pointer to the corresponding HasherInfo.
// Otherwise, returns nullptr.
_Ret_opt_ HasherInfo const*
HasherInfoByName(_In_z_ PCWSTR name) noexcept
{
    for (auto const& hasherInfo : g_hasherInfos)
    {
        if (StrEqualIgnoreCase(name, hasherInfo.Name))
        {
            return &hasherInfo;
        }
    }

    return nullptr;
}

// Returns a pointer to the HasherInfo for the Murmur3x64_128 hasher.
_Ret_ HasherInfo const*
HasherInfoDefault() noexcept
{
    return &g_hasherInfos[static_cast<unsigned>(HasherId::Default)];
}

// Creates a hasher by ID. If id is not recognized, throws an exception.
std::unique_ptr<Hasher>
HasherCreateById(HasherId id)
{
    switch (id)
    {
    case HasherId::Adler32:        return HasherCreateAdler32();
    case HasherId::Crc32:          return HasherCreateCrc32();
    case HasherId::Fnv1a32:        return HasherCreateFnv1a32();
    case HasherId::Fnv1a64:        return HasherCreateFnv1a64();
    case HasherId::MD4:            return HasherCreateMD4();
    case HasherId::MD5:            return HasherCreateMD5();
    case HasherId::Murmur3x64_128: return HasherCreateMurmur3x64_128();
    case HasherId::SHA1:           return HasherCreateSHA1();
    case HasherId::SHA256:         return HasherCreateSHA256();
    case HasherId::SHA384:         return HasherCreateSHA384();
    case HasherId::SHA512:         return HasherCreateSHA512();
    case HasherId::Xor64:          return HasherCreateXor64();
    default:
        ThrowHrException(E_INVALIDARG, "HasherCreateById(): unrecognized hasher ID.");
    }
}
