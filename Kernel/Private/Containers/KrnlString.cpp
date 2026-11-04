#include "Kernel/Public/Containers/KrnlString.h"
#include "Core/Public/Misc/Char.h"
#include "Core/Public/Misc/CString.h"
#include "Core/Public/Misc/GenericString.h"
#include "Core/Public/Math/MathFwd.h"

#include <stdarg.h>
#include <ntstrsafe.h>

#include "Core/Public/Misc/VarArgs.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"

template <EPoolType PoolType, typename CharType>
void AppendCharacters(TArray<PoolType, WIDECHAR>& Out, const CharType* Str, int32 Count)
{
	CORE_CHECK(Count >= 0);

	if (!Count)
	{
		return;
	}

	CORE_CHECK(Str);

	int32 OldEnd = Out.Num();

	Out.PushUninitialized(Count + (OldEnd ? 0 : 1));
	OldEnd -= OldEnd ? 1 : 0;

	WIDECHAR* Dest = Out.GetData() + OldEnd;
	WIDECHAR* NewEnd = FGenericString::Convert(Dest, Count, Str, Count);
	if (!NewEnd)
	{
		int32 Length = FGenericString::ConvertedLength<WIDECHAR>(Str, Count);

		Out.PushUninitialized(Length - Count);
		Dest = Out.GetData() + OldEnd;

		NewEnd = FGenericString::Convert(Dest, Length, Str, Count);
		CORE_CHECK(NewEnd);
	}
	else
	{
		int32 NewEndIndex = (int32)(NewEnd - Dest);
		if (NewEndIndex < Count)
		{
			Out.SetNumUninitialized(OldEnd + NewEndIndex, false);
		}
	}

	*NewEnd = '\0';
}

namespace Argus::String::Private
{
	template <EPoolType PoolType, typename CharType>
	FORCEINLINE void ConstructFromCString(_Out_ TArray<PoolType, WIDECHAR>& Data, const CharType* Src)
	{
		if (Src && *Src)
		{
			int32 SrcLen = TCString<CharType>::Strlen(Src) + 1;
			int32 DestLen = FGenericString::ConvertedLength<WIDECHAR>(Src, SrcLen);
			Data.Reserve(SrcLen);
			Data.PushUninitialized(SrcLen);

			FGenericString::Convert(Data.GetData(), DestLen, Src, SrcLen);
		}
	}

	template <EPoolType PoolType, typename CharType>
	FORCEINLINE void ConstructWithLength(_Out_ TArray<PoolType, WIDECHAR>& Data, int32 InCount, const CharType* InSrc)
	{
		if (InSrc)
		{
			int32 DestLen = FGenericString::ConvertedLength<WIDECHAR>(InSrc, InCount);
			if (DestLen > 0 && *InSrc)
			{
				Data.Reserve(DestLen + 1);
				Data.PushUninitialized(DestLen + 1);

				FGenericString::Convert(Data.GetData(), DestLen, InSrc, InCount);
				*(Data.GetData() + Data.Num() - 1) = TEXT('\0');
			}
		}
	}

	template <EPoolType PoolType, typename CharType>
	FORCEINLINE void ConstructWithSlack(_Out_ TArray<PoolType, WIDECHAR>& Data, const CharType* Src, int32 ExtraSlack)
	{
		if (Src && *Src)
		{
			int32 SrcLen = Strlen(Src) + 1;
			int32 DestLen = FGenericString::ConvertedLength<CharType>(Src, SrcLen);
			Data.Reserve(DestLen + ExtraSlack);
			Data.PushUninitialized(DestLen);

			FGenericString::Convert(Data.GetData(), DestLen, Src, SrcLen);
		}
		else if (ExtraSlack > 0)
		{
			Data.Reserve(ExtraSlack + 1);
		}
	}
}

