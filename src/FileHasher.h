// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

class Hasher;

class FileHasher
{
    std::vector<BYTE> m_fileData;

public:

    HRESULT
    HashFile(_In_z_ PCWSTR fileName, _Inout_ Hasher* pHasher);

    HRESULT
    HashFile(_In_z_ PCWSTR fileName, std::span<Hasher* const> hashers);

    HRESULT
    HashStdin(_Inout_ Hasher* pHasher);

    HRESULT
    HashStdin(std::span<Hasher* const> hashers);

private:

    HRESULT
    HashHandle(HANDLE handle, std::span<Hasher* const> hashers);
};
