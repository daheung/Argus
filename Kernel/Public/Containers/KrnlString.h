#pragma once
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h" 
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Core/Public/Math/NumericLimits.h"
#include "Core/Public/Misc/CString.h"
#include "Core/Public/Misc/GenericString.h"

namespace ESearchCase
{
	enum Type
	{
		CaseSecsitive,
		IgnoreCase,
	};
}

namespace ESearchDir
{
	enum Type
	{
		FromStart,
		FromEnd,
	};
}

namespace Argus::String
{
	template <EPoolType PoolType>
	NODISCARD FORCEINLINE WIDECHAR* GetData(FKrnlString<PoolType>& String);

	template <EPoolType PoolType>
	NODISCARD FORCEINLINE const WIDECHAR* GetData(const FKrnlString<PoolType>& String);

	template <EPoolType PoolType>
	NODISCARD FORCEINLINE int32 GetNum(const FKrnlString<PoolType>& String);
}

template <EPoolType InPoolType>
class FKrnlString
{
public:
	static constexpr EPoolType PoolType = InPoolType;

	using ElementType = WIDECHAR;

	using AllocatorType = TKrnlElLinearAllocator<ElementType, int32, PoolType>;

	using SizeType = typename AllocatorType::SizeType;

private:
	typedef ElementType TArrayCharType;

	typedef TKrnlArray<PoolType, TArrayCharType, AllocatorType> DataType;

	template <EPoolType OtherPoolType, typename OtherElementType, typename OtherAllocatorType>
	friend class TKrnlArray;

	template <typename RangeType>
	using TRangeElementType = typename TRemoveConstVolatile<typename TRemovePointer<decltype(Argus::String::GetData(DeclVal<RangeType>()))>::Type>::Type;

	template <typename CharRangeType>
	struct TIsRangeOfCharType : TIsChar<TRangeElementType<CharRangeType>>
	{
	};

	template <typename CharRangeType>
	struct TIsRangeOfWCHAR
	{
		enum { Value = TIsSame<WIDECHAR, TRangeElementType<CharRangeType>>::Value };
	};

	template <typename CharRangeType>
	using TIsCharRangeNotCArray = TAnd<
		TIsContiguousContainer<CharRangeType>,
		TNot<TIsArray<typename TRemoveReference<CharRangeType>::Type>>,
		TIsRangeOfCharType<CharRangeType>>;

	template <typename CharRangeType>
	using TIsCharRangeNotCArrayNotFKrnlString = TAnd<
		TIsCharRangeNotCArray<CharRangeType>,
		TNot<TIsDerivedFrom<typename TDecay<CharRangeType>::Type, FKrnlString>>>;

	template <typename CharRangeType>
	using TIsWideCharRangeNotCArray = TAnd <
		TIsContiguousContainer<CharRangeType>,
		TNot<TIsArray<typename TRemoveReference<CharRangeType>::Type>>,
		TIsRangeOfWCHAR<CharRangeType>>;

	template <typename CharRangeType>
	using TIsWideCharRangeNotCArrayNotFKrnlString = TAnd<
		TIsWideCharRangeNotCArray<CharRangeType>,
		TNot<TIsDerivedFrom<typename TDecay<CharRangeType>::Type, FKrnlString>>>;

	template <typename SrcEncoding>
	using TIsFormatCompatibleWithWideChar = TIsCharEncodingCompatibleWith<SrcEncoding, WIDECHAR>;

	template <typename SrcEncoding>
	using TIsAppendCompatibleWithChar = TAnd<TIsChar<SrcEncoding>,TNot<TIsSame<SrcEncoding, UTF32CHAR>>>;

public:
	FORCEINLINE FKrnlString() = default;
	FORCEINLINE FKrnlString(FKrnlString&&) = default;
	FORCEINLINE FKrnlString(const FKrnlString&) = default;
	FORCEINLINE FKrnlString& operator=(FKrnlString&&) = default;
	FORCEINLINE FKrnlString& operator=(const FKrnlString&) = default;

	FORCEINLINE FKrnlString(const FKrnlString& Other, int32 ExtraSlack)
		: Data(Other.Data, ExtraSlack + ((Other.Data.Num() || !ExtraSlack) ? 0 : 1))
	{
	}

	FORCEINLINE FKrnlString(FKrnlString&& Other, int32 ExtraSlack)
		: Data(MoveTemp(Other.Data), ExtraSlack + ((Other.Data.Num() || !ExtraSlack) ? 0 : 1))
	{
	}

	//FKrnlString(const ANSICHAR* Str);
	//FKrnlString(const WIDECHAR* Str);
	//FKrnlString(const UTF8CHAR* Str);

	//FKrnlString(int32 Len, const ANSICHAR* Str);
	//FKrnlString(int32 Len, const WIDECHAR* Str);
	//FKrnlString(int32 Len, const UTF8CHAR* Str);

	//FKrnlString(const ANSICHAR* Str, int32 ExtraSlack);
	//FKrnlString(const WIDECHAR* Str, int32 ExtraSlack);
	//FKrnlString(const UTF8CHAR* Str, int32 ExtraSlack);

	//template <typename CharRangeType, typename TEnableIf<TIsCharRangeNotCArrayNotFKrnlString<CharRangeType>::Value>::Type* = nullptr>
	//FORCEINLINE explicit FKrnlString(CharRangeType&& Str)
	//	: FKrnlString(GetNum(Str), GetData(Forward<CharRangeType>(Str)))
	//{
	//}

	//template <typename CharRangeType, typename TEnableIf<TIsCharRangeNotCArrayNotFKrnlString<CharRangeType>::Value>::Type* = nullptr>
	//FORCEINLINE explicit FKrnlString(CharRangeType&& Str, int32 ExtraSlack)
	//{
	//	uint32 InLen = GetNum(Str);
	//	Reserve(InLen + ExtraSlack);
	//	AppendChars(GetData(Forward<CharRangeType>(Str)), InLen);
	//}

	template <typename CharRangeType, typename TEnableIf<TIsCharRangeNotCArrayNotFKrnlString<CharRangeType>::Value>::Type* = nullptr>
	FORCEINLINE bool Initialize(CharRangeType&& Str) noexcept
	{
		return this->InitializeWithLength(Forward<CharRangeType>(Argus::String::GetData<PoolType>(Str)), Argus::String::GetNum<PoolType>(Str));
	}

	template <typename CharRangeType, typename TEnableIf<TIsCharRangeNotCArrayNotFKrnlString<CharRangeType>::Value>::Type* = nullptr>
	FORCEINLINE bool Initialize(CharRangeType&& Str, int32 ExtraSlack) noexcept
	{
		return this->InitializeWithSlack(Forward<CharRangeType>(Argus::String::GetData<PoolType>(Str)), ExtraSlack);
	}

	FORCEINLINE bool InitializeFromCString(const ANSICHAR* Str) noexcept { return this->InitializeFromCStringImpl<ANSICHAR>(Str); }
	FORCEINLINE bool InitializeFromCString(const WIDECHAR* Str) noexcept { return this->InitializeFromCStringImpl<WIDECHAR>(Str); }
	FORCEINLINE bool InitializeFromCString(const UTF8CHAR* Str) noexcept { return this->InitializeFromCStringImpl<UTF8CHAR>(Str); }