//template <EPoolType PoolType> FKrnlString<PoolType>::FKrnlString(const ANSICHAR* Str) { Argus::String::Private::ConstructFromCString(Data, Str); }
//template <EPoolType PoolType> FKrnlString<PoolType>::FKrnlString(const WIDECHAR* Str) { Argus::String::Private::ConstructFromCString(Data, Str); }
//template <EPoolType PoolType> FKrnlString<PoolType>::FKrnlString(const UTF8CHAR* Str) { Argus::String::Private::ConstructFromCString(Data, Str); }
//template <EPoolType PoolType> FKrnlString<PoolType>::FKrnlString(int32 Len, const ANSICHAR* Str) { Argus::String::Private::ConstructWithLength(Data, Len, Str); }
//template <EPoolType PoolType> FKrnlString<PoolType>::FKrnlString(int32 Len, const WIDECHAR* Str) { Argus::String::Private::ConstructWithLength(Data, Len, Str); }
//template <EPoolType PoolType> FKrnlString<PoolType>::FKrnlString(int32 Len, const UTF8CHAR* Str) { Argus::String::Private::ConstructWithLength(Data, Len, Str); }
//template <EPoolType PoolType> FKrnlString<PoolType>::FKrnlString(const ANSICHAR* Str, int32 ExtraSlack) { Argus::String::Private::ConstructWithSlack(Data, Str, ExtraSlack); }
//template <EPoolType PoolType> FKrnlString<PoolType>::FKrnlString(const WIDECHAR* Str, int32 ExtraSlack) { Argus::String::Private::ConstructWithSlack(Data, Str, ExtraSlack); }
//template <EPoolType PoolType> FKrnlString<PoolType>::FKrnlString(const UTF8CHAR* Str, int32 ExtraSlack) { Argus::String::Private::ConstructWithSlack(Data, Str, ExtraSlack); }

template <EPoolType PoolType>
FKrnlString<PoolType>& FKrnlString<PoolType>::operator=(const WIDECHAR* Str)
{
	if (Data.GetData() != Str)
	{
		int32 Len = (Str && *Str) ? TCString<WIDECHAR>::Strlen(Str) + 1 : 0;
		Data.Empty(Len);
		Data.PushUninitialized(Len);

		if (Len)
		{
			FMemory::Memcpy(Data.GetData(), Str, Len * sizeof(WIDECHAR));
		}
	}

	return *this;
}

//template <EPoolType PoolType>
//void FKrnlString<PoolType>::AssignRange(const WIDECHAR* OtherData, int32 OtherLen)
//{
//	if (!OtherLen)
//	{
//		Empty(0);
//	}
//	else
//	{
//		const int32 ThisLen = Len();
//		if (OtherLen <= ThisLen)
//		{
//			WIDECHAR* DataPtr = Data.GetData();
//			FMemory::Memmove(DataPtr, OtherData, OtherLen * sizeof(WIDECHAR));
//			DataPtr[OtherLen] = TEXT('\n');
//			Data.RemoveAt(OtherLen + 1, ThisLen - OtherLen);
//		}
//		else
//		{
//			Data.Empty(0);
//			Data.PushUninitialized(OtherLen + 1);
//			WIDECHAR* DataPtr = Data.GetData();
//			FMemory::Memmove(DataPtr, OtherData, OtherLen * sizeof(WIDECHAR));
//			DataPtr[OtherLen] = TEXT('\n');
//		}
//	}
//}

//template <EPoolType PoolType>
//void FKrnlString<PoolType>::Reset(int32 NewReservedSize)
//{
//}

//template <EPoolType PoolType>
//void FKrnlString<PoolType>::AppendChars(const ANSICHAR* Str, int32 Count)
//{
//	CheckInvariants();
//	AppendCharacters<PoolType, ANSICHAR>(Data, Str, Count);
//}
//
//template <EPoolType PoolType>
//void FKrnlString<PoolType>::AppendChars(const WIDECHAR* Str, int32 Count)
//{
//	CheckInvariants();
//	AppendCharacters<PoolType, WIDECHAR>(Data, Str, Count);
//}
//
//template <EPoolType PoolType>
//void FKrnlString<PoolType>::AppendChars(const UTF8CHAR* Str, int32 Count)
//{
//	CheckInvariants();
//	AppendCharacters<PoolType, UTF8CHAR>(Data, Str, Count);
//}

//template <EPoolType PoolType>
//FKrnlString<PoolType>& FKrnlString<PoolType>::AppendChar(WIDECHAR InChar)
//{
//	CheckInvariants();
//
//	if (InChar != 0)
//	{
//		int32 InsertIndex = (Data.Num() > 0) ? Data.Num() - 1 : 0;
//		int32 InsertCount = (Data.Num() > 0) ? 1 : 2;
//
//		Data.PushUninitialized(InsertCount);
//		Data[InsertIndex] = InChar;
//		Data[InsertIndex + 1] = TEXT('\0');
//	}
//
//	return *this;
//}

