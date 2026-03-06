// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

class Hasher;

class FileHasher
{
    struct VirtualFree_delete
    {
        void operator()(void* ptr) const noexcept;
    };
    
    std::unique_ptr<BYTE[], VirtualFree_delete> m_fileData;

public:

    FileHasher() noexcept;

    HRESULT
    HashFile(_In_z_ PCWSTR fileName, bool unbufferedIO, _Inout_ Hasher* pHasher);

    HRESULT
    HashFile(_In_z_ PCWSTR fileName, bool unbufferedIO, std::span<Hasher* const> hashers);

    HRESULT
    HashStdin(_Inout_ Hasher* pHasher);

    HRESULT
    HashStdin(std::span<Hasher* const> hashers);

private:

    HRESULT
    HashHandle(HANDLE handle, bool unbufferedIO, std::span<Hasher* const> hashers);
};