	/**
	 * @brief Replaces this string with an explicitly sized source range.
	 *
	 * The source need not be null-terminated. A zero length produces an empty
	 * string with a null terminator, but String must still be non-null.
	 *
	 * @param String Non-null pointer to the source elements.
	 * @param StrLength Number of source elements, not a byte count in general.
	 * @return True on success. False for invalid input, conversion failure, or
	 * allocation failure; the existing string remains unchanged on failure.
	 */
	FORCEINLINE bool InitializeWithLength(const ANSICHAR* String, int32 StrLength) noexcept { return this->InitializeWithLengthImpl<ANSICHAR>(String, StrLength); }
	FORCEINLINE bool InitializeWithLength(const WIDECHAR* String, int32 StrLength) noexcept { return this->InitializeWithLengthImpl<WIDECHAR>(String, StrLength); }
	FORCEINLINE bool InitializeWithLength(const UTF8CHAR* String, int32 StrLength) noexcept { return this->InitializeWithLengthImpl<UTF8CHAR>(String, StrLength); }

	FORCEINLINE bool InitializeWithSlack(const ANSICHAR* String, int32 ExtraSlack) noexcept { return this->InitializeWithSlackImpl<ANSICHAR>(String, ExtraSlack); }
	FORCEINLINE bool InitializeWithSlack(const WIDECHAR* String, int32 ExtraSlack) noexcept { return this->InitializeWithSlackImpl<WIDECHAR>(String, ExtraSlack); }
	FORCEINLINE bool InitializeWithSlack(const UTF8CHAR* String, int32 ExtraSlack) noexcept { return this->InitializeWithSlackImpl<UTF8CHAR>(String, ExtraSlack); }

private:
	template <typename CharType>
	bool InitializeFromCStringImpl(const CharType* Source) noexcept
	{
		if (Source == nullptr)
		{
			return false;
		}

		DataType Temp;
		int32 SrcLen = TCString<CharType>::Strlen(Source) + 1;
		int32 DestLen = FGenericString::ConvertedLength<TArrayCharType>(Source, SrcLen);

		ReturnIfFalse(Temp.PushUninitialized(DestLen) != INDEX_NONE);
		ReturnIfFalse(FGenericString::Convert(Temp.GetData(), DestLen, Source, SrcLen));

		DataType::Swap(Temp, this->Data);

		return true;
	}

	template <typename CharType>
	bool InitializeWithLengthImpl(const CharType* Source, SizeType InitialLength) noexcept
	{
		if (Source == nullptr || InitialLength < 0)
		{
			return false;
		}

		int32 DestLen = FGenericString::ConvertedLength<TArrayCharType>(Source, InitialLength);
		if (DestLen < 0 || DestLen >= TNumericLimits<SizeType>::Max())
		{
			return false;
		}

		DataType Temp;
		ReturnIfFalse(Temp.PushUninitialized(DestLen + 1) != INDEX_NONE);
		if (DestLen > 0)
		{
			ReturnIfFalse(FGenericString::Convert(Temp.GetData(), DestLen, Source, InitialLength));
		}

		Temp.Last() = TEXT('\0');
		DataType::Swap(this->Data, Temp);

		return true;
	}

	template <typename CharType>
	bool InitializeWithSlackImpl(const CharType* Source, SizeType ExtraSlack) noexcept
	{
		if (Source && *Source)
		{
			int32 SrcLen = TCString<TArrayCharType>::Strlen(Source) + 1;
			int32 DestLen = FGenericString::ConvertedLength<CharType>(Source, SrcLen);
			
			ReturnIfFalse(this->Data.ResizeAllocation(DestLen + ExtraSlack));
			ReturnIfFalse(this->Data.PushUninitialized(DestLen) != INDEX_NONE);

			ReturnIfFalse(FGenericString::Convert(this->Data.GetData(), DestLen, Source, SrcLen));

			return true;
		}
		else if (ExtraSlack > 0)
		{
			return this->Data.ResizeAllocation(ExtraSlack + 1);
		}
		else
		{
			return false;
		}
	}

public:
	FKrnlString& operator=(const WIDECHAR* Str);

	template <typename CharRangeType, typename TEnableIf<TIsCharRangeNotCArray<CharRangeType>::Value>::Type* = nullptr>
	FORCEINLINE FKrnlString& operator=(CharRangeType&& Range)
	{
		AssignRange(GetData(Range), GetNum(Range));
		return *this;
	}

private:
	bool AssignRange(const TArrayCharType* OtherData, int32 OtherLen) noexcept
	{
		if (!OtherData || OtherLen <= 0)
		{
			return ContainerSuccess(this->Empty(0));
		}

		const int32 ThisLen = Len();
		if (OtherLen <= ThisLen)
		{
			TArrayCharType* DataPtr = Data.GetData();
			ConstructItemsFrom<TArrayCharType, WIDECHAR>(DataPtr, OtherData, OtherLen);
			DataPtr[OtherLen] = TEXT('\0');
			
			ReturnIfFalse(Data.RemoveAt(OtherLen + 1, ThisLen - OtherLen));
		}
		else
		{
			DataType Temp;
			ReturnIfFalse(Temp.Initialize(OtherLen + 1) == EContainerStatus::Success);
			ReturnIfFalse(Temp.PushUninitialized(OtherLen + 1) != INDEX_NONE);

			TArrayCharType* DataPtr = Temp.GetData();
			ConstructItemsFrom<TArrayCharType, WIDECHAR>(DataPtr, OtherData, OtherLen);
			DataPtr[OtherLen] = TEXT('\0');

			DataType::Swap(this->Data, Temp);
		}

		return true;
	}

public:
	FORCEINLINE TArrayCharType& operator[](int32 Index)
	{
		CORE_CHECKF(IsValidIndex(Index), TEXT("String index out of bounds: Index %i from a string with a length of %i"), Index, Len());
		return Data.GetData()[Index];
	}

	FORCEINLINE const TArrayCharType& operator[](int32 Index) const
	{
		CORE_CHECKF(IsValidIndex(Index), TEXT("String index out of bounds: Index %i from a string with a length of %i"), Index, Len());
		return Data.GetData()[Index];
	}

public:
	FORCEINLINE size_t GetAllocatedSize() const
	{
		return Data.GetAllocatedSize();
	}

	FORCEINLINE void CheckInvariants() const
	{
		int32 Num = Data.Num();
		CORE_CHECK(Num >= 0);
		CORE_CHECK(!Num || !Data.GetData()[Num - 1]);
		CORE_CHECK(Data.Slack() >= 0);
	}

	FORCEINLINE EContainerStatus Empty() noexcept
	{
		return Data.Empty(0);
	}

	FORCEINLINE EContainerStatus Empty(int32 Slack) noexcept
	{
		return Data.Empty(Slack ? Slack + 1 : 0);
	}

	NODISCARD FORCEINLINE bool IsEmpty() const
	{
		return Data.IsEmpty();
	}