//template <EPoolType PoolType>
//void FKrnlString<PoolType>::InsertAt(int32 Index, WIDECHAR Charactor)
//{
//	if (Charactor != 0)
//	{
//		if (Data.Num() != 0)
//		{
//			*this += Charactor;
//		}
//		else
//		{
//			Data.Insert(Charactor, Index);
//		}
//	}
//}

//template <EPoolType PoolType>
//void FKrnlString<PoolType>::InsertAt(int32 Index, const FKrnlString& Charactors)
//{
//	if (Charactors.Len())
//	{
//		if (Data.Num())
//		{
//			*this += Charactors;
//		}
//		else
//		{
//			Data.Insert(Charactors.Data.GetData(), Charactors.Len(), Index);
//		}
//	}
//
//}

//template <EPoolType PoolType>
//void FKrnlString<PoolType>::RemoveAt(int32 Index, int32 Count, bool bAllowShrinking)
//{
//	Data.RemoveAt(Index, Count, bAllowShrinking);
//}

//template <EPoolType PoolType>
//bool FKrnlString<PoolType>::RemoveFromStart(const WIDECHAR* InPrefix, int32 InPrefixLen, ESearchCase::Type SearchCase)
//{
//	if (InPrefixLen == 0)
//	{
//		return 0;
//	}
//
//	if (StartsWith(InPrefix, InPrefixLen, SearchCase))
//	{
//		RemoveAt(0, InPrefixLen);
//		return true;
//	}
//
//	return false;
//}

//template <EPoolType PoolType>
//bool FKrnlString<PoolType>::RemoveFromEnd(const WIDECHAR* InSuffix, int32 InSuffixLen, ESearchCase::Type SearchCase)
//{
//	return false;
//}

//template <EPoolType PoolType>
//void FKrnlString<PoolType>::PathAppend(const WIDECHAR* Str, int32 StrLength)
//{
//	int32 DataNum = Data.Num();
//	if (StrLength == 0)
//	{
//		if (DataNum > 1 && Data[DataNum - 2] != TEXT('/') && Data[DataNum - 2] != TEXT('\\'))
//		{
//			Data[DataNum - 1] = TEXT('/');
//			Data.Add(TEXT('\0'));
//		}
//	}
//	else
//	{
//		if (DataNum > 0)
//		{
//			if (DataNum > 1 && Data[DataNum - 2] != TEXT('/') && Data[DataNum - 2] != TEXT('\\') && *Str != TEXT('/'))
//			{
//				Data[DataNum - 1] = TEXT('/');
//			}
//			else
//			{
//				Data.Pop(false);
//				--DataNum;
//			}
//		}
//
//		Reserve(DataNum + StrLength);
//		Data.Append(Str, StrLength);
//		Data.Add(TEXT('\0'));
//	}
//}

