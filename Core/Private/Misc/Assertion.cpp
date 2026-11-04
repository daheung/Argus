#include "Core/Public/Misc/Assertion.h"

CORE_C_API void ReportAssertionFailure(const char* Expression, const char* Message, const char* File, int32 Line)
{
	UNREFERENCED_PARAMETER(Expression);
	UNREFERENCED_PARAMETER(Message);
	UNREFERENCED_PARAMETER(File);
	UNREFERENCED_PARAMETER(Line);
}


CORE_C_API void ReportAssertionFailureFmt(const char* Expression, const char* Message, const char* File, int32 Line, ...)
{
	UNREFERENCED_PARAMETER(Expression);
	UNREFERENCED_PARAMETER(Message);
	UNREFERENCED_PARAMETER(File);
	UNREFERENCED_PARAMETER(Line);
}
