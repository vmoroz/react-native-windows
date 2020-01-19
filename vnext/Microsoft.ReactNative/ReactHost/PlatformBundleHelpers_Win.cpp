#include "precomp.h"

#include <msoFolly/MsoFolly.h>
#include "BytecodeHelpers.h"
#include "PlatformBundleHelpers.h"

#include <InstanceManager.h>
#include <JSBigAbiString.h>

namespace Mso { namespace React {

struct JSBundleAbiString : AbiSafe::AbiObject<AbiSafe::IAbiString>
{
	static AbiSafe::AbiStringPtr Make(Mso::TCntPtr<IJSBundle>&& jsBundle) noexcept
	{
		return AbiSafe::AbiMake<JSBundleAbiString, AbiSafe::IAbiString>(std::move(jsBundle));
	}

	AbiSafe::AbiCharSpan GetSpan() const noexcept override
	{
		return AbiSafe::AbiCharSpan{m_jsBundle->Content().data(), m_jsBundle->Content().size()};
	}

	JSBundleAbiString(Mso::TCntPtr<IJSBundle>&& jsBundle) noexcept
		: m_jsBundle{ std::move(jsBundle) }
	{
	}

private:
	Mso::TCntPtr<IJSBundle> m_jsBundle;
};

std::vector<facebook::react::PlatformBundleInfo> MakePlatformBundleInfos(const std::vector<TCntPtr<IJSBundle>>& jsBundles) noexcept
{
	std::vector<facebook::react::PlatformBundleInfo> platformBundleInfos;

	for (auto& jsBundle : jsBundles)
	{
		auto abiStringPtr = JSBundleAbiString::Make(Mso::TCntPtr<IJSBundle>(jsBundle)); // Add Ref
		JSBundleInfo jsBundleInfo = jsBundle->Info();

		platformBundleInfos.push_back(facebook::react::PlatformBundleInfo{
			/*BundleContent*/ facebook::react::JSBigAbiString::Make(std::move(abiStringPtr)),
			/*BundleUrl*/ jsBundleInfo.FileName, // Copy
			/*BytecodePath*/ GetBytecodeFilePath(GetFileName(jsBundleInfo.FileName)), // Create Custom String representing BytecodeFileName
			/*Version*/ jsBundleInfo.Timestamp});
	}

	return platformBundleInfos;
}

}} // namespace Mso::React
