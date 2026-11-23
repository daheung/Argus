#pragma once
#include "Core/Public/Misc/Types.h"
#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Templates/Template.h"
#include "Kernel/Public/Containers/KrnlSmartPointerFwd.h"
#include "Kernel/Public/Containers/KrnlContainerType.h"
#include "Kernel/Public/Containers/KrnlMemory.h"

#include "Core/Public/Templates/Atomic.h"

namespace SharedPointerInternals
{
	template <typename T>
	using TAtomic = TKernelAtomicCounter<T>;

	struct FNullTag {};

	template <ESPMode InMode> class FKrnlSharedReferencer;
	// The old reference-counted UniquePtr path is kept below for reference.
#if 0
	template <ESPMode InMode> class FKrnlUniqueReferencer;
#endif
	template <ESPMode InMode> class FKrnlWeakReferencer;

	template <ESPMode Mode>
	class TKrnlReferenceControllerBase
	{
		using RefCountType = typename TConditional<Mode == ESPMode::ThreadSafe, TAtomic<RefCounter>, RefCounter>::Type;

	public:
		virtual void DestroyObject() noexcept = 0;
		virtual void DeleteThis() noexcept = 0;

	protected:
		FORCEINLINE explicit TKrnlReferenceControllerBase() noexcept = default;

	public:
		TKrnlReferenceControllerBase(const TKrnlReferenceControllerBase& Other) = delete;
		FORCEINLINE TKrnlReferenceControllerBase& operator=(const TKrnlReferenceControllerBase& Other) = delete;

		virtual ~TKrnlReferenceControllerBase() noexcept {}

	public:
		FORCEINLINE bool IsUnique() const
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				return (StrongReferenceCount.load(Atomic::EMemoryOrder::Relaxed) == 1);
			}
			else
			{
				return (StrongReferenceCount == 1);
			}
		}

		FORCEINLINE RefCounter GetSharedReferenceCounter() const
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				return StrongReferenceCount.load(Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				return StrongReferenceCount;
			}
		}

		FORCEINLINE void AddSharedReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				StrongReferenceCount.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				++StrongReferenceCount;
			}
		}

		FORCEINLINE bool ConditionallyAddSharedReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				RefCounter OldRefCount = StrongReferenceCount.load(Atomic::EMemoryOrder::Relaxed);

				for (; ; )
				{

					if (!OldRefCount)
					{
						return false;
					}

					if (StrongReferenceCount.compare_exchange_weak(OldRefCount, OldRefCount + 1, Atomic::EMemoryOrder::AcqRel))
					{
						return true;
					}
				}
			}
			else
			{
				if (!StrongReferenceCount)
				{
					return false;
				}

				++StrongReferenceCount;
				return true;
			}
		}

		FORCEINLINE void ReleaseSharedReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				if (StrongReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::AcqRel) == 1)
				{
					DestroyObject();
					ReleaseWeakReference(); // Release the implicit weak reference.
				}
			}
			else
			{
				if (--StrongReferenceCount == 0)
				{
					DestroyObject();
					ReleaseWeakReference();
				}
			}
		}

#if 0 // UniquePtr no longer participates in reference counting.
		FORCEINLINE RefCounter GetUniqueReferenceCounter() const
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				return UniqueReferenceCount.load(Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				return UniqueReferenceCount;
			}
		}

		FORCEINLINE void AddUniqueReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				ControlReferenceCount.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
				StrongReferenceCount.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
				UniqueReferenceCount.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				++ControlReferenceCount;
				++StrongReferenceCount;
				++UniqueReferenceCount;
			}
		}

		FORCEINLINE bool ConditionallyAddUniqueReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				RefCounter OldRefCount = StrongReferenceCount.load(Atomic::EMemoryOrder::Relaxed);

				for (; ; )
				{

					if (!OldRefCount)
					{
						return false;
					}

					if (StrongReferenceCount.compare_exchange_weak(OldRefCount, OldRefCount + 1, Atomic::EMemoryOrder::AcqRel))
					{
						ControlReferenceCount.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
						UniqueReferenceCount.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
						return true;
					}
				}
			}
			else
			{
				if (!StrongReferenceCount)
				{
					return false;
				}

				++ControlReferenceCount;
				++StrongReferenceCount;
				++UniqueReferenceCount;
				return true;
			}
		}

		FORCEINLINE void ReleaseUniqueReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				UniqueReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::AcqRel);
				if (StrongReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::AcqRel) == 1)
				{
					DestroyObject();
				}
				if (ControlReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::AcqRel) == 1)
				{
					DeleteThis();
				}
			}
			else
			{
				--UniqueReferenceCount;
				if (--StrongReferenceCount == 0)
				{
					DestroyObject();
				}
				if (--ControlReferenceCount == 0)
				{
					DeleteThis();
				}
			}
		}