	FORCEINLINE bool Reset(int32 NewReservedSize = 0) noexcept
	{
		return ContainerSuccess(Data.Empty(NewReservedSize));
	}

	FORCEINLINE bool Shrink() noexcept
	{
		return Data.Shrink();
	}

	NODISCARD FORCEINLINE bool IsValidIndex(int32 Index) const
	{
		return Index >= 0 && Index < Len();
	}

	NODISCARD FORCEINLINE const TArrayCharType* operator*() const
	{
		return Data.Num() ? Data.GetData() : L"";
	}

	NODISCARD FORCEINLINE DataType& GetCharArray()
	{
		return Data;
	}

	NODISCARD FORCEINLINE const DataType& GetCharArray() const
	{
		return Data;
	}

public:
//	bool AppendChars(const ANSICHAR* Str, int32 Count) noexcept { CheckInvariants(); return AppendCharsImpl<ANSICHAR>(Str, Count); }
//	bool AppendChars(const WIDECHAR* Str, int32 Count) noexcept { CheckInvariants(); return AppendCharsImpl<WIDECHAR>(Str, Count); }
//	bool AppendChars(const UTF8CHAR* Str, int32 Count) noexcept { CheckInvariants(); return AppendCharsImpl<UTF8CHAR>(Str, Count); }

	template <typename CharType>
	FORCEINLINE bool Append(const CharType* Str, int32 Count) noexcept
	{
		CheckInvariants();
		return AppendCharsImpl<CharType>(Str, Count);
	}

	template <typename CharType>
	FORCEINLINE bool Append(CharType* Str) noexcept
	{
		if (Str == nullptr)
		{
			return false;
		}

		using CStringCharType = typename TRemoveConstVolatile<CharType>::Type;
		return AppendCharsImpl<CharType>(Str, TCString<CStringCharType>::Strlen(Str));
	}

	template <typename CharRangeType, typename TEnableIf<TIsCharRangeNotCArray<CharRangeType>::Value>::Type* = nullptr>
	FORCEINLINE bool Append(CharRangeType&& Str) noexcept
	{
		return AppendCharsImpl<CharRangeType>(Argus::String::GetData(Str), Argus::String::GetNum(Str));;
	}

	template <typename CharType, typename TEnableIf<TIsAppendCompatibleWithChar<CharType>::Value>::Type* = nullptr>
	bool AppendChar(CharType InChar) noexcept
	{
		CheckInvariants();

		if (InChar == (CharType)0)
		{
			return false;
		}
			
		return Append(&InChar, 1);
	}

	//template <typename StrType>
	//FORCEINLINE auto operator+=(StrType&& Str)
	//	-> decltype(Append(Forward<StrType>(Str)))
	//{
	//	return Append(Forward<StrType>(Str));
	//}

	//template <typename AppendedCharType, typename TEnableIf<TIsChar<AppendedCharType>::Value>::Type* = nullptr>
	//FORCEINLINE FKrnlString& operator+=(AppendedCharType Char)
	//{
	//	if constexpr (TIsCharEncodingSimplyConvertibleTo<AppendedCharType, TArrayCharType>::Value)
	//	{
	//		return AppendChar((TArrayCharType)Char);
	//	}
	//	else
	//	{
	//		AppendChars(&Char, 1);
	//		return *this;
	//	}
	//}

private:
	template <typename CharType>
	bool AppendCharsImpl(const CharType* Str, int32 Count) noexcept
	{
		if (Count == 0)
		{
			return true;
		}
		if (!Str || Count <= 0)
		{
			return false;
		}

		int32 OldEnd = this->Data.GetNum();
		const int32 ExtraTerminator = OldEnd == 0 ? 1 : 0;
		if (OldEnd < 0 || Count > TNumericLimits<int32>::Max() - OldEnd - ExtraTerminator)
		{
			return false;
		}

		DataType Temp;
		ReturnIfFalse(Temp.InitializeFromCopy(Data.GetData(), OldEnd) == EContainerStatus::Success);
		ReturnIfFalse(Temp.PushUninitialized(Count + ExtraTerminator) != INDEX_NONE);

		OldEnd -= OldEnd ? 1 : 0;

		TArrayCharType* Dest = Temp.GetData() + OldEnd;
		TArrayCharType* NewEnd = FGenericString::Convert<CharType, TArrayCharType>(Dest, Count, Str, Count);
		if (!NewEnd)
		{
			int32 Length = FGenericString::ConvertedLength<TArrayCharType>(Str, Count);
			if (Length == INDEX_NONE)
			{
				return false;
			}

			ReturnIfFalse(Temp.PushUninitialized(Length - Count) != INDEX_NONE);
			Dest = Temp.GetData() + OldEnd;

			NewEnd = FGenericString::Convert(Dest, Length, Str, Count);
			if (NewEnd == nullptr)
			{
				return false;
			}
		}
		else
		{
			int32 NewEndIndex = (int32)(NewEnd - Dest);
			if (NewEndIndex < Count)
			{
				ReturnIfFalse(Temp.SetNumUninitialized(OldEnd + NewEndIndex + 1, false));
			}
		}
			
		*NewEnd = '\0';

		DataType::Swap(this->Data, Temp);
		return true;
	}

public:
	bool InsertAt(int32 Index, TArrayCharType Charactor) noexcept
	{
		if (Charactor == (TArrayCharType)0)
		{
			return false;
		}
		if (!(0 <= Index && Index <= Data.Num()))
		{
			return false;
		}

		if (Data.IsEmpty())
		{
			return this->Append(Charactor);
		}
		else
		{
			return Data.Insert(Charactor, Index) != INDEX_NONE;
		}
	}

	bool InsertAt(int32 Index, const FKrnlString& Charactor) noexcept
	{
		if (!Charactor.Len())
		{
			return false;
		}
		if (!(0 <= Index && Index <= Data.Num()))
		{
			return false;
		}

		if (Data.IsEmpty())
		{
			return this->Append(Charactor);
		}
		else
		{
			return Data.Insert(Charactor.Data.GetData(), Charactor.Len(), Index) != INDEX_NONE;
		}
	}

	FORCEINLINE bool RemoveAt(int32 Index, int32 Count = 1, bool bAllowShrinking = true) noexcept
	{
		return Data.RemoveAt(Index, Count, bAllowShrinking);
	}

	template <typename TCharRangeType, typename TEnableIf<TIsCharRangeNotCArrayNotFKrnlString<TCharRangeType>::Value>::Type* = nullptr>
	FORCEINLINE bool RemoveFormStart(TCharRangeType&& InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase)
	{
		static_assert(TIsSame<typename TElementType<TCharRangeType>::Type, WIDECHAR>::Value, "Expected a range of WIDECHAR");
		return RemoveFormStart(Argus::String::GetData(InPrefix), Argus::String::GetNum(InPrefix), SearchCase);
	}

	FORCEINLINE bool RemoveFromStart(const TArrayCharType* InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase)
	{
		return RemoveFromStart(InPrefix, InPrefix ? TCString<TArrayCharType>::Strlen(InPrefix) : 0, SearchCase);
	}

