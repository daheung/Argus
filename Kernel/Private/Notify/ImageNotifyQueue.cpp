#include "ImageNotifyQueue.h"

void FImageNotifyQueue::OnImageNotify(PUNICODE_STRING FullImageName, HANDLE ProcessId, PIMAGE_INFO InImageInfo)
{
	constexpr uint32 KernelModeImage = 0x01;
	if (InImageInfo->SystemModeImage == KernelModeImage)
	{
		return;
	}
	if (!this->TryRestore())
	{
		return;
	}

	Argus::Notify::FImageNotifyInfo ImageInfo{};
	KeQuerySystemTimePrecise(&ImageInfo.LoadedAt);

	ImageInfo.ProcessId = HandleToULong(ProcessId);
	ImageInfo.ImageSize = InImageInfo->ImageSize;
	ImageInfo.ImageBaseAddress = static_cast<uint64>((size_t)InImageInfo->ImageBase);
	
	//if (InImageInfo->ExtendedInfoPresent)
	//{
	//	IMAGE_INFO_EX* ExInfo = CONTAINING_RECORD(InImageInfo, IMAGE_INFO_EX, ImageInfo);
	//	FLT_FILE_NAME_INFORMATION* NameInfo{};

	//	constexpr FLT_FILE_NAME_OPTIONS FileNameOptions = FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT;
	//	if (NT_SUCCESS(FltGetFileNameInformationUnsafe(ExInfo->FileObject, nullptr, FileNameOptions, &NameInfo)))
	//	{
	//		uint32 ImageFileNameSize = CoreMin(ImageNotifyMaxFileNameBytes, NameInfo->Name.Length);
	//		WCHAR* ImageFileName = static_cast<WCHAR*>(ExAllocatePool2(POOL_FLAG_PAGED, ImageFileNameSize + sizeof(WCHAR), 'NmIk'));
	//		if (ImageFileName != nullptr)
	//		{
	//			RtlCopyMemory(ImageFileName, NameInfo->Name.Buffer, ImageFileNameSize);
	//			ImageFileName[ImageFileNameSize / sizeof(WCHAR)] = L'\0';

	//			ImageInfo.ImageFileName = ImageFileName;
	//			ImageInfo.ImageFileNameChars = ImageFileNameSize / sizeof(WCHAR);
	//		}

	//		FltReleaseFileNameInformation(NameInfo);
	//	}
	//}

	if (FullImageName != nullptr && FullImageName->Length > 0)
	{
		//uint16 ImageFileNameSize = CoreMin(ImageNotifyMaxFileNameBytes, FullImageName->Length);
		uint16 ImageFileNameSize = FullImageName->Length;
		WCHAR* ImageFileName = static_cast<WCHAR*>(ExAllocatePool2(POOL_FLAG_PAGED, ImageFileNameSize + sizeof(WCHAR), 'NmIk'));
		if (ImageFileName != nullptr)
		{
			RtlCopyMemory(ImageFileName, FullImageName->Buffer, ImageFileNameSize);
			ImageFileName[ImageFileNameSize / sizeof(WCHAR)] = L'\0';

			ImageInfo.ImageFileName = ImageFileName;
			ImageInfo.ImageFileNameChars = ImageFileNameSize / sizeof(WCHAR);
		}
	}
	else
	{
		ImageInfo.ImageFileNameChars = 0;
	}

	KeEnterCriticalRegion();
	ExAcquirePushLockExclusive(&this->ImageNotifyInfosLock);

	const bool bWasFull = this->ImageNotifyInfos.IsFull();
	const bool bPushSucceed = this->ImageNotifyInfos.PushDropOldestIfFull(MoveTemp(ImageInfo)) != INDEX_NONE;
	if (!bPushSucceed || bWasFull)
	{
		++this->DroppedImageNotifyCount;
	}

	ExReleasePushLockExclusive(&this->ImageNotifyInfosLock);
	KeLeaveCriticalRegion();
}