#endif

		FORCEINLINE void AddWeakReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				WeakReferenceCount.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				++WeakReferenceCount;
			}
		}

		FORCEINLINE void ReleaseWeakReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				if (WeakReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::AcqRel) == 1)
				{
					DeleteThis();
				}
			}
			else
			{
				if (--WeakReferenceCount == 0)
				{
					DeleteThis();
				}
			}
		}

	private:
		RefCountType StrongReferenceCount{ 0 };
		RefCountType WeakReferenceCount{ 1 }; // Implicit weak reference until the last strong owner is destroyed.
	};

	template <ESPMode Mode>
	class FKrnlSharedReferencer
	{
	public:
		FORCEINLINE FKrnlSharedReferencer()
			: ReferenceController(nullptr)
		{
		}

		FORCEINLINE FKrnlSharedReferencer(TKrnlReferenceControllerBase<Mode>* InReferenceController)
			: ReferenceController(InReferenceController)
		{
			if (InReferenceController != nullptr)
			{
				ReferenceController->AddSharedReference();
			}
		}

		FORCEINLINE FKrnlSharedReferencer(FKrnlSharedReferencer<Mode> const& InSharedReference)
			: ReferenceController(InSharedReference.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddSharedReference();
			}
		}

		FORCEINLINE FKrnlSharedReferencer(FKrnlSharedReferencer<Mode>&& InSharedReference)
			: ReferenceController(InSharedReference.ReferenceController)
		{
			InSharedReference.ReferenceController = nullptr;
		}

		FORCEINLINE FKrnlSharedReferencer(FKrnlWeakReferencer<Mode> const& InWeakReference)
			: ReferenceController(InWeakReference.ReferenceController)
		{
			if (ReferenceController != nullptr && !ReferenceController->ConditionallyAddSharedReference())
			{
				ReferenceController = nullptr;
			}
		}

		FORCEINLINE FKrnlSharedReferencer(FKrnlWeakReferencer<Mode>&& InWeakReference)
			: ReferenceController(InWeakReference.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				const bool bPromoted = ReferenceController->ConditionallyAddSharedReference();
				InWeakReference.ReferenceController = nullptr;
				ReferenceController->ReleaseWeakReference();
				if (!bPromoted)
				{
					ReferenceController = nullptr;
				}
			}
		}

#if 0 // Reference-counted UniquePtr conversion.
		FORCEINLINE FKrnlSharedReferencer(FKrnlUniqueReferencer<Mode>&& InUniqueReference) noexcept
			: ReferenceController(InUniqueReference.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddSharedReference();

				InUniqueReference.ReferenceController->ReleaseUniqueReferenceToMove();
				InUniqueReference.ReferenceController = nullptr;
			}
		}