	FORCEINLINE bool RemoveFromStart(const FKrnlString& InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase)
	{
		return RemoveFromStart(*InPrefix, InPrefix.Len(), SearchCase);
	}

	bool RemoveFromStart(const TArrayCharType* InPrefix, int32 InPrefixLen, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase)
	{
		if (InPrefixLen == 0)
		{
			return 0;
		}

		if (StartsWith(InPrefix, InPrefixLen, SearchCase))
		{
			ReturnIfFalse(RemoveAt(0, InPrefixLen));
			return true;
		}

		return false;
	}

	template <typename TCharRangeType, typename TEnableIf<TIsCharRangeNotCArrayNotFKrnlString<TCharRangeType>::Value>::Type* = nullptr>
	FORCEINLINE bool RemoveFromEnd(TCharRangeType&& InSuffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase)
	{
		static_assert(TIsSame<typename TElementType<TCharRangeType>::Type, WIDECHAR>::Value, "Expected a range of WIDECHAR");
		return RemoveFromEnd(GetData(InSuffix), GetNum(InSuffix), SearchCase);
	}

	FORCEINLINE bool RemoveFromEnd(const WIDECHAR* InSuffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase)
	{
		return RemoveFromEnd(InSuffix, InSuffix ? TCString<TArrayCharType>::Strlen(InSuffix) : 0, SearchCase);
	}

	FORCEINLINE bool RemoveFromEnd(const FKrnlString& InSuffix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase)
	{
		return RemoveFromEnd(*InSuffix, InSuffix.Len(), SearchCase);
	}

	bool RemoveFromEnd(const TArrayCharType* InSuffix, int32 InSuffixLen, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase)
	{
		UNREFERENCED_PARAMETER(InSuffix);
		UNREFERENCED_PARAMETER(InSuffixLen);
		UNREFERENCED_PARAMETER(SearchCase);

		// Not Implementation.
		return false;
	}

	bool PathAppend(const TArrayCharType* Str, int32 StrLength) noexcept
	{
		DataType Temp;
		ReturnIfFalse(Temp.InitializeFromCopy(Data.GetData(), Data.Num()));

		int32 DataNum = Temp.Num();
		if (StrLength == 0)
		{
			if (DataNum > 1 && Temp[DataNum - 2] != TEXT('/') && Temp[DataNum - 2] != TEXT('\\'))
			{
				Temp[DataNum - 1] = TEXT('/');
				ReturnIfFalse(Temp.Push(TEXT('\0')) != INDEX_NONE);
			}
		}
		else
		{
			if (DataNum > 0)
			{
				if (DataNum > 1 && Temp[DataNum - 2] != TEXT('/') && Temp[DataNum - 2] != TEXT('\\') && *Str != TEXT('/'))
				{
					Temp[DataNum - 1] = TEXT('/');
				}
				else
				{
					Temp.RemoveAtLast(false);
					--DataNum;
				}
			}

			Reserve(DataNum + StrLength);

			ReturnIfFalse(Temp.Append(Str, StrLength));
			ReturnIfFalse(Temp.Push(TEXT('\0')) != INDEX_NONE);
		}

		DataType::Swap(this->Data, Temp);
		return true;
	}

	template <typename CharType, typename TEnableIf<TIsAppendCompatibleWithChar<CharType>::Value>::Type* = nullptr>
	static FORCEINLINE bool Concat(const FKrnlString& Lhs, CharType Rhs, FKrnlString& OutStr) noexcept
	{
		Lhs.CheckInvariants();

		FKrnlString OutResult;
		ReturnIfFalse(OutResult.Append(Lhs.Data.GetData(), Lhs.Len()));
		ReturnIfFalse(OutResult.AppendChar(Rhs));

		DataType::Swap(OutResult.Data, OutStr.Data);
		return true;
	}

	template <typename CharType, typename TEnableIf<TIsAppendCompatibleWithChar<CharType>::Value>::Type* = nullptr>
	static FORCEINLINE bool Concat(FKrnlString&& Lhs, CharType Rhs, FKrnlString& OutStr) noexcept
	{
		Lhs.CheckInvariants();

		ReturnIfFalse(Lhs.AppendChar(Rhs));

		// TKrnlArray has no move assignment, so FKrnlString's defaulted move assignment is deleted.
		// Move-construct a temporary and swap its data into OutStr instead.
		FKrnlString OutResult(MoveTempIfPossible(Lhs));
		DataType::Swap(OutResult.Data, OutStr.Data);
		return true;
	}

	//template <typename CharType>
	//NODISCARD FORCEINLINE friend typename TEnableIf<TIsChar<CharType>::Value, FKrnlString>::Type operator+(const FKrnlString& Lhs, CharType Rhs)
	//{
	//	Lhs.CheckInvariants();

	//	FKrnlString Result(Lhs, 1);
	//	Result += Rhs;

	//	return Result;
	//}

	//template <typename CharType>
	//NODISCARD FORCEINLINE friend typename TEnableIf<TIsChar<CharType>::Value, FKrnlString>::Type operator+(FKrnlString&& Lhs, CharType Rhs)
	//{
	//	Lhs.CheckInvariants();

	//	FKrnlString Result(MoveTemp(Lhs), 1);
	//	Result += Rhs;

	//	return Result;
	//}

private:
	static FORCEINLINE bool ConcatRangeFString(const WIDECHAR* Lhs, int32 LhsLen, const FKrnlString& Rhs, FKrnlString& OutStr) noexcept
	{
		if (LhsLen < 0 || (LhsLen > 0 && Lhs == nullptr))
		{
			return false;
		}

		Rhs.CheckInvariants();

		FKrnlString Result;
		ReturnIfFalse(Result.Append(Lhs, LhsLen));
		ReturnIfFalse(Result.Append(Rhs.Data.GetData(), Rhs.Len()));

		DataType::Swap(Result.Data, OutStr.Data);
		return true;
	}

	static FORCEINLINE bool ConcatFStringRange(const FKrnlString& Lhs, const WIDECHAR* Rhs, int32 RhsLen, FKrnlString& OutStr) noexcept
	{
		if (RhsLen < 0 || (RhsLen > 0 && Rhs == nullptr))
		{
			return false;
		}

		Lhs.CheckInvariants();

		FKrnlString Result;
		ReturnIfFalse(Result.Append(Lhs.Data.GetData(), Lhs.Len()));
		ReturnIfFalse(Result.Append(Rhs, RhsLen));

		DataType::Swap(Result.Data, OutStr.Data);
		return true;
	}

	static FORCEINLINE bool ConcatFStringRange(FKrnlString&& Lhs, const WIDECHAR* Rhs, int32 RhsLen, FKrnlString& OutStr) noexcept
	{
		if (RhsLen < 0 || (RhsLen > 0 && Rhs == nullptr))
		{
			return false;
		}

		Lhs.CheckInvariants();

		// Append uses temporary storage, so failure leaves Lhs unchanged.
		ReturnIfFalse(Lhs.Append(Rhs, RhsLen));

		// Move construction transfers the allocation without allocating or returning a failure.
		FKrnlString Result(MoveTempIfPossible(Lhs));
		DataType::Swap(Result.Data, OutStr.Data);
		return true;
	}

public:
	static FORCEINLINE bool Concat(const FKrnlString& Lhs, const FKrnlString& Rhs, FKrnlString& OutStr) noexcept
	{
		Rhs.CheckInvariants();
		return ConcatFStringRange(Lhs, Rhs.Data.GetData(), Rhs.Len(), OutStr);
	}