//namespace Argus::String::Private
//{
//	template <EPoolType PoolType, typename LhsType, typename RhsType>
//	NODISCARD FORCEINLINE FKrnlString<PoolType> ConcatFKrnlStrings(LhsType&& Lhs, RhsType&& Rhs)
//	{
//		Lhs.CheckInvariants();
//		Rhs.CheckInvariants();
//
//		if (Lhs.IsEmpty())
//		{
//			return Forward<RhsType>(Rhs);
//		}
//
//		int32 RhsLen = Rhs.Len();
//
//		FKrnlString<PoolType> Result(Forward<LhsType>(Lhs), RhsLen);
//		Result.AppendChars(Rhs.GetCharArray().GetData(), RhsLen);
//
//		return Result;
//	}
//
//	template <EPoolType PoolType, typename RhsType>
//	NODISCARD FORCEINLINE FKrnlString<PoolType> ConcatRangeFKrnlString(const WIDECHAR* Lhs, int32 LhsLen, RhsType&& Rhs)
//	{
//		CORE_CHECK(LhsLen >= 0);
//		Rhs.CheckInvariants();
//		if (LhsLen == 0)
//		{
//			return Forward<RhsType>(Rhs);
//		}
//
//		int32 RhsLen = Rhs.Len();
//
//		FKrnlString<PoolType> Result;
//		Result.GetCharArray().Reserve(LhsLen + RhsLen + 1);
//		Result.GetCharArray().PushUninitialized(LhsLen + RhsLen + 1);
//
//		WIDECHAR* ResultData = Result.GetCharArray().GetData();
//		CopyAssignItems(ResultData, Lhs, LhsLen);
//		CopyAssignItems(ResultData + LhsLen, Rhs.GetCharArray().GetData(), RhsLen);
//		*(ResultData + LhsLen + RhsLen) = TEXT('\0');
//
//		return Result;
//	}
//
//	template <EPoolType PoolType, typename LhsType>
//	NODISCARD FORCEINLINE FKrnlString<PoolType> ConcatFKrnlStringRange(LhsType&& Lhs, const WIDECHAR* Rhs, int32 RhsLen)
//	{
//		Lhs.CheckInvariants();
//		CORE_CHECK(RhsLen >= 0);
//		if (RhsLen == 0)
//		{
//			return Forward<LhsType>(Lhs);
//		}
//
//		FKrnlString<PoolType> Result(Forward<LhsType>(Lhs), RhsLen);
//		Result.AppendChars(Rhs, RhsLen);
//
//		return Result;
//	}
//
//	template <EPoolType PoolType, typename RhsType>
//	NODISCARD FORCEINLINE FKrnlString<PoolType> ConcatCStringFKrnlString(const WIDECHAR* Lhs, RhsType&& Rhs)
//	{
//		CORE_CHECK(Lhs);
//		if (!Lhs)
//		{
//			return Forward<RhsType>(Rhs);
//		}
//
//		return ConcatRangeFKrnlString<PoolType>(Lhs, TCString<WIDECHAR>::Strlen(Lhs), Forward<RhsType>(Rhs));
//	}
//
//	template <EPoolType PoolType, typename LhsType>
//	NODISCARD FORCEINLINE FKrnlString<PoolType> ConcatFKrnlStringCString(LhsType&& Lhs, const WIDECHAR* Rhs)
//	{
//		CORE_CHECK(Rhs);
//		if (!Rhs)
//		{
//			return Forward<LhsType>(Lhs);
//		}
//
//		return ConcatFKrnlStringRange<PoolType>(Forward<LhsType>(Lhs), Rhs, TCString<WIDECHAR>::Strlen(Rhs));
//	}
//}
//
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatFF(const FKrnlString& Lhs, const FKrnlString& Rhs) 
//{ 
//	return Argus::String::Private::ConcatFKrnlStrings<PoolType>(Lhs, Rhs); 
//}
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatFF(FKrnlString&& Lhs, const FKrnlString& Rhs) 
//{ 
//	return Argus::String::Private::ConcatFKrnlStrings<PoolType>(MoveTemp(Lhs), Rhs); 
//}
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatFF(const FKrnlString& Lhs, FKrnlString&& Rhs) 
//{ 
//	return Argus::String::Private::ConcatFKrnlStrings<PoolType>(Lhs, MoveTemp(Rhs)); 
//}
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatFF(FKrnlString&& Lhs, FKrnlString&& Rhs) 
//{ 
//	return Argus::String::Private::ConcatFKrnlStrings<PoolType>(MoveTemp(Lhs), MoveTemp(Rhs)); 
//}
//
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatFC(const FKrnlString& Lhs, const WIDECHAR* Rhs) 
//{ 
//	return Argus::String::Private::ConcatFKrnlStringCString<PoolType>(Lhs, Rhs); 
//}
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatFC(FKrnlString&& Lhs, const WIDECHAR* Rhs) 
//{ 
//	return Argus::String::Private::ConcatFKrnlStringCString<PoolType>(MoveTemp(Lhs), Rhs); 
//}
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatCF(const WIDECHAR* Lhs, const FKrnlString& Rhs) 
//{ 
//	return Argus::String::Private::ConcatCStringFKrnlString<PoolType>(Lhs, Rhs); 
//}
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatCF(const WIDECHAR* Lhs, FKrnlString&& Rhs) 
//{ 
//	return Argus::String::Private::ConcatCStringFKrnlString<PoolType>(Lhs, MoveTemp(Rhs)); 
//}
//
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatFR(const FKrnlString& Lhs, const WIDECHAR* Rhs, int32 RhsLen) 
//{ 
//	return Argus::String::Private::ConcatFKrnlStringRange<PoolType>(Lhs, Rhs, RhsLen); 
//}
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatFR(FKrnlString&& Lhs, const WIDECHAR* Rhs, int32 RhsLen) 
//{ 
//	return Argus::String::Private::ConcatFKrnlStringRange<PoolType>(MoveTemp(Lhs), Rhs, RhsLen); 
//}
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatRF(const WIDECHAR* Lhs, int32 LhsLen, const FKrnlString& Rhs) 
//{ 
//	return Argus::String::Private::ConcatRangeFKrnlString<PoolType>(Lhs, LhsLen, Rhs); 
//}
//template <EPoolType PoolType>
//NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ConcatRF(const WIDECHAR* Lhs, int32 LhsLen, FKrnlString&& Rhs) 
//{ 
//	return Argus::String::Private::ConcatRangeFKrnlString<PoolType>(Lhs, LhsLen, MoveTemp(Rhs)); 
//}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::Mid(int32 Start, int32 Count) const&
{
	if (Count >= 0)
	{
		const int32 Length = Len();
		const int32 RequestedStart = Start;
		Start = CoreClamp(Start, 0, Length);
		const int32 End = (int32)CoreClamp((int64)Count + RequestedStart, (int64)Start, (int64)Length);
		return FKrnlString<PoolType>(End - Start, **this + Start);
	}

	return FKrnlString<PoolType>();
}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::Mid(int32 Start, int32 Count)&&
{
	MidInline(Start, Count, false);
	return MoveTemp(*this);
}