#endif

		FORCEINLINE ~FKrnlSharedReferencer() noexcept
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->ReleaseSharedReference();
			}
		}

		FORCEINLINE FKrnlSharedReferencer& operator=(FKrnlSharedReferencer const& InSharedReference)
		{
			TKrnlReferenceControllerBase<Mode>* NewSharedReferencer = InSharedReference.ReferenceController;
			if (NewSharedReferencer != ReferenceController)
			{
				if (NewSharedReferencer != nullptr)
				{
					NewSharedReferencer->AddSharedReference();
				}

				if (ReferenceController != nullptr)
				{
					ReferenceController->ReleaseSharedReference();
				}

				ReferenceController = NewSharedReferencer;
			}

			return *this;
		}

		FORCEINLINE FKrnlSharedReferencer& operator=(FKrnlSharedReferencer&& InSharedReference)
		{
			if (this != &InSharedReference)
			{
				TKrnlReferenceControllerBase<Mode>* OldSharedReferencer = ReferenceController;
				ReferenceController = InSharedReference.ReferenceController;
				InSharedReference.ReferenceController = nullptr;
				if (OldSharedReferencer != nullptr)
				{
					OldSharedReferencer->ReleaseSharedReference();
				}
			}

			return *this;
		}

		FORCEINLINE bool IsValied() const noexcept
		{
			return ReferenceController != nullptr;
		}

		FORCEINLINE const RefCounter GetSharedReferenceCount() const noexcept
		{
			return ReferenceController != nullptr ? ReferenceController->GetSharedReferenceCounter() : 0;
		}

		FORCEINLINE bool IsUnique() const noexcept
		{
			return ReferenceController != nullptr && ReferenceController->IsUnique();
		}

	private:
		template <ESPMode OtherMode> friend class FKrnlSharedReferencer;
		template <ESPMode OtherMode> friend class FKrnlWeakReferencer;

	private:
		TKrnlReferenceControllerBase<Mode>* ReferenceController;
	};

	#if 0 // Old reference-counted UniquePtr implementation.
	template <ESPMode Mode>
	class FKrnlUniqueReferencer
	{
	public:
		FORCEINLINE FKrnlUniqueReferencer()
			: ReferenceController()
		{
		}

		FORCEINLINE FKrnlUniqueReferencer(TKrnlReferenceControllerBase<Mode>* InReferenceController)
			: ReferenceController(InReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddUniqueReference();
			}
		}

		FORCEINLINE FKrnlUniqueReferencer(FKrnlUniqueReferencer&& InUniqueReferencer) noexcept
			: ReferenceController(InUniqueReferencer.ReferenceController)
		{
			InUniqueReferencer.ReferenceController = nullptr;
		}

		FORCEINLINE FKrnlUniqueReferencer(FKrnlSharedReferencer<Mode>&& InSharedReferencer) noexcept
			: ReferenceController(InSharedReferencer.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddUniqueReference();

				InSharedReferencer.ReferenceController->ReleaseSharedReference();
				InSharedReferencer.ReferenceController = nullptr;
			}
		}

		FORCEINLINE ~FKrnlUniqueReferencer()
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->ReleaseUniqueReference();
			}
		}

	public:
		FORCEINLINE FKrnlUniqueReferencer& operator=(FKrnlUniqueReferencer&& InUniqueReferencer)
		{
			if (this != &InUniqueReferencer)
			{
				TKrnlReferenceControllerBase<Mode>* OldController = ReferenceController;
				ReferenceController = InUniqueReferencer.ReferenceController;
				InUniqueReferencer.ReferenceController = nullptr;
				if (OldController != nullptr)
				{
					OldController->ReleaseUniqueReference();
				}
			}
			return *this;
		}

	public:
		FORCEINLINE bool IsValied() const noexcept
		{
			return ReferenceController != nullptr;
		}

		FORCEINLINE const RefCounter GetUniqueReferenceCount() const noexcept
		{
			return ReferenceController != nullptr ? ReferenceController->GetUniqueReferenceCounter() : 0;
		}

		FORCEINLINE bool IsUnique() const noexcept
		{
			return ReferenceController != nullptr && ReferenceController->IsUnique();
		}

	private:
		FORCEINLINE void MoveReferenceController(FKrnlUniqueReferencer& InUniqueReferencer)
		{
			if (ReferenceController != InUniqueReferencer.ReferenceController)
			{
				ReferenceController = InUniqueReferencer.ReferenceController;
				InUniqueReferencer.ReferenceController = nullptr;
			}
		}

		FORCEINLINE void MoveOrCopyReferenceController(FKrnlUniqueReferencer& InUniqueReferencer)
		{
			if (ReferenceController != InUniqueReferencer.ReferenceController)
			{
				ReferenceController = MoveTemp(InUniqueReferencer.ReferenceController);
			}
		}

	private:
		template <ESPMode OtherMode> friend class FKrnlSharedReferencer;
		template <ESPMode OtherMode> friend class FKrnlUniqueReferencer;
		template <ESPMode OtherMode> friend class FKrnlWeakReferencer;


	private:
		TKrnlReferenceControllerBase<Mode>* ReferenceController;
	};
	#endif

	template <ESPMode Mode>
	class FKrnlWeakReferencer
	{
	public:
		FORCEINLINE FKrnlWeakReferencer()
			: ReferenceController(nullptr)
		{
		}

		FORCEINLINE FKrnlWeakReferencer(FKrnlWeakReferencer const& InWeakRefCountPointer)
			: ReferenceController(InWeakRefCountPointer.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddWeakReference();
			}
		}

		FORCEINLINE FKrnlWeakReferencer(FKrnlWeakReferencer&& InWeakRefCountPointer)
			: ReferenceController(InWeakRefCountPointer.ReferenceController)
		{
			InWeakRefCountPointer.ReferenceController = nullptr;
		}

		FORCEINLINE FKrnlWeakReferencer(FKrnlSharedReferencer<Mode> const& InSharedRefCountPointer)
			: ReferenceController(InSharedRefCountPointer.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddWeakReference();
			}
		}

		FORCEINLINE FKrnlWeakReferencer(FKrnlSharedReferencer<Mode>&& InSharedRefCountPointer)
			: ReferenceController(InSharedRefCountPointer.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddWeakReference();
				InSharedRefCountPointer.ReferenceController = nullptr;
				ReferenceController->ReleaseSharedReference();
			}
		}

		#if 0 // UniquePtr has no reference controller.
		FORCEINLINE FKrnlWeakReferencer(FKrnlUniqueReferencer<Mode> const& InUniqueRefCountPointer)
			: ReferenceController(InUniqueRefCountPointer.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddWeakReference();
			}
		}
		#endif

		FORCEINLINE ~FKrnlWeakReferencer()
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->ReleaseWeakReference();
			}
		}

	public:
		FORCEINLINE FKrnlWeakReferencer& operator=(FKrnlWeakReferencer const& InWeakReference)
		{
			AssignReferenceController(InWeakReference.ReferenceController);
			return *this;
		}

		FORCEINLINE FKrnlWeakReferencer& operator=(FKrnlWeakReferencer&& InWeakReference) noexcept
		{
			if (this != &InWeakReference)
			{
				TKrnlReferenceControllerBase<Mode>* OldController = ReferenceController;
				ReferenceController = InWeakReference.ReferenceController;
				InWeakReference.ReferenceController = nullptr;
				if (OldController != nullptr)
				{
					OldController->ReleaseWeakReference();
				}
			}
			return *this;
		}

	private:
		FORCEINLINE void AssignReferenceController(TKrnlReferenceControllerBase<Mode>* NewReferenceController)
		{
			if (ReferenceController != NewReferenceController)
			{
				if (NewReferenceController != nullptr)
				{
					NewReferenceController->AddWeakReference();
				}

				if (ReferenceController != nullptr)
				{
					ReferenceController->ReleaseWeakReference();
				}

				ReferenceController = NewReferenceController;
			}
		}


	private:
		template <ESPMode OtherMode> friend class FKrnlSharedReferencer;
		template <ESPMode OTherMode> friend class FKrnlWeakReferencer;

	private:
		TKrnlReferenceControllerBase<Mode>* ReferenceController;
	};

	template <typename DeleterType, bool bIsZeroSize = TIsEmpty<DeleterType>::Value && !__is_final(DeleterType)>
	struct TDeleterHolder :
		private DeleterType
	{
		FORCEINLINE TDeleterHolder() noexcept = default;

		FORCEINLINE explicit TDeleterHolder(DeleterType&& Arg)
			: DeleterType(MoveTemp(Arg))
		{
		}

		template <typename ObjectType>
		void InvokeDeleter(ObjectType* Object)
		{
			Invoke(*static_cast<DeleterType*>(this), Object);
		}

		FORCEINLINE DeleterType& GetDeleter() noexcept
		{
			return *static_cast<DeleterType*>(this);
		}
	};

	template <typename DeleterType>
	struct TDeleterHolder<DeleterType, false>
	{
		FORCEINLINE TDeleterHolder() noexcept = default;

		FORCEINLINE explicit TDeleterHolder(DeleterType&& Arg)
			: Deleter(MoveTemp(Arg))
		{
		}

		template <typename ObjectType>
		void InvokeDeleter(ObjectType* Object)
		{
			Invoke(Deleter, Object);
		}

		FORCEINLINE DeleterType& GetDeleter() noexcept
		{
			return Deleter;
		}

	private:
		DeleterType Deleter;
	};

	template <typename ObjectType, typename DeleterType, EPoolType PoolType, ESPMode Mode>
	class TKrnlReferenceControllerWithDeleter :
		private TDeleterHolder<DeleterType>, public TKrnlReferenceControllerBase<Mode>
	{
	public:
		FORCEINLINE explicit TKrnlReferenceControllerWithDeleter(ObjectType* InObject, DeleterType&& Deleter)
			: TDeleterHolder<DeleterType>(MoveTemp(Deleter))
			, Object(InObject)
		{
		}

		virtual void DestroyObject() noexcept override
		{
			this->InvokeDeleter(Object);
		}

		virtual void DeleteThis() noexcept override
		{
			this->~TKrnlReferenceControllerWithDeleter();
			FKrnlMemory::Free(this);
		}

		FORCEINLINE TKrnlReferenceControllerWithDeleter(const TKrnlReferenceControllerWithDeleter& Other) = delete;
		FORCEINLINE TKrnlReferenceControllerWithDeleter& operator=(const TKrnlReferenceControllerWithDeleter& Other) = delete;

	private:
		ObjectType* Object;
	};

	template <typename Type>
	struct FDefaultDeleter
	{
		template <
			typename PointerType,
			typename = decltype(ImplicitConv<PointerType*>((Type*)nullptr)),
			typename = decltype(static_cast<Type*>((PointerType*)nullptr))
		>
		FORCEINLINE void operator()(PointerType* Object) const noexcept
		{
			if (Object != nullptr)
			{
				// Q1. Why can Type and PointerType be different?
				//
				// A1. The converting TKrnlUniquePtr move constructor accepts an
				// OtherType* when ImplicitConv permits conversion to ObjectType*.
				// Moving UniquePtr<Derived> to UniquePtr<Base> stores a Base* but
				// retains FDefaultDeleter<Derived>. Here Type is Derived and
				// PointerType is Base.
				
				// Q2. Why cast PointerType* back to Type*?
				//
				// A2. A Base* may point inside the Derived allocation. Recovering
				// Derived* makes destruction use the original allocation type and
				// passes the allocation address to FKrnlMemory::Free. ImplicitConv
				// checks only the upcast; the reverse static_cast must also be valid.
				// Virtual base conversions cannot be reversed this way.
				
				// Q3. Why call the destructor directly instead of using DestructItem?
				//
				// A3. DestructItem uses a qualified destructor call, which suppresses
				// virtual dispatch. The unqualified call below preserves virtual
				// destructor dispatch when Type has a virtual destructor.
				Type* Allocation = static_cast<Type*>(Object);
				Allocation->~Type();
				FKrnlMemory::Free(Allocation);
			}
		}
	};