	static FORCEINLINE bool Concat(FKrnlString&& Lhs, const FKrnlString& Rhs, FKrnlString& OutStr) noexcept
	{
		Rhs.CheckInvariants();
		return ConcatFStringRange(MoveTempIfPossible(Lhs), Rhs.Data.GetData(), Rhs.Len(), OutStr);
	}

	static FORCEINLINE bool Concat(const WIDECHAR* Lhs, int32 LhsLen, const FKrnlString& Rhs, FKrnlString& OutStr) noexcept
	{
		return ConcatRangeFString(Lhs, LhsLen, Rhs, OutStr);
	}

	static FORCEINLINE bool Concat(const FKrnlString& Lhs, const WIDECHAR* Rhs, int32 RhsLen, FKrnlString& OutStr) noexcept
	{
		return ConcatFStringRange(Lhs, Rhs, RhsLen, OutStr);
	}

	static FORCEINLINE bool Concat(FKrnlString&& Lhs, const WIDECHAR* Rhs, int32 RhsLen, FKrnlString& OutStr) noexcept
	{
		return ConcatFStringRange(MoveTempIfPossible(Lhs), Rhs, RhsLen, OutStr);
	}

	static FORCEINLINE bool Concat(const WIDECHAR* Lhs, const FKrnlString& Rhs, FKrnlString& OutStr) noexcept
	{
		if (Lhs == nullptr)
		{
			return false;
		}
		return Concat(Lhs, TCString<WIDECHAR>::Strlen(Lhs), Rhs, OutStr);
	}

	static FORCEINLINE bool Concat(const FKrnlString& Lhs, const WIDECHAR* Rhs, FKrnlString& OutStr) noexcept
	{
		if (Rhs == nullptr)
		{
			return false;
		}
		return Concat(Lhs, Rhs, TCString<WIDECHAR>::Strlen(Rhs), OutStr);
	}

	static FORCEINLINE bool Concat(FKrnlString&& Lhs, const WIDECHAR* Rhs, FKrnlString& OutStr) noexcept
	{
		if (Rhs == nullptr)
		{
			return false;
		}
		return Concat(MoveTempIfPossible(Lhs), Rhs, TCString<WIDECHAR>::Strlen(Rhs), OutStr);
	}

	template <typename T, typename TEnableIf<TIsWideCharRangeNotCArrayNotFKrnlString<T>::Value>::Type* = nullptr>
	static FORCEINLINE bool Concat(T&& Lhs, const FKrnlString& Rhs, FKrnlString& OutStr) noexcept
	{
		return Concat(Argus::String::GetData(Lhs), Argus::String::GetNum(Lhs), Rhs, OutStr);
	}

	template <typename T, typename TEnableIf<TIsWideCharRangeNotCArrayNotFKrnlString<T>::Value>::Type* = nullptr>
	static FORCEINLINE bool Concat(const FKrnlString& Lhs, T&& Rhs, FKrnlString& OutStr) noexcept
	{
		return Concat(Lhs, Argus::String::GetData(Rhs), Argus::String::GetNum(Rhs), OutStr);
	}

	template <typename T, typename TEnableIf<TIsWideCharRangeNotCArrayNotFKrnlString<T>::Value>::Type* = nullptr>
	static FORCEINLINE bool Concat(FKrnlString&& Lhs, T&& Rhs, FKrnlString& OutStr) noexcept
	{
		return Concat(MoveTempIfPossible(Lhs), Argus::String::GetData(Rhs), Argus::String::GetNum(Rhs), OutStr);
	}

public:
	//NODISCARD FORCEINLINE friend FKrnlString operator+(const FKrnlString&  Lhs, const FKrnlString&  Rhs) { return ConcatFF(Lhs, Rhs); }
	//NODISCARD FORCEINLINE friend FKrnlString operator+(		 FKrnlString&& Lhs, const FKrnlString&  Rhs) { return ConcatFF(MoveTemp(Lhs), Rhs); }
	//NODISCARD FORCEINLINE friend FKrnlString operator+(const FKrnlString&  Lhs,		  FKrnlString&& Rhs) { return ConcatFF(Lhs, MoveTemp(Rhs)); }
	//NODISCARD FORCEINLINE friend FKrnlString operator+(		 FKrnlString&& Lhs,		  FKrnlString&& Rhs) { return ConcatFF(MoveTemp(Lhs), MoveTemp(Rhs)); }

	//NODISCARD FORCEINLINE friend FKrnlString operator+(const WIDECHAR* Lhs, const FKrnlString&  Rhs) { return ConcatCF(Lhs, Rhs); }
	//NODISCARD FORCEINLINE friend FKrnlString operator+(const WIDECHAR* Lhs,	      FKrnlString&& Rhs) { return ConcatCF(Lhs, MoveTemp(Rhs)); }
	//NODISCARD FORCEINLINE friend FKrnlString operator+(const FKrnlString&  Lhs, const WIDECHAR* Rhs) { return ConcatFC(Lhs, Rhs); }
	//NODISCARD FORCEINLINE friend FKrnlString operator+(		 FKrnlString&& Lhs, const WIDECHAR* Rhs) { return ConcatFC(MoveTemp(Lhs), Rhs); }

	//template <typename T, typename TEnableIf<TIsWideCharRangeNotCArrayNotFKrnlString<T>::Value>::Type* = nullptr>
	//NODISCARD FORCEINLINE friend FKrnlString operator+(T&& Lhs, const FKrnlString& Rhs) { return ConcatRF(GetData(Lhs), GetNum(Lhs), Rhs); }
	//template <typename T, typename TEnableIf<TIsWideCharRangeNotCArrayNotFKrnlString<T>::Value>::Type* = nullptr>
	//NODISCARD FORCEINLINE friend FKrnlString operator+(T&& Lhs, FKrnlString&& Rhs) { return ConcatRF(GetData(Lhs), GetNum(Lhs), MoveTemp(Rhs)); }
	//template <typename T, typename TEnableIf<TIsWideCharRangeNotCArrayNotFKrnlString<T>::Value>::Type* = nullptr>
	//NODISCARD FORCEINLINE friend FKrnlString operator+(const FKrnlString& Lhs, T&& Rhs) { return ConcatFR(Lhs, GetData(Rhs), GetNum(Rhs)); }
	//template <typename T, typename TEnableIf<TIsWideCharRangeNotCArrayNotFKrnlString<T>::Value>::Type* = nullptr>
	//NODISCARD FORCEINLINE friend FKrnlString operator+(FKrnlString&& Lhs, T&& Rhs) { return ConcatFR(MoveTemp(Lhs), GetData(Rhs), GetNum(Rhs)); }

