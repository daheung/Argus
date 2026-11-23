#pragma once
#include "Kernel/Public/Containers/KrnlSmartPointerInternals.h"

template <typename ObjectType, EPoolType PoolType, ESPMode InMode>
class TKrnlSharedPtr
{
public:
	using ElementType = ObjectType;
	static constexpr EPoolType Type = PoolType;
	static constexpr ESPMode Mode = InMode;

public:
	FORCEINLINE TKrnlSharedPtr()
		: Object(nullptr)
		, SharedReferenceCount()
	{
	}

	FORCEINLINE TKrnlSharedPtr(TKrnlSharedPtr const& InSharedPtr)
		: Object(InSharedPtr.Object)
		, SharedReferenceCount(InSharedPtr.SharedReferenceCount)
	{
	}

	private:
	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlSharedPtr(OtherType* InObject)
		: Object(InObject)
		, SharedReferenceCount(SharedPointerInternals::NewDefaultReferenceController<PoolType, Mode>(InObject))
	{
		if (InObject != nullptr && SharedReferenceCount.GetSharedReferenceCount() == 0)
		{
			SharedPointerInternals::FDefaultDeleter<OtherType>{}(InObject);
			Object = nullptr;
		}
	}

	public:
	template <
		typename OtherType,
		typename DeleterType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlSharedPtr(OtherType* InObject, DeleterType&& InDeleter)
		: Object(InObject)
		, SharedReferenceCount(SharedPointerInternals::NewCustomReferenceController<PoolType, Mode>(InObject, Forward<DeleterType>(InDeleter)))
	{
		if (InObject != nullptr && SharedReferenceCount.GetSharedReferenceCount() == 0)
		{
			Invoke(InDeleter, InObject);
			Object = nullptr;
		}
	}

	template <typename OtherType>
	FORCEINLINE explicit TKrnlSharedPtr(TKrnlSharedPtr<OtherType, PoolType, Mode> const& InSharedPtr)
		: Object(static_cast<ObjectType*>(InSharedPtr.Object))
		, SharedReferenceCount(InSharedPtr.SharedReferenceCount)
	{
	}

	template <typename OtherType>
	FORCEINLINE explicit TKrnlSharedPtr(TKrnlSharedPtr<OtherType, PoolType, Mode> const& OtherSharedPtr, OtherType* InObject)
		: Object(InObject)
		, SharedReferenceCount(OtherSharedPtr.SharedReferenceCount)
	{
	}