#if 0 // Previous type-erased UniquePtr deleter.
	struct FUniqueDeleter
	{
		using FDestroy = void (*)(void*, void*) noexcept;
		using FReleaseContext = void (*)(void*) noexcept;

		void* Context = nullptr;
		FDestroy Destroy = nullptr;
		FReleaseContext ReleaseContext = nullptr;

		FORCEINLINE void operator()(void* Allocation) const noexcept
		{
			if (Allocation != nullptr && Destroy != nullptr)
			{
				Destroy(Allocation, Context);
			}
			if (Context != nullptr && ReleaseContext != nullptr)
			{
				ReleaseContext(Context);
			}
		}
	};

	template <typename ObjectType>
	FORCEINLINE void DestroyUniquePoolObject(void* Allocation, void*) noexcept
	{
		FDefaultDeleter<ObjectType>{}(static_cast<ObjectType*>(Allocation));
	}

	template <typename ObjectType, typename DeleterType>
	FORCEINLINE void DestroyUniqueCustomObject(void* Allocation, void* Context) noexcept
	{
		Invoke(*static_cast<DeleterType*>(Context), static_cast<ObjectType*>(Allocation));
	}

	template <typename DeleterType>
	FORCEINLINE void ReleaseUniqueDeleterContext(void* Context) noexcept
	{
		DestructItem(static_cast<DeleterType*>(Context));
		FKrnlMemory::Free(Context);
	}