template <EPoolType PoolType>
NODISCARD int32 FKrnlString<PoolType>::Find(const WIDECHAR* SubStr, int32 SubStrLen, ESearchCase::Type SearchCase, ESearchDir::Type SearchDir, int32 StartPosition) const
{
	CORE_CHECKF(SubStrLen >= 0, TEXT("Invalid SubStrLen %d"), SubStrLen);

	if (SearchDir == ESearchDir::FromStart)
	{
		const WIDECHAR* Start = **this;
		int32 RemainingLength = Len();
		if (StartPosition != INDEX_NONE && RemainingLength > 0)
		{
			const WIDECHAR* End = Start + RemainingLength;
			Start += CoreClamp(StartPosition, 0, RemainingLength - 1);
			RemainingLength = static_cast<int32>(End - Start);
		}

		const WIDECHAR* Tmp = (SearchCase == ESearchCase::IgnoreCase)
			? TCString<WIDECHAR>::Strnistr(Start, RemainingLength, SubStr, SubStrLen)
			: TCString<WIDECHAR>::Strnstr(Start, RemainingLength, SubStr, SubStrLen);

		return Tmp ? static_cast<int32>(Tmp - **this) : INDEX_NONE;
	}
	else
	{
		if (SearchCase == ESearchCase::IgnoreCase)
		{
			return ToUpper().Find(FKrnlString<PoolType>(SubStrLen, SubStr).ToUpper(), ESearchCase::CaseSecsitive, SearchDir, StartPosition);
		}
		else
		{
			const int32 SearchStringLength = CoreMax(1, SubStrLen);

			if (StartPosition == INDEX_NONE || StartPosition >= Len())
			{
				StartPosition = Len();
			}

			for (int32 I = StartPosition - SearchStringLength; I >= 0; --I)
			{
				int32 J = 0;
				for (; J != SubStrLen; ++J)
				{
					if ((*this)[I + J] != SubStr[J])
					{
						break;
					}
				}

				if (J == SubStrLen)
				{
					return I;
				}
			}

			return INDEX_NONE;
		}
	}
}

template <EPoolType PoolType>
bool FKrnlString<PoolType>::Split(const FKrnlString& InS, FKrnlString* LeftS, FKrnlString* RightS, ESearchCase::Type SearchCase, ESearchDir::Type SearchDir) const
{
	CORE_CHECK(LeftS != RightS || LeftS == nullptr);

	int32 InPos = Find(InS, SearchCase, SearchDir);

	if (InPos < 0) { return false; }

	if (LeftS)
	{
		if (LeftS != this)
		{
			*LeftS = Left(InPos);
			if (RightS) { *RightS = Mid(InPos + InS.Len()); }
		}
		else
		{
			// we know that RightS can't be this so we can safely modify it before we deal with LeftS
			if (RightS) { *RightS = Mid(InPos + InS.Len()); }
			*LeftS = Left(InPos);
		}
	}
	else if (RightS)
	{
		*RightS = Mid(InPos + InS.Len());
	}

	return true;
}