	template <typename OtherType>
	FORCEINLINE explicit TKrnlSharedPtr(TKrnlSharedPtr<OtherType, PoolType, Mode>&& OtherSharedPtr, OtherType* InObject)
		: Object(InObject)
		, SharedReferenceCount(MoveTemp(OtherSharedPtr.SharedReferenceCount))
	{
		OtherSharedPtr.Object = nullptr;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlSharedPtr(TKrnlWeakPtr<OtherType, PoolType, Mode> const& OtherWeakPtr)
		: Object(OtherWeakPtr.Object)
		, SharedReferenceCount(OtherWeakPtr.WeakReferenceCount)
	{
		if (SharedReferenceCount.GetSharedReferenceCount() == 0)
		{
			Object = nullptr;
		}
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlSharedPtr(TKrnlWeakPtr<OtherType, PoolType, Mode>&& OtherWeakPtr)
		: Object(OtherWeakPtr.Object)
		, SharedReferenceCount(MoveTemp(OtherWeakPtr.WeakReferenceCount))
	{
		OtherWeakPtr.Object = nullptr;
		if (SharedReferenceCount.GetSharedReferenceCount() == 0)
		{
			Object = nullptr;
		}
	}

#if 0 // Old reference-counted UniquePtr conversion.
	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlSharedPtr(TKrnlUniquePtr<OtherType, PoolType, Mode>&& InUniquePtr)
		: Object(InUniquePtr.Object)
		, SharedReferenceCount(MoveTemp(InUniquePtr.UniqueReferenceCount))
	{
		InUniquePtr.Object = nullptr;
	}
#endif

	template <
		typename OtherType,
		typename OtherDeleterType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlSharedPtr(TKrnlUniquePtr<OtherType, PoolType, Mode, OtherDeleterType>&& InUniquePtr) noexcept
		: Object(nullptr)
		, SharedReferenceCount()
	{
		if (InUniquePtr.Object == nullptr)
		{
			return;
		}
		auto* Controller = SharedPointerInternals::NewCustomReferenceController<PoolType, Mode>(
			InUniquePtr.Object, MoveTemp(InUniquePtr.GetDeleter()));
		if (Controller == nullptr)
		{
			return; // The UniquePtr keeps ownership on allocation failure.
		}
		Object = static_cast<ObjectType*>(InUniquePtr.Object);
		SharedReferenceCount = SharedPointerInternals::FKrnlSharedReferencer<Mode>(Controller);
		InUniquePtr.Object = nullptr;
	}

public:
	FORCEINLINE TKrnlSharedPtr& operator=(TKrnlSharedPtr const& InSharedPtr) noexcept
	{
		TKrnlSharedPtr Temp = InSharedPtr;
		Swap(Temp, *this);
		return *this;
	}

	FORCEINLINE TKrnlSharedPtr& operator=(TKrnlSharedPtr&& InSharedPtr) noexcept
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

	FORCEINLINE ObjectType* operator->() const noexcept
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
		*this = TKrnlSharedPtr<ObjectType, PoolType, Mode>();
	}

	#if 0 // SharedPtr cannot transfer ownership to UniquePtr.
public:
	FORCEINLINE TKrnlUniquePtr<ObjectType, PoolType, Mode> ToUniquePtr()
	{
		return TKrnlUniquePtr<ObjectType, PoolType, Mode>(MoveTemp(*this));
	}
	#endif

private:
	template <EPoolType InPoolType, typename T, typename... ArgsType>
	friend TKrnlSharedPtr<T, InPoolType> MakeShared(ArgsType&&... Args) noexcept;
	template <class, EPoolType, ESPMode> friend class TKrnlSharedPtr;
	template <class, EPoolType, ESPMode> friend class TKrnlWeakPtr;
	template <class, EPoolType, ESPMode, class> friend class TKrnlUniquePtr;

private:
	ObjectType* Object;
	SharedPointerInternals::FKrnlSharedReferencer<Mode> SharedReferenceCount;
};


template <typename T> struct TIsSharedPtr { enum { Value = false }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsSharedPtr<	 		    TKrnlSharedPtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsSharedPtr<const	 	    TKrnlSharedPtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsSharedPtr<	   volatile TKrnlSharedPtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsSharedPtr<const volatile TKrnlSharedPtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };


template <typename ObjectType, EPoolType PoolType, ESPMode InMode>
class TKrnlWeakPtr
{
public:
	static constexpr EPoolType Type = PoolType;
	constexpr static ESPMode Mode = InMode;

public:
	FORCEINLINE TKrnlWeakPtr()
		: Object(nullptr)
		, WeakReferenceCount()
	{
	}