#endif

	template <EPoolType PoolType, ESPMode Mode, typename ObjectType>
	FORCEINLINE TKrnlReferenceControllerBase<Mode>* NewDefaultReferenceController(ObjectType* Object)
	{
		if (Object == nullptr)
		{
			return nullptr;
		}

		using FController = TKrnlReferenceControllerWithDeleter<ObjectType, FDefaultDeleter<ObjectType>, PoolType, Mode>;
		FController* Storage = FKrnlMemory::MallocAlignment<PoolType, FController>('cShK');
		return Storage != nullptr ? ::new (Storage) FController(Object, FDefaultDeleter<ObjectType>()) : nullptr;
	}

	template <EPoolType PoolType, ESPMode Mode, typename ObjectType, typename DeleterType>
	FORCEINLINE TKrnlReferenceControllerBase<Mode>* NewCustomReferenceController(ObjectType* Object, DeleterType&& Deleter)
	{
		if (Object == nullptr)
		{
			return nullptr;
		}

		using FStoredDeleter = typename TRemoveCVReference<DeleterType>::Type;
		using FController = TKrnlReferenceControllerWithDeleter<ObjectType, FStoredDeleter, PoolType, Mode>;
		FController* Storage = FKrnlMemory::MallocAlignment<PoolType, FController>('cShK');
		return Storage != nullptr ? ::new (Storage) FController(Object, FStoredDeleter(Forward<DeleterType>(Deleter))) : nullptr;
	}
};
