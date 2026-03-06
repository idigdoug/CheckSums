// Copyright (c) Doug Cook.
// Licensed under the MIT License.

#pragma once

struct HrException
    : public std::exception
{
    HRESULT hr;

    HrException(HRESULT hr, PCSTR what)
        : std::exception(what), hr(hr)
    {
        return;
    }
};
