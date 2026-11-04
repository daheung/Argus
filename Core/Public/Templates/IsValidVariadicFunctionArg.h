#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/IsEnum.h"

template <typename T, bool = TIsEnum<T>::Value>
struct TIsValidVariadicFunctionArg
{
private:
	typedef char No [1];
	typedef char Yes[2];

	static constexpr FORCEINLINE Yes& Tester(uint32) noexcept;
	static constexpr FORCEINLINE Yes& Tester(uint8 ) noexcept;
	static constexpr FORCEINLINE Yes& Tester(int32 ) noexcept;
	static constexpr FORCEINLINE Yes& Tester(uint64) noexcept;
	static constexpr FORCEINLINE Yes& Tester(int64 ) noexcept;
	static constexpr FORCEINLINE Yes& Tester(double) noexcept;
	static constexpr FORCEINLINE Yes& Tester(		  long) noexcept;
	static constexpr FORCEINLINE Yes& Tester(unsigned long) noexcept;
	static constexpr FORCEINLINE Yes& Tester(wchar_t) noexcept;
	static constexpr FORCEINLINE Yes& Tester(bool   ) noexcept;
	static constexpr FORCEINLINE Yes& Tester(const void*) noexcept;

	static constexpr FORCEINLINE No& Tester(...) noexcept;

	static constexpr FORCEINLINE T DeclValT() noexcept;

public:
	enum { Value = sizeof(Tester(DeclValT())) == sizeof(Yes) };
};

template <typename T>
struct TIsValidVariadicFunctionArg<T, true>
{
	enum { Value = true };
};