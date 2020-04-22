// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "ModuleRegistration.h"
#include <sstream>
#include <string_view>
#include "Crash.h"

namespace winrt::Microsoft::ReactNative {

const ModuleRegistration *ModuleRegistration::s_head{nullptr};

ModuleRegistration::ModuleRegistration(wchar_t const *structName, wchar_t const *moduleName) noexcept
    : m_structName{structName}, m_moduleName{moduleName}, m_next{s_head} {
  s_head = this;
}

static void ValidateModuleNames() noexcept {
  // See if there are any duplicates for the struct or module names.
  // We do this validation only once.
  [[maybe_unused]] static bool AreNamesValid = []() noexcept {
    std::map<std::wstring_view, ModuleRegistration const*, std::less<>> structNames;
    std::map<std::wstring_view, ModuleRegistration const*, std::less<>> moduleNames;

    for (auto const *reg = ModuleRegistration::Head(); reg != nullptr; reg = reg->Next()) {
      auto structResult = structNames.try_emplace(reg->StructName(), reg);
      if (!structResult.second) {
        std::stringstream ss;
        ss << "Trying to register struct '" << reg->StructName() << "' with module name '" << reg->ModuleName()
           << "', but it is already registered with module name '" << structResult.first->second->ModuleName() << "'.";
        std::cout<< ss.str();

         char *p1, *p2;

        // The Reporting Mode and File must be specified
        // before generating a debug report via an assert
        // or report macro.
        // This program sends all report types to STDOUT.
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

        // Allocate and assign the pointer variables.
        p1 = (char *)malloc(10);
        strcpy_s(p1, 10, "I am p1");
        p2 = (char *)malloc(10);
        strcpy_s(p2, 10, "I am p2");

        // Use the report macros as a debugging
        // warning mechanism, similar to printf.
        // Use the assert macros to check if the
        // p1 and p2 variables are equivalent.
        // If the expression fails, _ASSERTE will
        // include a string representation of the
        // failed expression in the report.
        // _ASSERT does not include the
        // expression in the generated report.
        _RPT0(_CRT_WARN, "Use the assert macros to evaluate the expression p1 == p2.\n");
        _RPTF2(_CRT_WARN, "\n Will _ASSERT find '%s' == '%s' ?\n", p1, p2);
        _ASSERT(p1 == p2);

        _RPTF2(_CRT_WARN, "\n\n Will _ASSERTE find '%s' == '%s' ?\n", p1, p2);
        _ASSERTE(p1 == p2);

        _RPT2(_CRT_ERROR, "'%s' != '%s'\n", p1, p2);

        free(p2);
        free(p1);

        VerifyElseCrashSz(false, ss.str().c_str());
      }
    }

    for (auto const *reg = ModuleRegistration::Head(); reg != nullptr; reg = reg->Next()) {
      auto moduleResult = moduleNames.try_emplace(reg->ModuleName(), reg);
      if (!moduleResult.second) {
        std::stringstream ss;
        ss << "Trying to register struct '" << reg->StructName() << "' with module name '" << reg->ModuleName()
           << "', but this module name registered for the  '" << moduleResult.first->second->StructName()
           << "' module.";
        VerifyElseCrashSz(false, ss.str().c_str());
      }
    }

    return true;
  }();
}

void AddAttributedModules(IReactPackageBuilder const &packageBuilder) noexcept {
  ValidateModuleNames();
  for (auto const *reg = ModuleRegistration::Head(); reg != nullptr; reg = reg->Next()) {
    packageBuilder.AddModule(reg->ModuleName(), reg->MakeModuleProvider());
  }
}

bool TryAddAttributedModule(IReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) noexcept {
  ValidateModuleNames();
  for (auto const *reg = ModuleRegistration::Head(); reg != nullptr; reg = reg->Next()) {
    if (moduleName == reg->ModuleName()) {
      packageBuilder.AddModule(moduleName, reg->MakeModuleProvider());
      return true;
    }
  }
  return false;
}

} // namespace winrt::Microsoft::ReactNative