	FORCEINLINE TKrnlWeakPtr(TKrnlWeakPtr const& InWeakPtr)
		: Object(InWeakPtr.Object)
		, WeakReferenceCount(InWeakPtr.WeakReferenceCount)
	{
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlWeakPtr(TKrnlWeakPtr<OtherType, PoolType, Mode> const& InWeakPtr)
		: Object(InWeakPtr.Object)
		, WeakReferenceCount(InWeakPtr.WeakReferenceCount)
	{
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlWeakPtr(TKrnlWeakPtr<OtherType, PoolType, Mode>&& InWeakPtr)
		: Object(static_cast<ObjectType*>(InWeakPtr.Object))
		, WeakReferenceCount(MoveTemp(InWeakPtr.WeakReferenceCount))
	{
		InWeakPtr.Object = nullptr;
	}

	#if 0 // UniquePtr has no reference controller.
	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlWeakPtr(TKrnlUniquePtr<OtherType, PoolType, Mode> const& InUniquePtr)
		: Object(InUniquePtr.Object)
		, WeakReferenceCount(InUniquePtr.UniqueReferenceCount)
	{
	}
	#endif

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlWeakPtr(TKrnlSharedPtr<OtherType, PoolType, Mode> const& InSharedPtr)
		: Object(static_cast<ObjectType*>(InSharedPtr.Object))
		, WeakReferenceCount(InSharedPtr.SharedReferenceCount)
	{
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlWeakPtr(TKrnlSharedPtr<OtherType, PoolType, Mode>&& InSharedPtr)
		: Object(static_cast<ObjectType*>(InSharedPtr.Object))
		, WeakReferenceCount(MoveTemp(InSharedPtr.SharedReferenceCount))
	{
		InSharedPtr.Object = nullptr;
	}

public:
	FORCEINLINE TKrnlWeakPtr& operator=(TKrnlWeakPtr const& InWeakPtr)
	{
		WeakReferenceCount = InWeakPtr.WeakReferenceCount;
		Object = InWeakPtr.Object;
		return *this;
	}

	FORCEINLINE TKrnlWeakPtr& operator=(TKrnlWeakPtr&& InWeakPtr)
	{
		if (this != &InWeakPtr)
		{
			WeakReferenceCount = MoveTemp(InWeakPtr.WeakReferenceCount);
			Object = InWeakPtr.Object;
			InWeakPtr.Object = nullptr;
		}

		return *this;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TKrnlWeakPtr& operator=(TKrnlWeakPtr<OtherType, PoolType, Mode> const& InWeakPtr)
	{
		WeakReferenceCount = InWeakPtr.WeakReferenceCount;
		Object = InWeakPtr.Object;
		return *this;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TKrnlWeakPtr& operator=(TKrnlWeakPtr<OtherType, PoolType, Mode>&& InWeakPtr)
	{
		WeakReferenceCount = MoveTemp(InWeakPtr.WeakReferenceCount);
		Object = InWeakPtr.Object;
		InWeakPtr.Object = nullptr;
		return *this;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TKrnlWeakPtr& operator=(TKrnlSharedPtr<OtherType, PoolType, Mode> const& InSharedPtr)
	{
		WeakReferenceCount = InSharedPtr.SharedReferenceCount;
		Object = InSharedPtr.Object;
		return *this;
	}

public:
	FORCEINLINE void Reset()
	{
		*this = TKrnlWeakPtr();
	}

private:
	FORCEINLINE TKrnlSharedPtr<ObjectType, PoolType, Mode> Pin()
	{
		return TKrnlSharedPtr<ObjectType, PoolType, Mode>(MoveTemp(*this));
	}

public:
	FORCEINLINE TKrnlSharedPtr<ObjectType, PoolType, Mode> ToSharedPtr() const
	{
		return TKrnlSharedPtr<ObjectType, PoolType, Mode>(*this);
	}

private:
	template <class, EPoolType, ESPMode> friend class TKrnlSharedPtr;
	template <class, EPoolType, ESPMode> friend class TKrnlWeakPtr;

private:
	ObjectType* Object;
	SharedPointerInternals::FKrnlWeakReferencer<Mode> WeakReferenceCount;
};


template <typename T> struct TIsWeakPtr { enum { Value = false }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsWeakPtr<				  TKrnlWeakPtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsWeakPtr<const		  TKrnlWeakPtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsWeakPtr<	     volatile TKrnlWeakPtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsWeakPtr<const volatile TKrnlWeakPtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };

template <typename T> constexpr bool TIsWeakPtr_V = false;
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> constexpr bool TIsWeakPtr_V<				TKrnlWeakPtr<ObjectType, InPoolType, InMode>> = true;
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> constexpr bool TIsWeakPtr_V<const		    TKrnlWeakPtr<ObjectType, InPoolType, InMode>> = true;
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> constexpr bool TIsWeakPtr_V<	   volatile TKrnlWeakPtr<ObjectType, InPoolType, InMode>> = true;
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> constexpr bool TIsWeakPtr_V<const volatile TKrnlWeakPtr<ObjectType, InPoolType, InMode>> = true;

#if 0 // Previous reference-counted UniquePtr; a direct owner replaces this below.
template <typename ObjectType, EPoolType PoolType, ESPMode InMode>
class TKrnlUniquePtr
{
public:
	static constexpr EPoolType Type = PoolType;
	static constexpr ESPMode Mode = InMode;

public:
	FORCEINLINE TKrnlUniquePtr()
		: Object(nullptr)
		, UniqueReferenceCount()
	{
	}

	TKrnlUniquePtr(TKrnlUniquePtr& InUniquePtr) = delete;

	FORCEINLINE TKrnlUniquePtr(TKrnlUniquePtr&& InUniquePtr) noexcept
		: Object(InUniquePtr.Object)
		, UniqueReferenceCount(MoveTemp(InUniquePtr.UniqueReferenceCount))
	{
		InUniquePtr.Object = nullptr;
	}

	private:
	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TKrnlUniquePtr(OtherType* InObject)
		: Object(InObject)
		, UniqueReferenceCount(SharedPointerInternals::NewDefaultReferenceController<PoolType, Mode>(InObject))
	{
		if (InObject != nullptr && UniqueReferenceCount.GetUniqueReferenceCount() == 0)
		{
			SharedPointerInternals::FDefaultDeleter<OtherType>{}(InObject);
			Object = nullptr;
		}
	}

	public:
	template <
		typename OtherType,
		typename DeleterType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TKrnlUniquePtr(OtherType* InObject, DeleterType&& Deleter)
		: Object(InObject)
		, UniqueReferenceCount(SharedPointerInternals::NewCustomReferenceController<PoolType, Mode>(InObject, Forward<DeleterType>(Deleter)))
	{
		if (InObject != nullptr && UniqueReferenceCount.GetUniqueReferenceCount() == 0)
		{
			Invoke(Deleter, InObject);
			Object = nullptr;
		}
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	TKrnlUniquePtr(TKrnlUniquePtr<OtherType, PoolType, Mode>& InUniquePtr) = delete;

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TKrnlUniquePtr(TKrnlUniquePtr<OtherType, PoolType, Mode>&& InUniquePtr) noexcept
		: Object(static_cast<ObjectType*>(InUniquePtr.Object))
		, UniqueReferenceCount(MoveTemp(InUniquePtr.UniqueReferenceCount))
	{
		InUniquePtr.Object = nullptr;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	TKrnlUniquePtr(TKrnlSharedPtr<OtherType, PoolType, Mode>& InSharedPtr) = delete;

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE TKrnlUniquePtr(TKrnlSharedPtr<OtherType, PoolType, Mode>&& InSharedPtr)
		: Object(nullptr)
		, UniqueReferenceCount()
	{
		if (InSharedPtr.IsUnique())
		{
			Object = InSharedPtr.Object;
			UniqueReferenceCount = SharedPointerInternals::FKrnlUniqueReferencer<Mode>(MoveTemp(InSharedPtr.SharedReferenceCount));
			InSharedPtr.Object = nullptr;
		}
	}

	TKrnlUniquePtr& operator=(TKrnlUniquePtr& InUniquePtr) = delete;

	FORCEINLINE TKrnlUniquePtr& operator=(TKrnlUniquePtr&& InUniquePtr) noexcept
	{
		if (this != &InUniquePtr)
		{
			UniqueReferenceCount = MoveTemp(InUniquePtr.UniqueReferenceCount);
			Object = InUniquePtr.Object;
			InUniquePtr.Object = nullptr;
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

	FORCEINLINE ObjectType* operator->() const noexcept
	{
		return Object;
	}

	FORCEINLINE RefCounter GetUniqueReferenceCount() const noexcept
	{
		return UniqueReferenceCount.GetUniqueReferenceCount();
	}

	FORCEINLINE bool IsUnique() const noexcept
	{
		return UniqueReferenceCount.IsUnique();
	}

public:
	FORCEINLINE void Reset() noexcept
	{
		*this = TKrnlUniquePtr<ObjectType, PoolType, Mode>();
	}

	FORCEINLINE void Reset(TKrnlUniquePtr& InUniquePtr)
	{
		*this = MoveTemp(InUniquePtr);
	}

	FORCEINLINE void Reset(TKrnlUniquePtr&& InUniquePtr)
	{
		*this = MoveTemp(InUniquePtr);
	}

	FORCEINLINE void Release()
	{

	}

private:
	template <EPoolType InPoolType, typename T, typename... ArgsType>
	friend TKrnlUniquePtr<T, InPoolType> MakeUnique(ArgsType&&... Args) noexcept;
	template <class, EPoolType, ESPMode> friend class TKrnlSharedPtr;
	template <class, EPoolType, ESPMode> friend class TKrnlWeakPtr;
	template <class, EPoolType, ESPMode> friend class TKrnlUniquePtr;

private:
	ObjectType* Object;
	SharedPointerInternals::FKrnlUniqueReferencer<Mode> UniqueReferenceCount;
};


template <typename T>						   struct TIsUniquePtr { enum { Value = false }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsUniquePtr<				  TKrnlUniquePtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsUniquePtr<const		  TKrnlUniquePtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsUniquePtr<		 volatile TKrnlUniquePtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
template <typename ObjectType, EPoolType InPoolType, ESPMode InMode> struct TIsUniquePtr<const volatile TKrnlUniquePtr<ObjectType, InPoolType, InMode>> { enum { Value = true }; };
#endif

template <typename ObjectType, EPoolType PoolType, ESPMode InMode, typename DeleterType>
class TKrnlUniquePtr : 
	private SharedPointerInternals::TDeleterHolder<DeleterType>
{
private:
	using FDeleterHolder = SharedPointerInternals::TDeleterHolder<DeleterType>;

public:
	using ElementType = ObjectType;
	using Deleter = DeleterType;
	static constexpr EPoolType Type = PoolType;
	static constexpr ESPMode Mode = InMode;

	TKrnlUniquePtr() noexcept = default;
	TKrnlUniquePtr(TKrnlUniquePtr const&) = delete;
	TKrnlUniquePtr& operator=(TKrnlUniquePtr const&) = delete;

	FORCEINLINE TKrnlUniquePtr(TKrnlUniquePtr&& Other) noexcept
		: FDeleterHolder(DeleterType(MoveTemp(Other.GetDeleter())))
		, Object(Other.Object)
	{
		Other.Object = nullptr;
	}

	template <
		typename OtherType,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlUniquePtr(TKrnlUniquePtr<OtherType, PoolType, Mode, DeleterType>&& Other) noexcept
		: FDeleterHolder(DeleterType(MoveTemp(Other.GetDeleter())))
		, Object(static_cast<ObjectType*>(Other.Object))
	{
		Other.Object = nullptr;
	}

	template <
		typename OtherType, 
		typename DeleterArg,
		typename = decltype(ImplicitConv<ObjectType*>((OtherType*)nullptr))
	>
	FORCEINLINE explicit TKrnlUniquePtr(OtherType* InObject, DeleterArg&& InDeleter) noexcept
		: FDeleterHolder(DeleterType(Forward<DeleterArg>(InDeleter)))
		, Object(InObject)
	{
	}

	FORCEINLINE ~TKrnlUniquePtr() noexcept
	{
		Reset();
	}

	FORCEINLINE TKrnlUniquePtr& operator=(TKrnlUniquePtr&& Other) noexcept
	{
		if (this != &Other)
		{
			Reset();
			GetDeleter() = MoveTemp(Other.GetDeleter());
			Object = Other.Object;
			Other.Object = nullptr;
		}
		return *this;
	}

	FORCEINLINE ObjectType* Get() const noexcept { return Object; }
	FORCEINLINE DeleterType& GetDeleter() noexcept { return FDeleterHolder::GetDeleter(); }
	FORCEINLINE ObjectType* operator->() const noexcept { return Object; }
	FORCEINLINE explicit operator bool() const noexcept { return Object != nullptr; }
	FORCEINLINE bool IsValid() const noexcept { return Object != nullptr; }
	FORCEINLINE bool IsUnique() const noexcept { return Object != nullptr; }

	FORCEINLINE void Reset() noexcept
	{
		ObjectType* OldObject = Object;
		Object = nullptr;
		if (OldObject != nullptr)
		{
			this->InvokeDeleter(OldObject);
		}
	}

private:
	FORCEINLINE explicit TKrnlUniquePtr(ObjectType* InObject) noexcept
		: FDeleterHolder()
		, Object(InObject)
	{
	}

	template <EPoolType InPoolType, typename T, typename... ArgsType>
	friend TKrnlUniquePtr<T, InPoolType> MakeUnique(ArgsType&&... Args) noexcept;
	template <class, EPoolType, ESPMode, class> friend class TKrnlUniquePtr;
	template <class, EPoolType, ESPMode> friend class TKrnlSharedPtr;

	ObjectType* Object = nullptr;
};

template <typename T> struct TIsUniquePtr { enum { Value = false }; };
template <typename ObjectType, EPoolType PoolType, ESPMode Mode, typename DeleterType> struct TIsUniquePtr<				  TKrnlUniquePtr<ObjectType, PoolType, Mode, DeleterType>> { enum { Value = true }; };
template <typename ObjectType, EPoolType PoolType, ESPMode Mode, typename DeleterType> struct TIsUniquePtr<const		  TKrnlUniquePtr<ObjectType, PoolType, Mode, DeleterType>> { enum { Value = true }; };
template <typename ObjectType, EPoolType PoolType, ESPMode Mode, typename DeleterType> struct TIsUniquePtr<		 volatile TKrnlUniquePtr<ObjectType, PoolType, Mode, DeleterType>> { enum { Value = true }; };
template <typename ObjectType, EPoolType PoolType, ESPMode Mode, typename DeleterType> struct TIsUniquePtr<const volatile TKrnlUniquePtr<ObjectType, PoolType, Mode, DeleterType>> { enum { Value = true }; };

template< class ObjectTypeA, class ObjectTypeB, EPoolType PoolType, ESPMode Mode >
[[nodiscard]] FORCEINLINE bool operator==(TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrA, TKrnlSharedPtr<ObjectTypeB, PoolType, Mode> const& InSharedPtrB) noexcept
{
	return InSharedPtrA.Get() == InSharedPtrB.Get();
}

template< class ObjectTypeA, class ObjectTypeB, EPoolType PoolType, ESPMode Mode >
[[nodiscard]] FORCEINLINE bool operator!=(TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrA, TKrnlSharedPtr<ObjectTypeB, PoolType, Mode> const& InSharedPtrB) noexcept
{
	return InSharedPtrA.Get() != InSharedPtrB.Get();
}


template <typename ObjectTypeA, EPoolType PoolType, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrA, TYPE_OF_NULLPTR) noexcept
{
	return !InSharedPtrA.IsValid();
}

template <typename ObjectTypeA, EPoolType PoolType, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator!=(TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrA, TYPE_OF_NULLPTR) noexcept
{
	return InSharedPtrA.IsValid();
}


template <typename ObjectTypeA, EPoolType PoolType, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TYPE_OF_NULLPTR, TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrB) noexcept
{
	return !InSharedPtrB.IsValid();
}

template <typename ObjectTypeA, EPoolType PoolType, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator!=(TYPE_OF_NULLPTR, TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrB) noexcept
{
	return InSharedPtrB.IsValid();
}


template <typename ObjectTypeA, typename ObjectTypeB, EPoolType PoolType, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrA, TKrnlWeakPtr<ObjectTypeB, PoolType, Mode> const& InWeakPtrB) noexcept
{
	return InSharedPtrA.Get() == InWeakPtrB.ToSharedPtr().Get();
}

template <typename ObjectTypeA, typename ObjectTypeB, EPoolType PoolType, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator!=(TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrA, TKrnlWeakPtr<ObjectTypeB, PoolType, Mode> const& InWeakPtrB) noexcept
{
	return InSharedPtrA.Get() != InWeakPtrB.ToSharedPtr().Get();
}


#if 0 // Comparison with the old reference-counted UniquePtr.
template <typename ObjectTypeA, typename ObjectTypeB, EPoolType PoolType, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrA, TKrnlUniquePtr<ObjectTypeB, PoolType, Mode> const& InUniquePtrB) noexcept
{
	return InSharedPtrA.Get() == InUniquePtrB.Get();
}
#endif

template <typename ObjectTypeA, typename ObjectTypeB, EPoolType PoolType, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator==(TKrnlWeakPtr<ObjectTypeB, PoolType, Mode> const& InWeakPtrA, TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrB) noexcept
{
	return InWeakPtrA == InSharedPtrB;
}

template <typename ObjectTypeA, typename ObjectTypeB, EPoolType PoolType, ESPMode Mode>
[[nodiscard]] FORCEINLINE bool operator!=(TKrnlWeakPtr<ObjectTypeB, PoolType, Mode> const& InWeakPtrA, TKrnlSharedPtr<ObjectTypeA, PoolType, Mode> const& InSharedPtrB) noexcept
{
	return InWeakPtrA != InSharedPtrB;
}

#if 0 // Previous MakeUnique allocated a reference controller.
template <EPoolType PoolType, typename T, typename... ArgsType>
NODISCARD FORCEINLINE TKrnlUniquePtr<T, PoolType> MakeUnique(ArgsType&&... Args) noexcept
{
	static_assert(alignof(T) <= MEMORY_ALLOCATION_ALIGNMENT,
		"MakeUnique requires an object supported by the pool allocator's alignment");
	void* Storage = FKrnlMemory::Malloc<PoolType>(sizeof(T), 'EhSk');
	if (Storage == nullptr)
	{
		return {};
	}
	T* Object = static_cast<T*>(Storage);
	ConstructItem(Object, Forward<ArgsType>(Args)...);
	return TKrnlUniquePtr<T, PoolType>(Object);
}
#endif

template <EPoolType PoolType, typename T, typename... ArgsType>
NODISCARD FORCEINLINE TKrnlUniquePtr<T, PoolType> MakeUnique(ArgsType&&... Args) noexcept
{
	T* Object = FKrnlMemory::MallocAlignment<PoolType, T>('EhSk');
	if (Object == nullptr)
	{
		return {};
	}

	ConstructItem(Object, Forward<ArgsType>(Args)...);
	return TKrnlUniquePtr<T, PoolType>(Object);
}

template <EPoolType PoolType, typename T, typename... ArgsType>
NODISCARD FORCEINLINE TKrnlSharedPtr<T, PoolType> MakeShared(ArgsType&&... Args) noexcept
{
	T* Object = FKrnlMemory::MallocAlignment<PoolType, T>('EhSk');
	if (Object == nullptr)
	{
		return {};
	}

	ConstructItem(Object, Forward<ArgsType>(Args)...);
	return TKrnlSharedPtr<T, PoolType>(Object);
}
