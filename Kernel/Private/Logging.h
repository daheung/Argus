#pragma once

#include <ntifs.h>
#include <ntstrsafe.h>
#include <stdarg.h>

namespace Argus::KernelLogging
{
	inline void Write(ULONG DebugLevel, const char* Level, const char* Event,
		const char* Format, ...) noexcept
	{
		LARGE_INTEGER UtcTime{};
		KeQuerySystemTimePrecise(&UtcTime);

		TIME_FIELDS Fields{};
		RtlTimeToTimeFields(&UtcTime, &Fields);

		char Prefix[96]{};
		const NTSTATUS Status = RtlStringCchPrintfA(Prefix, RTL_NUMBER_OF(Prefix),
			"[Argus] %04d-%02d-%02dT%02d:%02d:%02d.%03dZ %s kernel %s ",
			Fields.Year, Fields.Month, Fields.Day,
			Fields.Hour, Fields.Minute, Fields.Second, Fields.Milliseconds,
			Level, Event);
		if (!NT_SUCCESS(Status))
		{
			return;
		}

		va_list Args;
		va_start(Args, Format);
		vDbgPrintExWithPrefix(Prefix, DPFLTR_IHVDRIVER_ID, DebugLevel, Format, Args);
		va_end(Args);
	}
}
