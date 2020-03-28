// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "winrt/base.h"
void* winrt_make_facebook_react_MemoryTracker();
void* winrt_make_facebook_react_NativeLogEventSource();
void* winrt_make_facebook_react_NativeTraceEventSource();

bool __stdcall Desktop_can_unload_now() noexcept
{
    if (winrt::get_module_lock())
    {
        return false;
    }

    winrt::clear_factory_cache();
    return true;
}

void* __stdcall Desktop_get_activation_factory([[maybe_unused]] std::wstring_view const& name)
{
    auto requal = [](std::wstring_view const& left, std::wstring_view const& right) noexcept
    {
        return std::equal(left.rbegin(), left.rend(), right.rbegin(), right.rend());
    };

    if (requal(name, L"facebook.react.MemoryTracker"))
    {
        return winrt_make_facebook_react_MemoryTracker();
    }

    if (requal(name, L"facebook.react.NativeLogEventSource"))
    {
        return winrt_make_facebook_react_NativeLogEventSource();
    }

    if (requal(name, L"facebook.react.NativeTraceEventSource"))
    {
        return winrt_make_facebook_react_NativeTraceEventSource();
    }

    return nullptr;
}
