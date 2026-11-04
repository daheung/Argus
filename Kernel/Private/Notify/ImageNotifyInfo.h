#pragma once

#include "Core/Public/Notify/ImageNotifyTypes.h"
#include "Core/Public/Templates/IsTriviallyRelocatable.h"
#include "Kernel/Public/Misc/KernelDefines.h"

namespace Argus::Notify
{
	struct FImageNotifyInfo
	{
		LARGE_INTEGER LoadedAt{};
		uint32 ProcessId = 0;
		uint64 ImageSize = 0;
		uint64 ImageBaseAddress = 0;
		uint32 ImageFileNameChars = 0;
		PWCHAR ImageFileName = nullptr;

		FORCEINLINE bool operator==(const FImageNotifyInfo& Other) const noexcept
		{
			return ProcessId == Other.ProcessId;
		}

		FImageNotifyInfo() = default;
		FImageNotifyInfo(const FImageNotifyInfo&) = delete;
		FImageNotifyInfo& operator=(const FImageNotifyInfo&) = delete;

		FImageNotifyInfo(FImageNotifyInfo&& Other) noexcept
			: LoadedAt(Other.LoadedAt)
			, ProcessId(Other.ProcessId)
			, ImageSize(Other.ImageSize)
			, ImageBaseAddress(Other.ImageBaseAddress)
			, ImageFileNameChars(Other.ImageFileNameChars)
			, ImageFileName(Other.ImageFileName)
		{
			Other.ImageFileNameChars = 0;
			Other.ImageFileName = nullptr;
		}

		FImageNotifyInfo& operator=(FImageNotifyInfo&& Other) noexcept
		{
			if (this != &Other)
			{
				if (ImageFileName != nullptr)
				{
					ExFreePool(ImageFileName);
				}

				LoadedAt = Other.LoadedAt;
				ProcessId = Other.ProcessId;
				ImageSize = Other.ImageSize;
				ImageBaseAddress = Other.ImageBaseAddress;
				ImageFileNameChars = Other.ImageFileNameChars;
				ImageFileName = Other.ImageFileName;

				Other.ImageFileNameChars = 0;
				Other.ImageFileName = nullptr;
			}

			return *this;
		}

		~FImageNotifyInfo() noexcept
		{
			if (ImageFileName != nullptr)
			{
				ExFreePool(ImageFileName);
			}
		}
	};
}

// The command line lives in a separate pool allocation. Relocation transfers
// its pointer and abandons the old record without invoking its destructor.
template <>
struct TIsTriviallyRelocatable<Argus::Notify::FImageNotifyInfo>
{
	static constexpr bool Value = true;
};