template <EPoolType PoolType>
bool FKrnlString<PoolType>::Split(const FKrnlString& InS, _Out_ TKrnlArray<PoolType, FKrnlString, AllocatorType>* OutS, ESearchCase::Type SearchCase, ESearchDir::Type SearchDir) const
{
	CORE_CHECK(OutS == nullptr);

	if (SearchDir == ESearchDir::FromStart)
	{
		int32 StartPos = INDEX_NONE;
		int32 EndPos = Find(InS, SearchCase, SearchDir, StartPos);

		if (EndPos < 0) { return false; }

		while (EndPos > 0)
		{
			OutS->Add(Mid(StartPos, EndPos - StartPos));

			StartPos = EndPos + InS.Len();
			EndPos = Find(*InS, SearchCase, SearchDir, StartPos);
		}

		if (StartPos != Len())
		{
			OutS->Add(Mid(StartPos));
		}
	}
	else
	{
		int32 StartPos = Find(InS, SearchCase, SearchDir) + InS.Len();
		int32 EndPos = Len();

		if (EndPos < 0) { return false; }

		while (StartPos > 0)
		{
			OutS->Add(Mid(StartPos, CoreAbs(EndPos - StartPos)));

			EndPos = StartPos - InS.Len();
			StartPos = Find(*InS, SearchCase, SearchDir, EndPos) + InS.Len();
		}

		if (EndPos != Len())
		{
			OutS->Add(Mid(0, EndPos));
		}
	}

	return true;
}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ToUpper() const&
{
	FKrnlString<PoolType> New = *this;
	New.ToUpperInline();
	return New;
}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ToUpper()&&
{
	this->ToUpperInline();
	return MoveTemp(*this);
}

template <EPoolType PoolType>
void FKrnlString<PoolType>::ToUpperInline()
{
	const int32 StringLength = Len();
	WIDECHAR* RawData = Data.GetData();
	for (int32 I = 0; I < StringLength; ++I)
	{
		RawData[I] = TChar<WIDECHAR>::ToUpper(RawData[I]);
	}
}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ToLower() const&
{
	FKrnlString<PoolType> New = *this;
	New.ToLowerInline();
	return New;
}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ToLower()&&
{
	this->ToLowerInline();
	return MoveTemp(*this);
}

template <EPoolType PoolType>
void FKrnlString<PoolType>::ToLowerInline()
{
}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::LeftPad(int32 ChCount) const
{
	int32 Pad = ChCount - Len();

	if (Pad > 0)
	{
		return ChrN(Pad, TEXT(' ')) + *this;
	}
	else
	{
		return *this;
	}
}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::RightPad(int32 ChCount) const
{
	int32 Pad = ChCount - Len();

	if (Pad > 0)
	{
		return *this + ChrN(Pad, TEXT(' '));
	}
	else
	{
		return *this;
	}
}

template <EPoolType PoolType>
NODISCARD bool FKrnlString<PoolType>::IsNumeric() const
{
	if (IsEmpty())
	{
		return 0;
	}

	return TCString<WIDECHAR>::IsNumeric(Data.GetData());
}

namespace Argus::Private::String
{
#define STARTING_BUFFER_SIZE 512
	//constexpr uint32 StartingBufferSize = 512;

	NODISCARD int32 GetWideVarArgs(WIDECHAR* Dest, size_t DestSize, const WIDECHAR* Fmt, va_list Args) noexcept
	{
		WIDECHAR* DestEnd = nullptr;
		const NTSTATUS Status = RtlStringCchVPrintfExW(
			Dest, DestSize, &DestEnd, nullptr, STRSAFE_NO_TRUNCATION, Fmt, Args);
		if (Status == STATUS_BUFFER_OVERFLOW)
		{
			return -1;
		}
		if (!NT_SUCCESS(Status) || DestEnd == nullptr)
		{
			return -2;
		}

		return static_cast<int32>(DestEnd - Dest);
	}
}

