#pragma once
#include "SmartPointerInternals.h"

template <typename ObjectType, ESPMode InMode>
class TSharedPtr
{
public:
	using ElementType = ObjectType;
	static constexpr ESPMode Mode = InMode;

public:
	FORCEINLINE TSharedPtr()
		: Object(nullptr)
		, SharedReferenceCount()
	{
	}

	FORCEINLINE TSharedPtr(TSharedPtr const& InSharedPtr)
		: Object(InSharedPtr.Object)
		, SharedReferenceCount(InSharedPtr.SharedReferenceCount)
	{
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TSharedPtr(OtherType* InObject)
		: Object(InObject)
		, SharedReferenceCount(SharedPointerInternals::NewDefaultReferenceController<Mode>(InObject))
	{
	}

	template <
		typename OtherType,
		typename DeleterType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TSharedPtr(OtherType* InObject, DeleterType&& InDeleter)
		: Object(InObject)
		, SharedReferenceCount(SharedPointerInternals::NewCustomReferenceController<Mode, OtherType, DeleterType>(InObject, Forward<DeleterType>(InDeleter)))
	{
	}

	template <typename OtherType>
	FORCEINLINE explicit TSharedPtr(TSharedPtr<OtherType, Mode> const& InSharedPtr)
		: Object(static_cast<ObjectType*>(InSharedPtr.Object))
		, SharedReferenceCount(InSharedPtr.SharedReferenceCount)
	{
	}

	template <typename OtherType>
	FORCEINLINE explicit TSharedPtr(TSharedPtr<OtherType, Mode> const& OtherSharedPtr, OtherType* InObject)
		: Object(InObject)
		, SharedReferenceCount(OtherSharedPtr.SharedReferenceCount)
	{
	}

	template <typename OtherType>
	FORCEINLINE explicit TSharedPtr(TSharedPtr<OtherType, Mode>&& OtherSharedPtr, OtherType* InObject)
		: Object(InObject)
		, SharedReferenceCount(MoveTemp(OtherSharedPtr.SharedReferenceCount))
	{
		OtherSharedPtr.Object = nullptr;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TSharedPtr(TWeakPtr<OtherType, Mode> const& OtherWeakPtr)
		: Object(OtherWeakPtr.Object)
		, SharedReferenceCount(OtherWeakPtr.WeakReferenceCount)
	{
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TSharedPtr(TUniquePtr<OtherType, Mode>&& InUniquePtr)
		: Object(InUniquePtr.Object)
		, SharedReferenceCount(MoveTemp(InUniquePtr.UniqueReferenceCount))
	{
		InUniquePtr.Object = nullptr;
	}

public:
	FORCEINLINE TSharedPtr& operator=(TSharedPtr const& InSharedPtr) noexcept
	{
		TSharedPtr Temp = InSharedPtr;
		Swap(Temp, *this);
		return *this;
	}

	FORCEINLINE TSharedPtr& operator=(TSharedPtr&& InSharedPtr) noexcept
	{
		if (this != &InSharedPtr)
		{
			Object = InSharedPtr.Object;
			InSharedPtr.Object = nullptr;
			SharedReferenceCount = MoveTemp(InSharedPtr.SharedReferenceCount);
		}

		return *this;
	}

public:
	FORCEINLINE ObjectType* Get() const noexcept
	{
		return Object;
	}

	FORCEINLINE ObjectType** GetAddressOf() const noexcept
	{
		return &Object;
	}

	FORCEINLINE explicit operator bool() const noexcept
	{
		return Object != nullptr;
	}

	FORCEINLINE const bool IsValid() const noexcept
	{
		return Object != nullptr;
	}

	FORCEINLINE ObjectType& operator->() const noexcept
	{
		return Object;
	}

	FORCEINLINE RefCounter GetSharedReferenceCount() const noexcept
	{
		return SharedReferenceCount.GetSharedReferenceCount();
	}

	FORCEINLINE bool IsUnique() const noexcept
	{
		return SharedReferenceCount.IsUnique();
	}

public:
	FORCEINLINE void Reset()
	{
		*this = TSharedPtr<ObjectType, Mode>();
	}

public:
	FORCEINLINE TUniquePtr<ObjectType, Mode> ToUniquePtr()
	{
		return TUniquePtr<ObjectType, Mode>(MoveTemp(*this));
	}

private:
	template <class OtherType, ESPMode OtherMode> friend class TSharedPtr;
	template <class OtherType, ESPMode OtherMode> friend class TWeakPtr;
	template <class OtherType, ESPMode OtherMode> friend class TUniquePtr;

private:
	ObjectType* Object;
	SharedPointerInternals::FSharedReferencer<Mode> SharedReferenceCount;
};


template <typename T>							struct TIsSharedPtr { enum { Value = false }; };
template <typename ObjectType, ESPMode InMode>	struct TIsSharedPtr<			   TSharedPtr<ObjectType, InMode>> { enum { Value = true }; };
template <typename ObjectType, ESPMode InMode>	struct TIsSharedPtr<const		   TSharedPtr<ObjectType, InMode>> { enum { Value = true }; };
template <typename ObjectType, ESPMode InMode>	struct TIsSharedPtr<	  volatile TSharedPtr<ObjectType, InMode>> { enum { Value = true }; };
template <typename ObjectType, ESPMode InMode>	struct TIsSharedPtr<const volatile TSharedPtr<ObjectType, InMode>> { enum { Value = true }; };


template <typename ObjectType, ESPMode InMode>
class TWeakPtr
{
public:
	constexpr static ESPMode Mode = InMode;

public:
	FORCEINLINE TWeakPtr()
		: Object(nullptr)
		, WeakReferenceCount()
	{
	}

	FORCEINLINE TWeakPtr(TWeakPtr const& InWeakPtr)
		: Object(InWeakPtr.Object)
		, WeakReferenceCount(InWeakPtr.WeakReferenceCount)
	{
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TWeakPtr(TWeakPtr<OtherType, Mode> const& InWeakPtr)
		: Object(InWeakPtr.Object)
		, WeakReferenceCount(InWeakPtr.WeakReferenceCount)
	{
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TWeakPtr(TWeakPtr<OtherType, Mode>&& InWeakPtr)
		: Object(static_cast<ObjectType*>(InWeakPtr.Object))
		, WeakReferenceCount(InWeakPtr.SharedRefeerenceCount)
	{
		InWeakPtr.Object = nullptr;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TWeakPtr(TUniquePtr<OtherType, Mode> const& InUniquePtr)
		: Object(InUniquePtr.Object)
		, WeakReferenceCount(InUniquePtr.UniqueReferenceCount)
	{
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TWeakPtr(TSharedPtr<OtherType, Mode> const& InSharedPtr)
		: Object(static_cast<ObjectType*>(InSharedPtr.Object))
		, WeakReferenceCount(InSharedPtr.SharedReferenceCount)
	{
	}

public:
	FORCEINLINE TWeakPtr& operator=(TWeakPtr const& InWeakPtr)
	{
		TWeakPtr Tmp = InWeakPtr;
		Swap<TWeakPtr>(Tmp, *this);
		return *this;
	}

	FORCEINLINE TWeakPtr& operator=(TWeakPtr&& InWeakPtr)
	{
		if (this != &InWeakPtr)
		{
			Object = InWeakPtr.Object;
			InWeakPtr.Object = nullptr;
			WeakReferenceCount = MoveTemp(InWeakPtr.WeakReferenceCount);
		}

		return *this;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TWeakPtr& operator=(TWeakPtr<OtherType, Mode> const& InWeakPtr)
	{
		Object = InWeakPtr.Pin().Get();
		WeakReferenceCount = InWeakPtr.WeakReferenceCount;
		return *this;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TWeakPtr& operator=(TWeakPtr<OtherType, Mode>&& InWeakPtr)
	{
		Object = InWeakPtr.Object;
		InWeakPtr.Object = nullptr;
		WeakReferenceCount = MoveTemp(InWeakPtr.WeakReferenceCount);
		return *this;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TWeakPtr& operator=(TSharedPtr<OtherType, Mode> const& InSharedPtr)
	{
		Object = InSharedPtr.Object;
		WeakReferenceCount = InSharedPtr.SharedReferenceCount;
		return *this;
	}

public:
	FORCEINLINE void Reset()
	{
		*this = TSharedPtr<ObjectType, Mode>();
	}

private:
	FORCEINLINE TSharedPtr<ObjectType, Mode> Pin()
	{
		return TSharedPtr<ObjectType, Mode>(MoveTemp(*this));
	}

public:
	FORCEINLINE TSharedPtr<ObjectType, Mode> ToSharedPtr() const
	{
		return TSharedPtr<ObjectType, Mode>(MoveTemp(*this));
	}

private:
	template <class OtherType, ESPMode OtherMode> friend class TSharedPtr;
	template <class OtherType, ESPMode OtherMode> friend class TWeakPtr;

private:
	ObjectType* Object;
	SharedPointerInternals::FWeakReferencer<Mode> WeakReferenceCount;
};


template <typename T>						   struct TIsWeakPtr { enum { Value = false }; };
template <typename ObjectType, ESPMode InMode> struct TIsWeakPtr<				TWeakPtr<ObjectType, InMode>> { enum { Value = true }; };
template <typename ObjectType, ESPMode InMode> struct TIsWeakPtr<const			TWeakPtr<ObjectType, InMode>> { enum { Value = true }; };
template <typename ObjectType, ESPMode InMode> struct TIsWeakPtr<	   volatile TWeakPtr<ObjectType, InMode>> { enum { Value = true }; };
template <typename ObjectType, ESPMode InMode> struct TIsWeakPtr<const volatile TWeakPtr<ObjectType, InMode>> { enum { Value = true }; };

template <typename T>						   constexpr bool TIsWeakPtr_V = false;
template <typename ObjectType, ESPMode InMode> constexpr bool TIsWeakPtr_V<				  TWeakPtr<ObjectType, InMode>> = true;
template <typename ObjectType, ESPMode InMode> constexpr bool TIsWeakPtr_V<const		  TWeakPtr<ObjectType, InMode>> = true;
template <typename ObjectType, ESPMode InMode> constexpr bool TIsWeakPtr_V<	     volatile TWeakPtr<ObjectType, InMode>> = true;
template <typename ObjectType, ESPMode InMode> constexpr bool TIsWeakPtr_V<const volatile TWeakPtr<ObjectType, InMode>> = true;

template <typename ObjectType, ESPMode InMode>
class TUniquePtr
{
public:
	static constexpr ESPMode Mode = InMode;

public:
	FORCEINLINE TUniquePtr()
		: Object(nullptr)
		, UniqueReferenceCount()
	{
	}

	FORCEINLINE TUniquePtr(TUniquePtr& InUniquePtr)
		: Object(InUniquePtr.Object)
		, UniqueReferenceCount(MoveTemp(InUniquePtr.UniqueReferenceCount))
	{
		InUniquePtr.Object = nullptr;
	}

	FORCEINLINE TUniquePtr(TUniquePtr&& InUniquePtr) noexcept
		: Object(InUniquePtr.Object)
		, UniqueReferenceCount(MoveTemp(InUniquePtr.UniqueReferenceCount))
	{
		InUniquePtr.Object = nullptr;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TUniquePtr(OtherType* InObject)
		: Object(InObject)
		, UniqueReferenceCount(SharedPointerInternals::NewDefaultReferenceController<Mode, OtherType>(InObject))
	{
	}

	template <
		typename OtherType,
		typename DeleterType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TUniquePtr(OtherType* InObject, DeleterType&& Deleter)
		: Object(InObject)
		, UniqueReferenceCount(SharedPointerInternals::NewCustomReferenceController<Mode, OtherType, DeleterType>(InObject, Forward<DeleterType>(Deleter)))
	{
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TUniquePtr(TUniquePtr<OtherType, Mode>& InUniquePtr)
		: Object(static_cast<ObjectType*>(InUniquePtr.Object))
		, UniqueReferenceCount(MoveTemp(InUniquePtr.UniqueReferenceCount))
	{
		InUniquePtr.Object = nullptr;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TUniquePtr(TSharedPtr<OtherType, Mode>& InSharedPtr)
		: Object(InSharedPtr.Object)
		, UniqueReferenceCount(MoveTemp(InSharedPtr.SharedReferenceCount))
	{
		InSharedPtr.Object = nullptr;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TUniquePtr(TSharedPtr<OtherType, Mode>&& InSharedPtr)
		: Object(InSharedPtr.Object)
		, UniqueReferenceCount(MoveTemp(InSharedPtr.SharedReferenceCount))
	{
		InSharedPtr.Object = nullptr;
	}

	FORCEINLINE TUniquePtr& operator=(TUniquePtr& InUniquePtr)
	{
		Swap(*this, InUniquePtr);
		return *this;
	}

	FORCEINLINE TUniquePtr& operator=(TUniquePtr&& InUniquePtr) noexcept
	{
		Reset(Forward<TUniquePtr>(InUniquePtr));
		return *this;
	}

public:
	FORCEINLINE ObjectType* Get() const noexcept
	{
		return Object;
	}

	FORCEINLINE ObjectType** GetAddressOf() const noexcept
	{
		return &Object;
	}

	FORCEINLINE explicit operator bool() const noexcept
	{
		return Object != nullptr;
	}

	FORCEINLINE const bool IsValid() const noexcept
	{
		return Object != nullptr;
	}

	FORCEINLINE ObjectType& operator->() const noexcept
	{
		return Object;
	}

	FORCEINLINE RefCounter GetUniqueReferenceCount() const noexcept
	{
		return UniqueReferenceCount.GetSharedReferenceCount();
	}

	FORCEINLINE bool IsUnique() const noexcept
	{
		return UniqueReferenceCount.IsUnique();
	}

public:
	FORCEINLINE void Reset() noexcept
	{
		*this = TUniquePtr<ObjectType, Mode>();
	}

	FORCEINLINE void Reset(TUniquePtr& InUniquePtr)
	{
		Object = InUniquePtr.Object;
		InUniquePtr.Object = nullptr;
		UniqueReferenceCount.MoveReferenceController(InUniquePtr.UniqueReferenceCount);
	}

	FORCEINLINE void Reset(TUniquePtr&& InUniquePtr)
	{
		Object = InUniquePtr.Object;
		InUniquePtr.Object = nullptr;
		UniqueReferenceCount.MoveReferenceController(InUniquePtr.UniqueReferenceCount);
	}

	FORCEINLINE void Release()
	{

	}

private:
	template <class OtherType, ESPMode OtherMode> friend class TSharedPtr;
	template <class OtherType, ESPMode OtherMode> friend class TWeakPtr;
	template <class OtherType, ESPMode OtherMode> friend class TUniquePtr;

private:
	ObjectType* Object;
	SharedPointerInternals::FUniqueReferencer<Mode> UniqueReferenceCount;
};


template <typename T>						   struct TIsUniquePtr { enum { Value = false }; };
template <typename ObjectType, ESPMode InMode> struct TIsUniquePtr<				  TUniquePtr<ObjectType, InMode>> { enum { Value = true }; };
template <typename ObjectType, ESPMode InMode> struct TIsUniquePtr<const		  TUniquePtr<ObjectType, InMode>> { enum { Value = true }; };
template <typename ObjectType, ESPMode InMode> struct TIsUniquePtr<		 volatile TUniquePtr<ObjectType, InMode>> { enum { Value = true }; };
template <typename ObjectType, ESPMode InMode> struct TIsUniquePtr<const volatile TUniquePtr<ObjectType, InMode>> { enum { Value = true }; };

template< class ObjectTypeA, class ObjectTypeB, ESPMode Mode >
[[nodiscard]] FORCEINLINE bool operator==(TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrA, TSharedPtr<ObjectTypeB, Mode> const& InSharedPtrB) noexcept
{
	return InSharedPtrA.Get() == InSharedPtrB.Get();
}

template< class ObjectTypeA, class ObjectTypeB, ESPMode Mode >
[[nodiscard]] FORCEINLINE bool operator!=(TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrA, TSharedPtr<ObjectTypeB, Mode> const& InSharedPtrB) noexcept
{
	return InSharedPtrA.Get() != InSharedPtrB.Get();
}


template <typename ObjectTypeA, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrA, TYPE_OF_NULLPTR) noexcept
{
	return !InSharedPtrA.IsValid();
}

template <typename ObjectTypeA, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator!=(TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrA, TYPE_OF_NULLPTR) noexcept
{
	return InSharedPtrA.IsValid();
}


template <typename ObjectTypeA, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TYPE_OF_NULLPTR, TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrB) noexcept
{
	return !InSharedPtrB.IsValid();
}

template <typename ObjectTypeA, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator!=(TYPE_OF_NULLPTR, TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrB) noexcept
{
	return InSharedPtrB.IsValid();
}


template <typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrA, TWeakPtr<ObjectTypeB, Mode> const& InWeakPtrB) noexcept
{
	return InSharedPtrA.Get() == InWeakPtrB.ToSharedPtr().Get();
}

template <typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator!=(TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrA, TWeakPtr<ObjectTypeB, Mode> const& InWeakPtrB) noexcept
{
	return InSharedPtrA.Get() != InWeakPtrB.ToSharedPtr().Get();
}


template <typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrA, TUniquePtr<ObjectTypeB, Mode> const& InUniquePtrB) noexcept
{
	return InSharedPtrA.Get();
}

template <typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TWeakPtr<ObjectTypeB, Mode> const& InWeakPtrA, TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrB) noexcept
{
	return InWeakPtrA == InSharedPtrB;
}

template <typename ObjectTypeA, typename ObjectTypeB, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator!=(TWeakPtr<ObjectTypeB, Mode> const& InWeakPtrA, TSharedPtr<ObjectTypeA, Mode> const& InSharedPtrB) noexcept
{
	return InWeakPtrA != InSharedPtrB;
}

template <typename T, typename... ArgsType>
NODISCARD FORCEINLINE TUniquePtr<T> MakeUnique(ArgsType&&... Args) noexcept
{
	return TUniquePtr<T>(::new T(Forward<ArgsType>(Args)...));
}

template <typename T, typename... ArgsType>
NODISCARD FORCEINLINE TSharedPtr<T> MakeShared(ArgsType&&... Args) noexcept
{
	return TSharedPtr<T>(::new T(Forward<ArgsType>(Args)...));
}