	static FORCEINLINE void Swap(FKrnlString& First, FKrnlString& Second) noexcept
	{
		DataType::Swap(First.Data, Second.Data);
	}

	FORCEINLINE FKrnlString& operator/=(const WIDECHAR* Str)
	{
		CORE_CHECK(Str);
		PathAppend(Str, TCString<TArrayCharType>::Strlen(Str));
		return *this;
	}

	template <typename CharRangeType, typename TEnableIf<TIsWideCharRangeNotCArray<CharRangeType>::Value>::Type* = nullptr>
	FORCEINLINE FKrnlString& operator/=(CharRangeType&& Str)
	{
		PathAppend(GetData(Str), GetNum(Str));
		return *this;
	}

	template <typename CharType, typename TEnableIf<TIsChar<CharType>::Value>::Type* = nullptr>
	FORCEINLINE FKrnlString& operator/=(const CharType* Str)
	{
		FKrnlString Temp(Str);
		PathAppend(GetData(Temp), GetNum(Str));
		return *this;
	}

	NODISCARD FORCEINLINE friend FKrnlString operator/(const FKrnlString& Lhs, const WIDECHAR* Rhs)
	{
		CORE_CHECK(Rhs);

		int32 StrLength = TCString<TArrayCharType>::Strlen(Rhs);

		FKrnlString Result(Lhs, StrLength + 1);
		Result.PathAppend(Rhs, StrLength);
		return Result;
	}

	NODISCARD FORCEINLINE friend FKrnlString operator/(FKrnlString&& Lhs, const WIDECHAR* Rhs)
	{
		CORE_CHECK(Rhs);

		int32 StrLength = TCString<TArrayCharType>::Strlen(Rhs);

		FKrnlString Result(Lhs, StrLength + 1);
		Result.PathAppend(Rhs, StrLength);
		return Result;
	}

	NODISCARD FORCEINLINE friend FKrnlString operator/(const FKrnlString& Lhs, const FKrnlString& Rhs)
	{
		int32 StrLength = Rhs.Len();

		FKrnlString Result(Lhs, StrLength);
		Result.PathAppend(Rhs.Data.GetData(), StrLength);
		return Result;
	}

	NODISCARD FORCEINLINE friend FKrnlString operator/(FKrnlString&& Lhs, const FKrnlString& Rhs)
	{
		int32 StrLength = Rhs.Len();

		FKrnlString Result(MoveTemp(Lhs), StrLength + 1);
		Result.PathAppend(Rhs.Data.GetData(), StrLength);
		return Result;
	}

	NODISCARD FORCEINLINE friend FKrnlString operator/(const WIDECHAR* Lhs, const FKrnlString& Rhs)
	{
		int32 StrLength = Rhs.Len();

		FKrnlString Result(FKrnlString(Lhs), StrLength + 1);
		Result.PathAppend(Rhs.Data.GetData(), Rhs.Len());
		return Result;
	}