template <EPoolType PoolType>
FKrnlString<PoolType> FKrnlString<PoolType>::PrintfImpl(const WIDECHAR* Fmt, ...)
{
	int32  BufferSize = STARTING_BUFFER_SIZE;
	WIDECHAR  StartingBuffer[STARTING_BUFFER_SIZE];
	WIDECHAR* Buffer = StartingBuffer;
	int32  Result = -1;

	CORE_GET_VARARGS_RESULT(Buffer, BufferSize, Fmt, Fmt, Argus::Private::String::GetWideVarArgs, Result);

	if (Result == -1)
	{
		Buffer = nullptr;
		while (Result == -1)
		{
			BufferSize *= 2;
			Buffer = (WIDECHAR*)FMemory::Realloc(Buffer, BufferSize * sizeof(WIDECHAR));
			CORE_GET_VARARGS_RESULT(Buffer, BufferSize, Fmt, Fmt, Argus::Private::String::GetWideVarArgs, Result);
		}
	}

	if (Result < 0)
	{
		if (BufferSize != STARTING_BUFFER_SIZE)
		{
			FMemory::Free(Buffer);
		}
		return FKrnlString<PoolType>();
	}

	Buffer[Result] = TEXT('\0');
	FKrnlString<PoolType> ResultString(Buffer);

	if (BufferSize != STARTING_BUFFER_SIZE)
	{
		FMemory::Free(Buffer);
	}

	return ResultString;
}

template <EPoolType PoolType>
void VARARGS FKrnlString<PoolType>::AppendImpl(FKrnlString& AppendToMe, const WIDECHAR* Fmt, ...)
{
	int32 BufferSize = STARTING_BUFFER_SIZE;
	WIDECHAR StartingBuffer[STARTING_BUFFER_SIZE];
	WIDECHAR* Buffer = StartingBuffer;
	int32 Result = -1;

	CORE_GET_VARARGS_RESULT(Buffer, BufferSize, Fmt, Fmt, Argus::Private::String::GetWideVarArgs, Result);

	if (Result == -1)
	{
		Buffer = nullptr;
		while (Result == -1)
		{
			BufferSize *= 2;
			Buffer = (WIDECHAR*)FMemory::Realloc(Buffer, BufferSize * sizeof(WIDECHAR));
			CORE_GET_VARARGS_RESULT(Buffer, BufferSize, Fmt, Fmt, Argus::Private::String::GetWideVarArgs, Result);
		}
	}

	if (Result < 0)
	{
		if (BufferSize != STARTING_BUFFER_SIZE)
		{
			FMemory::Free(Buffer);
		}
		return;
	}

	Buffer[Result] = TEXT('\0');
	AppendToMe += Buffer;

	if (BufferSize != STARTING_BUFFER_SIZE)
	{
		FMemory::Free(Buffer);
	}
}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::Chr(WIDECHAR Char)
{
	WIDECHAR Temp[2] = { Char, TEXT('\0') };
	return FKrnlString<PoolType>(Temp);
}

template <EPoolType PoolType>
NODISCARD FKrnlString<PoolType> FKrnlString<PoolType>::ChrN(int32 NumCharactor, WIDECHAR Char)
{
	CORE_CHECK(NumCharactor >= 0);

	FKrnlString<PoolType> Temp;
	Temp.Data.PushUninitialized(NumCharactor + 1);

	for (int32 Cx = 0; Cx < NumCharactor; ++Cx)
	{
		Temp[Cx] = Char;
	}

	Temp.Data[NumCharactor] = TEXT('\0');
	return Temp;
}



template <EPoolType PoolType>
void FKrnlString<PoolType>::Reserve(int32 CharacterCount)
{
	CORE_CHECK(CharacterCount >= 0 && CharacterCount < MAX_int32);
	if (CharacterCount > 0)
	{
		Data.Reserve(CharacterCount);
	}
}

template <EPoolType PoolType>
bool FKrnlString<PoolType>::StartsWith(const WIDECHAR* InPrefix, int32 InPrefixLen, ESearchCase::Type SearchCase) const
{
	if (SearchCase == ESearchCase::IgnoreCase)
	{
		return InPrefixLen > 0 && !TCString<WIDECHAR>::Strnicmp(**this, InPrefix, InPrefixLen);
	}
	else
	{
		return InPrefixLen > 0 && !TCString<WIDECHAR>::Strncmp(**this, InPrefix, InPrefixLen);
	}
}

template <EPoolType PoolType>
bool FKrnlString<PoolType>::EndsWith(const WIDECHAR* InSuffix, int32 InSuffixLen, ESearchCase::Type SearchCase) const
{
	if (SearchCase == ESearchCase::IgnoreCase)
	{
		return InSuffixLen > 0 &&
			Len() >= InSuffixLen && 
			!Stricmp(&(*this)[Len() - InSuffixLen], InSuffix);
	}
	else
	{
		return InSuffixLen > 0 &&
			Len() >= InSuffixLen &&
			!Strcmp(&(*this)[Len() - InSuffixLen], InSuffix);
	}
}