	NODISCARD FORCEINLINE friend bool operator<=(const FKrnlString& Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(*Lhs, *Rhs) <= 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator<=(const FKrnlString& Lhs, const CharType* Rhs)
	{
		return Stricmp(*Lhs, Rhs) <= 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator<=(const CharType* Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(Lhs, *Rhs) <= 0;
	}


	NODISCARD FORCEINLINE friend bool operator<(const FKrnlString& Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(*Lhs, *Rhs) < 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator<(const FKrnlString& Lhs, const CharType* Rhs)
	{
		return Stricmp(*Lhs, Rhs) < 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator<(const CharType* Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(Lhs, *Rhs) < 0;
	}


	NODISCARD FORCEINLINE friend bool operator>=(const FKrnlString& Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(*Lhs, *Rhs) >= 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator>=(const FKrnlString& Lhs, const CharType* Rhs)
	{
		return Stricmp(*Lhs, Rhs) >= 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator>=(const CharType* Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(Lhs, *Rhs) >= 0;
	}


	NODISCARD FORCEINLINE friend bool operator>(const FKrnlString& Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(*Lhs, *Rhs) > 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator>(const FKrnlString& Lhs, const CharType* Rhs)
	{
		return Stricmp(*Lhs, Rhs) > 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator>(const CharType* Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(Lhs, *Rhs) > 0;
	}


	NODISCARD FORCEINLINE bool operator==(const FKrnlString& Rhs) const
	{
		return Equals(Rhs, ESearchCase::IgnoreCase);
	}

	template <typename CharType>
	NODISCARD FORCEINLINE bool operator==(const CharType* Rhs) const
	{
		return Stricmp(**this, Rhs) == 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator==(const CharType* Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(Lhs, *Rhs) == 0;
	}

	NODISCARD FORCEINLINE bool operator!=(const FKrnlString& Rhs) const
	{
		return !Equals(Rhs, ESearchCase::IgnoreCase);
	}

	template <typename CharType>
	NODISCARD FORCEINLINE bool operator!=(const CharType* Rhs) const
	{
		return Stricmp(**this, Rhs) != 0;
	}

	template <typename CharType>
	NODISCARD FORCEINLINE friend bool operator!=(const CharType* Lhs, const FKrnlString& Rhs)
	{
		return Stricmp(Lhs, **Rhs) != 0;
	}

	NODISCARD FORCEINLINE int32 Len() const
	{
		return Data.Num() ? Data.Num() - 1 : 0;
	}

	NODISCARD FORCEINLINE FKrnlString Left(int32 Count) const&
	{
		return FKrnlString(CoreClamp(Count, 0, Len()), **this);
	}

	NODISCARD FORCEINLINE FKrnlString Left(int32 Count)&&
	{
		LeftInline(Count, false);
		return MoveTemp(*this);
	}

	FORCEINLINE void LeftInline(int32 Count, bool bAllowShrinking = true)
	{
		const int32 Length = Len();
		Count = CoreClamp(Count, 0, Length);
		RemoveAt(Count, Length - Count, bAllowShrinking);
	}

	NODISCARD FORCEINLINE FKrnlString LeftChop(int32 Count) const&
	{
		const int32 Length = Len();
		return FKrnlString(CoreClamp(Length - Count, 0, Length), **this);
	}

	NODISCARD FORCEINLINE FKrnlString LeftChop(int32 Count)&&
	{
		LeftChopInline(Count, false);
		return MoveTemp(*this);
	}

	FORCEINLINE void LeftChopInline(int32 Count, bool bAllowShrinking = true)
	{
		const int32 Length = Len();
		RemoveAt(CoreClamp(Length - Count, 0, Length), Count, bAllowShrinking);
	}

	NODISCARD FORCEINLINE FKrnlString Right(int32 Count) const&
	{
		const int32 Length = Len();
		return FKrnlString(**this + Length - CoreClamp(Count, 0, Length));
	}

	NODISCARD FORCEINLINE FKrnlString Right(int32 Count) &&
	{
		RightInline(Count, false);
		return MoveTemp(*this);
	}

	FORCEINLINE void RightInline(int32 Count, bool bAllowShrinking = true)
	{
		const int32 Length = Len();
		RemoveAt(0, Length - CoreClamp(Count, 0, Length), bAllowShrinking);
	}

	NODISCARD FORCEINLINE FKrnlString RightChop(int32 Count) const&
	{
		const int32 Length = Len();
		const int32 Skip = CoreClamp(Count, 0, Length);
		return FKrnlString(Length - Skip, **this + Skip);
	}

	NODISCARD FORCEINLINE FKrnlString RightChop(int32 Count) &&
	{
		RightChopInline(Count, false);
		return MoveTemp(*this);
	}

	FORCEINLINE bool RightChopInline(int32 Count, bool bAllowShrinking = true)
	{
		return RemoveAt(0, Count, bAllowShrinking);
	}

	NODISCARD FKrnlString Mid(int32 Start, int32 Count) const&;
	NODISCARD FKrnlString Mid(int32 Start, int32 Count) &&;

	NODISCARD FORCEINLINE FKrnlString Mid(int32 Start) const& { return RightChop(Start); }
	NODISCARD FORCEINLINE FKrnlString Mid(int32 Start) && { return ((FKrnlString&&)*this).RightChop(Start); }

	FORCEINLINE void MidInline(int32 Start, int32 Count, bool bAllowShrinking = true)
	{
		if (Count != MAX_int32 && (int64)(Start)+Count < MAX_int32)
		{
			LeftInline(Count + Start, false);
		}

		RightChopInline(Start, bAllowShrinking);
	}

	template <typename TCharType, typename TEnableIf<TIsWideCharRangeNotCArrayNotFKrnlString<TCharType>::Value>::Type* = nullptr>
	NODISCARD FORCEINLINE int32 Find(
		TCharType&& SubStr,
		ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
		ESearchDir::Type SearchDir = ESearchDir::FromStart,
		int32 StartPosition = INDEX_NONE) const&
	{
		static_assert(TIsSame<typename TElementType<TCharType>::Type, WIDECHAR>::Value, "Expected a range of WIDECHAR");
		return Find(GetData(SubStr), GetNum(SubStr), SearchCase, SearchDir, StartPosition);
	}

	NODISCARD FORCEINLINE int32 Find(
		const WIDECHAR* SubStr,
		ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
		ESearchDir::Type SearchDir = ESearchDir::FromStart,
		int32 StartPosition = INDEX_NONE) const
	{
		return SubStr ? Find(SubStr, TCString<TArrayCharType>::Strlen(SubStr), SearchCase, SearchDir, StartPosition) : INDEX_NONE;
	}

	NODISCARD FORCEINLINE int32 Find(
		const FKrnlString& SubStr,
		ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
		ESearchDir::Type SearchDir = ESearchDir::FromStart,
		int32 StartPosition = INDEX_NONE) const
	{
		return Find(*SubStr, SubStr.Len(), SearchCase, SearchDir, StartPosition);
	}

	NODISCARD int32 Find(
		const WIDECHAR* SubStr,
		int32 InSubStrLen,
		ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
		ESearchDir::Type SearchDir = ESearchDir::FromStart,
		int32 StartPosition = INDEX_NONE) const;

	template <typename TCharRangeType, typename TEnableIf<TIsWideCharRangeNotCArrayNotFKrnlString<TCharRangeType>::Value>::Type* = nullptr>
	NODISCARD FORCEINLINE bool Contains(TCharRangeType&& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
		ESearchDir::Type SearchDir = ESearchDir::FromStart) const
	{
		static_assert(TIsSame<typename TElementType<TCharRangeType>::Type, WIDECHAR>::Value, "Expected a range of WIDECHAR");
		return Find(Forward<TCharRangeType>(SubStr), SearchCase, SearchDir) != INDEX_NONE;
	}

	NODISCARD FORCEINLINE bool Contains(const WIDECHAR* SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
		ESearchDir::Type SearchDir = ESearchDir::FromStart) const
	{
		return Find(SubStr, SearchCase, SearchDir) != INDEX_NONE;
	}

	NODISCARD FORCEINLINE bool Contains(const FKrnlString& SubStr, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase,
		ESearchDir::Type SearchDir = ESearchDir::FromStart) const
	{
		return Find(*SubStr, SearchCase, SearchDir) != INDEX_NONE;
	}

	NODISCARD FORCEINLINE bool Contains(const WIDECHAR* SubStr, int32 SubStrLen,
		ESearchCase::Type SearchCase = ESearchCase::IgnoreCase, ESearchDir::Type SearchDir = ESearchDir::FromStart) const
	{
		return Find(SubStr, SubStrLen, SearchCase, SearchDir) != INDEX_NONE;
	}

	NODISCARD FORCEINLINE bool FindChar(WIDECHAR InChar, int32& Index) const
	{
		return Data.Find(InChar, Index);
	}

	NODISCARD FORCEINLINE bool FindLastChar(WIDECHAR InChar, int32& Index) const
	{
		for (Index = Len(); Index > 0;)
		{
			--Index;
			if (Data.GetData()[Index] == InChar)
			{
				return true;
			}
		}
		Index = INDEX_NONE;
		return false;
	}

	NODISCARD FORCEINLINE bool Equals(const FKrnlString& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSecsitive) const
	{
		int32 Num = Data.Num();
		int32 OtherNum = Other.Data.Num();

		if (Num != OtherNum)
		{
			// special case FKrnlString() == FKrnlString("")
			return Num + OtherNum == 1;
		}
		else if (Num > 1)
		{
			return SearchCase == ESearchCase::CaseSecsitive
				? TCString<TArrayCharType>::Strcmp(Data.GetData(), Other.Data.GetData()) == 0
				: TCString<TArrayCharType>::Stricmp(Data.GetData(), Other.Data.GetData()) == 0;
		}

		return true;
	}

	FORCEINLINE int32 Compare(const FKrnlString& Other, ESearchCase::Type SearchCase = ESearchCase::CaseSecsitive) const
	{
		return SearchCase == ESearchCase::CaseSecsitive
			? TCString<TArrayCharType>::Strcmp (**this, *Other)
			: TCString<TArrayCharType>::Stricmp(**this, *Other);
	}

	bool Split(const FKrnlString& InS, FKrnlString* LeftS, FKrnlString* RightS, ESearchCase::Type SearchCase,
		ESearchDir::Type SearchDir = ESearchDir::FromStart) const;

	FORCEINLINE bool Split(const FKrnlString InS, FKrnlString* LeftS, FKrnlString* RightS) const
	{
		return Split(InS, LeftS, RightS, ESearchCase::IgnoreCase);
	}

	bool Split(const FKrnlString& InS, _Out_ TKrnlArray<PoolType, FKrnlString, AllocatorType>* OutS, ESearchCase::Type SearchCase,
		ESearchDir::Type SearchDir = ESearchDir::FromStart) const;

	FORCEINLINE bool Split(const FKrnlString& InS, _Out_ TKrnlArray<PoolType, FKrnlString, AllocatorType>* OutS) const
	{
		return Split(InS, OutS, ESearchCase::IgnoreCase);
	}

	NODISCARD FKrnlString ToUpper() const&;
	NODISCARD FKrnlString ToUpper() &&;

	void ToUpperInline();

	NODISCARD FKrnlString ToLower() const&;
	NODISCARD FKrnlString ToLower() &&;

	void ToLowerInline();

	NODISCARD FKrnlString LeftPad(int32 ChCount) const;
	NODISCARD FKrnlString RightPad(int32 ChCount) const;

	NODISCARD bool IsNumeric() const;

	template <typename FmtType, typename... Types>
	NODISCARD static FORCEINLINE FKrnlString Printf(const FmtType& Fmt, Types... Args)
	{
		static_assert(TIsArrayOrRefOfTypeByPredicate<FmtType, TIsFormatCompatibleWithWideChar>::Value, "Formatting string must be WIDECHAR array.");
		static_assert(TAnd<TIsValidVariadicFunctionArg<Types>...>::Value, "Invalid argument(s) passed to FKrnlString::Printf");

		return PrintfImpl((const WIDECHAR*)Fmt, Args...);
	}

	template <typename FmtType, typename... Types>
	FKrnlString& Appendf(const FmtType& Fmt, Types... Args)
	{
		static_assert(TIsArrayOrRefOfTypeByPredicate<FmtType, TIsFormatCompatibleWithWideChar>::Value, "Formatting string must be a WIDECHAR array.");
		static_assert(TAnd<TIsValidVariadicFunctionArg<Types>...>::Value, "Invalid argument(s) passed to FKrnlString::Appendf.");

		AppendImpl(*this, (const WIDECHAR*)Fmt, Args...);
		return *this;
	}

	/**
	 * @brief Replaces the first matching character without changing the string length.
	 * @param From Character to find; must not be the null terminator.
	 * @param To Replacement character; must differ from From and must not be null.
	 * @param OutFindIndex Optional output for the replaced index. Receives
	 * INDEX_NONE when no match exists; remains unchanged for invalid characters.
	 * @return True if a character was replaced, otherwise false.
	 */
	FORCEINLINE bool Replace(WIDECHAR From, WIDECHAR To, SizeType* const OutFindIndex) noexcept
	{
		if (From == (WIDECHAR)0 || To == (WIDECHAR)0)
		{
			return false;
		}
		if (From == To)
		{
			return false;
		}

		SizeType Index = 0;
		bool bFindSuccess = FindChar(From, Index);
		if (!bFindSuccess)
		{
			if (OutFindIndex)
			{
				*OutFindIndex = Index;
			}
			return false;
		}

		Data.GetData()[Index] = To;
		if (OutFindIndex)
		{
			*OutFindIndex = Index;
		}

		return true;
	}

	/**
	 * @brief Replaces every matching character in one pass.
	 * @param From Character to find; must not be the null terminator.
	 * @param To Replacement character; must differ from From and must not be null.
	 * @param OutFindCount Optional output for the number of replacements; set to
	 * zero when no match exists or the characters are invalid.
	 * @return True if at least one character was replaced, otherwise false.
	 */
	FORCEINLINE bool ReplaceAll(WIDECHAR From, WIDECHAR To, SizeType* const OutFindCount) noexcept
	{
		if (OutFindCount)
		{
			*OutFindCount = 0;
		}
		if (From == (WIDECHAR)0 || To == (WIDECHAR)0 || From == To)
		{
			return false;
		}

		SizeType FindCount = 0;
		for (SizeType Index = 0; Index < Len(); ++Index)
		{
			if (Data.GetData()[Index] == From)
			{
				Data.GetData()[Index] = To;
				++FindCount;
			}
		}

		if (OutFindCount)
		{
			*OutFindCount = FindCount;
		}

		return FindCount != 0;
	}

private:
	static FKrnlString VARARGS PrintfImpl(const WIDECHAR* Fmt, ...);

	static void VARARGS AppendImpl(FKrnlString& AppendToMe, const WIDECHAR* Fmt, ...);

public:
	//NODISCARD static FKrnlString Format(const WIDECHAR* InFormatString, ...);
	//NODISCARD static FKrnlString Format(const WIDECHAR* InFormatString, ...);

	NODISCARD static FKrnlString Chr(WIDECHAR Char);
	NODISCARD static FKrnlString ChrN(int32 NumCharactor, WIDECHAR Char);

	template <typename TCharRangeType, typename TEnableIf<TIsCharRangeNotCArrayNotFKrnlString<TCharRangeType>::Value>::Type* = nullptr>
	NODISCARD FORCEINLINE bool StartsWith(TCharRangeType&& InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const
	{
		static_assert(TIsSame<typename TElementType<TCharRangeType>::Type, WIDECHAR>::Value, "Expected a range of WIDECHAR");
		return StartsWith(GetData(InPrefix), GetNum(InPrefix), SearchCase);
	}

	NODISCARD FORCEINLINE bool StartsWith(const WIDECHAR* InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const
	{
		return StartsWith(InPrefix, InPrefix ? TCString<TArrayCharType>::Strlen(InPrefix) : 0, SearchCase);
	}

	NODISCARD FORCEINLINE bool StartsWith(const FKrnlString& InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const
	{
		return StartsWith(*InPrefix, InPrefix.Len(), SearchCase);
	}

	NODISCARD bool StartsWith(const WIDECHAR* InPrefix, int32 InPrefixLen, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;


	template <typename TCharRangeType, typename TEnableIf<TIsCharRangeNotCArrayNotFKrnlString<TCharRangeType>::Value>::Type* = nullptr>
	NODISCARD FORCEINLINE bool EndsWith(TCharRangeType&& InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const
	{
		static_assert(TIsSame<typename TElementType<TCharRangeType>::Type, WIDECHAR>::Value, "Expected a range of WIDECHAR");
		return EndsWith(GetData(InPrefix), GetNum(InPrefix), SearchCase);
	}

	NODISCARD FORCEINLINE bool EndsWith(const WIDECHAR* InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const
	{
		return EndsWith(InPrefix, InPrefix ? TCString<TArrayCharType>::Strlen(InPrefix) : 0, SearchCase);
	}

	NODISCARD FORCEINLINE bool EndsWith(const FKrnlString& InPrefix, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const
	{
		return EndsWith(*InPrefix, InPrefix.Len(), SearchCase);
	}

	NODISCARD bool EndsWith(const WIDECHAR* InSuffix, int32 InSuffixLen, ESearchCase::Type SearchCase = ESearchCase::IgnoreCase) const;

public:
	void Reserve(int32 CharacterCount);

private:
	DataType Data;
};

namespace Argus::String
{
	template <EPoolType PoolType>
	NODISCARD FORCEINLINE WIDECHAR* GetData(FKrnlString<PoolType>& String)
	{
		return String.GetCharArray().GetData();
	}
	template <EPoolType PoolType>
	NODISCARD FORCEINLINE const WIDECHAR* GetData(const FKrnlString<PoolType>& String)
	{
		return String.GetCharArray().GetData();
	}
	template <EPoolType PoolType>
	NODISCARD FORCEINLINE int32 GetNum(const FKrnlString<PoolType>& String)
	{
		return String.Len();
	}
}
