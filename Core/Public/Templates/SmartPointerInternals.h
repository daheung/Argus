#pragma once
#include "Core/Public/Misc/Types.h"
#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Templates/Template.h"
#include "SmartPointerFwd.h"

#if defined(PLATFORM_WDM) || defined(PLATOFRM_KMDF)
	#include "Core/Public/Templates/Atomic.h"
#else
	#include <atomic>
#endif

namespace SharedPointerInternals
{
#if defined(PLATFORM_WDM) || defined(PLATOFRM_KMDF)
	template <typename T>
	using TAtomic = TKernelAtomicCounter<T>;
#else
	template <typename T>
	using TAtomic = std::atomic<T>;
#endif

	struct FNullTag {};

	template <ESPMode InMode> class FSharedReferencer;
	template <ESPMode InMode> class FUniqueReferencer;
	template <ESPMode InMode> class FWeakReferencer;

	template <ESPMode Mode>
	class TReferenceControllerBase
	{
		using RefCountType = typename TConditional<Mode == ESPMode::ThreadSafe, TAtomic<RefCounter>, RefCounter>::Type;

	public:
		virtual void DestroyObject() noexcept = 0;
		virtual void DeleteThis() noexcept = 0;

	protected:
		FORCEINLINE explicit TReferenceControllerBase() noexcept = default;

	public:
		TReferenceControllerBase(const TReferenceControllerBase& Other) = delete;
		FORCEINLINE TReferenceControllerBase& operator=(const TReferenceControllerBase& Other) = delete;

		virtual ~TReferenceControllerBase() noexcept {}

	public:
		FORCEINLINE bool IsUnique() const
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				return (SharedReferenceCount.load(Atomic::EMemoryOrder::Relaxed) == 1);
			}
			else
			{
				return (SharedReferenceCount == 1);
			}
		}

		FORCEINLINE RefCounter GetSharedReferenceCounter() const
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				return SharedReferenceCount.load(Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				return SharedReferenceCount;
			}
		}

		FORCEINLINE void AddSharedReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				SharedReferenceCount.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				++SharedReferenceCount;
			}
		}

		FORCEINLINE bool ConditionallyAddSharedReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				RefCounter OldRefCount = SharedReferenceCount.load(Atomic::EMemoryOrder::Relaxed);

				for (; ; )
				{

					if (!OldRefCount)
					{
						return false;
					}

					if (SharedReferenceCount.compare_exchange_weak(OldRefCount, OldRefCount + 1, Atomic::EMemoryOrder::Relaxed))
					{
						return true;
					}
				}
			}
			else
			{
				if (!SharedReferenceCount)
				{
					return false;
				}

				++SharedReferenceCount;
				return true;
			}
		}

		FORCEINLINE void ReleaseSharedReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{

				RefCounter OldRefCount = SharedReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::Relaxed);

				if (OldRefCount == 1)
				{
					if (!UniqueReferenceCount)
					{
						DestroyObject();
						DeleteThis();
					}
				}
			}
			else
			{
				if (!(--SharedReferenceCount))
				{
					if (!UniqueReferenceCount)
					{
						DestroyObject();
						DeleteThis();
					}
				}
			}
		}

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
				UniqueReferenceCount.fetch_add(1, Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				++UniqueReferenceCount;
			}
		}

		FORCEINLINE bool ConditionallyAddUniqueReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				RefCounter OldRefCount = UniqueReferenceCount.load(Atomic::EMemoryOrder::Relaxed);

				for (; ; )
				{

					if (!OldRefCount)
					{
						return false;
					}

					if (UniqueReferenceCount.compare_exchange_weak(OldRefCount, OldRefCount + 1, Atomic::EMemoryOrder::Relaxed))
					{
						return true;
					}
				}
			}
			else
			{
				if (!UniqueReferenceCount)
				{
					return false;
				}

				++UniqueReferenceCount;
				return true;
			}
		}

		FORCEINLINE void ReleaseUniqueReference()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				RefCounter OldRefCount = UniqueReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::Relaxed);

				if (OldRefCount == 1)
				{
					DestroyObject();
					DeleteThis();
				}
			}
			else
			{
				if (!(--UniqueReferenceCount))
				{
					DestroyObject();
					DeleteThis();
				}
			}
		}

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
				RefCounter OldRefCount = WeakReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::AcqRel);

				if (OldRefCount == 1)
				{
					delete this;
				}
			}
			else
			{
				if (!(--WeakReferenceCount))
				{
					delete this;
				}
			}
		}

	private:
		FORCEINLINE void ReleaseUniqueReferenceToMove()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				UniqueReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				--UniqueReferenceCount;
			}
		}

		FORCEINLINE void ReleaseSharedReferenceToMove()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				SharedReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::Relaxed);
			}
			else
			{
				--SharedReferenceCount;
			}
		}

		FORCEINLINE void ReleaseWeakReferenceToMove()
		{
			if constexpr (Mode == ESPMode::ThreadSafe)
			{
				WeakReferenceCount.fetch_sub(1, Atomic::EMemoryOrder::AcqRel);
			}
			else
			{
				--WeakReferenceCount;
			}
		}

	private:
		template <ESPMode OtherMode> friend class FSharedReferencer;

	private:
		RefCountType UniqueReferenceCount{ 0 };
		RefCountType SharedReferenceCount{ 0 };
		RefCountType WeakReferenceCount{ 0 };
	};

	template <ESPMode Mode>
	class FSharedReferencer
	{
	public:
		FORCEINLINE FSharedReferencer()
			: ReferenceController(nullptr)
		{
		}

		FORCEINLINE FSharedReferencer(TReferenceControllerBase<Mode>* InReferenceController)
			: ReferenceController(InReferenceController)
		{
			if (InReferenceController != nullptr)
			{
				ReferenceController->AddSharedReference();
			}
		}

		FORCEINLINE FSharedReferencer(FSharedReferencer<Mode> const& InSharedReference)
			: ReferenceController(InSharedReference.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddSharedReference();
			}
		}

		FORCEINLINE FSharedReferencer(FSharedReferencer<Mode>&& InSharedReference)
			: ReferenceController(InSharedReference.ReferenceController)
		{
			InSharedReference.ReferenceController = nullptr;
		}

		FORCEINLINE FSharedReferencer(FWeakReferencer<Mode> const& InWeakReference)
			: ReferenceController(InWeakReference.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddSharedReference();
			}
		}

		FORCEINLINE FSharedReferencer(FUniqueReferencer<Mode>&& InUniqueReference) noexcept
			: ReferenceController(InUniqueReference.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddSharedReference();

				InUniqueReference.ReferenceController->ReleaseUniqueReferenceToMove();
				InUniqueReference.ReferenceController = nullptr;
			}
		}

		FORCEINLINE ~FSharedReferencer() noexcept
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->ReleaseSharedReference();
			}
		}

		FORCEINLINE FSharedReferencer& operator=(FSharedReferencer const& InSharedReference)
		{
			TReferenceControllerBase<Mode>* NewSharedReferencer = InSharedReference.ReferenceController;
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

		FORCEINLINE FSharedReferencer& operator=(FSharedReferencer&& InSharedReference)
		{
			TReferenceControllerBase<Mode>* NewSharedReferencer = InSharedReference.ReferenceController;
			TReferenceControllerBase<Mode>* OldSharedReferencer = ReferenceController;

			if (NewSharedReferencer != OldSharedReferencer)
			{
				InSharedReference.ReferenceController = nullptr;
				ReferenceController = NewSharedReferencer;

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
		template <ESPMode OtherMode> friend class FSharedReferencer;
		template <ESPMode OtherMode> friend class FUniqueReferencer;
		template <ESPMode OtherMode> friend class FWeakReferencer;

	private:
		TReferenceControllerBase<Mode>* ReferenceController;
	};

	template <ESPMode Mode>
	class FUniqueReferencer
	{
	public:
		FORCEINLINE FUniqueReferencer()
			: ReferenceController()
		{
		}

		FORCEINLINE FUniqueReferencer(TReferenceControllerBase<Mode>* InReferenceController)
			: ReferenceController(InReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddUniqueReference();
			}
		}

		FORCEINLINE FUniqueReferencer(FUniqueReferencer&& InUniqueReferencer) noexcept
			: ReferenceController(InUniqueReferencer.ReferenceController)
		{
			InUniqueReferencer.ReferenceController = nullptr;
		}

		FORCEINLINE FUniqueReferencer(FSharedReferencer<Mode>&& InSharedReferencer) noexcept
			: ReferenceController(InSharedReferencer.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddUniqueReference();

				InSharedReferencer.ReferenceController->ReleaseSharedReference();
				InSharedReferencer.ReferenceController = nullptr;
			}
		}

		FORCEINLINE ~FUniqueReferencer()
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->ReleaseUniqueReference();
			}
		}

	public:
		FORCEINLINE FUniqueReferencer& operator=(FUniqueReferencer&& InUniqueReferencer)
		{
			ReferenceController = InUniqueReferencer.ReferenceController;
			InUniqueReferencer.ReferenceController = nullptr;
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
		FORCEINLINE void MoveReferenceController(FUniqueReferencer& InUniqueReferencer)
		{
			if (ReferenceController != InUniqueReferencer.ReferenceController)
			{
				ReferenceController = InUniqueReferencer.ReferenceController;
				InUniqueReferencer.ReferenceController = nullptr;
			}
		}

		FORCEINLINE void MoveOrCopyReferenceController(FUniqueReferencer& InUniqueReferencer)
		{
			if (ReferenceController != InUniqueReferencer.ReferenceController)
			{
				ReferenceController = MoveTemp(InUniqueReferencer.ReferenceController);
			}
		}

	private:
		template <ESPMode OtherMode> friend class FSharedReferencer;
		template <ESPMode OtherMode> friend class FUniqueReferencer;
		template <ESPMode OtherMode> friend class FWeakReferencer;

		template <typename OtherType, ESPMode OtherMode> friend void TUniquePtr<OtherType, OtherMode>::Reset(TUniquePtr<OtherType, OtherMode>&);
		template <typename OtherType, ESPMode OtherMode> friend void TUniquePtr<OtherType, OtherMode>::Reset(TUniquePtr<OtherType, OtherMode>&&);

	private:
		TReferenceControllerBase<Mode>* ReferenceController;
	};

	template <ESPMode Mode>
	class FWeakReferencer
	{
	public:
		FORCEINLINE FWeakReferencer()
			: ReferenceController(nullptr)
		{
		}

		FORCEINLINE FWeakReferencer(FWeakReferencer const& InWeakRefCountPointer)
			: ReferenceController(InWeakRefCountPointer.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddWeakReference();
			}
		}

		FORCEINLINE FWeakReferencer(FWeakReferencer&& InWeakRefCountPointer)
			: ReferenceController(InWeakRefCountPointer.ReferenceController)
		{
			InWeakRefCountPointer.ReferenceController = nullptr;
		}

		FORCEINLINE FWeakReferencer(FSharedReferencer<Mode> const& InSharedRefCountPointer)
			: ReferenceController(InSharedRefCountPointer.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddWeakReference();
			}
		}

		FORCEINLINE FWeakReferencer(FUniqueReferencer<Mode> const& InUniqueRefCountPointer)
			: ReferenceController(InUniqueRefCountPointer.ReferenceController)
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->AddWeakReference();
			}
		}

		FORCEINLINE ~FWeakReferencer()
		{
			if (ReferenceController != nullptr)
			{
				ReferenceController->ReleaseWeakReference();
			}
		}

	public:
		FORCEINLINE FWeakReferencer& operator=(FWeakReferencer const& InWeakReference)
		{
			AssignReferenceController(InWeakReference.ReferenceController);
			return *this;
		}

	private:
		FORCEINLINE void AssignReferenceController(TReferenceControllerBase<Mode>* NewReferenceController)
		{
			if (ReferenceController != NewReferenceController)
			{
				if (NewReferenceController != nullptr)
				{
					NewReferenceController->ReleaseWeakReference();
				}

				if (ReferenceController != nullptr)
				{
					ReferenceController->ReleaseWeakReference();
				}

				ReferenceController = NewReferenceController;
			}
		}


	private:
		template <ESPMode OtherMode> friend class FSharedReferencer;
		template <ESPMode OtherMode> friend class FUniqueReferencer;
		template <ESPMode OTherMode> friend class FWeakReferencer;

	private:
		TReferenceControllerBase<Mode>* ReferenceController;
	};


	template <typename DeleterType, bool bIsZeroSize = TIsEmpty<DeleterType>::Value>
	struct TDeleterHolder :
		private DeleterType
	{
		FORCEINLINE explicit TDeleterHolder(DeleterType&& Arg)
			: DeleterType(MoveTemp(Arg))
		{
		}

		template <typename ObjectType>
		void InvokeDeleter(ObjectType* Object)
		{
			Invoke(*static_cast<DeleterType*>(this), Object);
		}
	};

	template <typename DeleterType>
	struct TDeleterHolder<DeleterType, false>
	{
		FORCEINLINE explicit TDeleterHolder(DeleterType&& Arg)
			: Deleter(MoveTemp(Arg))
		{
		}

		template <typename ObjectType>
		void InvokeDeleter(ObjectType* Object)
		{
			Invoke(Deleter, Object);
		}

	private:
		DeleterType Deleter;
	};

	template <typename ObjectType, typename DeleterType, ESPMode Mode>
	class TReferenceControllerWithDeleter :
		private TDeleterHolder<DeleterType>, public TReferenceControllerBase<Mode>
	{
	public:
		FORCEINLINE explicit TReferenceControllerWithDeleter(ObjectType* InObject, DeleterType&& Deleter)
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
			delete this;
		}

		FORCEINLINE TReferenceControllerWithDeleter(const TReferenceControllerWithDeleter& Other) = delete;
		FORCEINLINE TReferenceControllerWithDeleter& operator=(const TReferenceControllerWithDeleter& Other) = delete;

	private:
		ObjectType* Object;
	};

	template <typename Type>
	struct DefaultDeleter
	{
		FORCEINLINE void operator()(Type* Object) const
		{
			delete Object;
			Object = nullptr;
		}
	};

	template <ESPMode Mode, typename ObjectType>
	FORCEINLINE TReferenceControllerBase<Mode>* NewDefaultReferenceController(ObjectType* Object)
	{
		return new TReferenceControllerWithDeleter<ObjectType, DefaultDeleter<ObjectType>, Mode>(Object, DefaultDeleter<ObjectType>());
	}

	template <ESPMode Mode, typename ObjectType, typename DeleterType>
	FORCEINLINE TReferenceControllerBase<Mode>* NewCustomReferenceController(ObjectType* Object, DeleterType&& Deleter)
	{
		return new TReferenceControllerWithDeleter<ObjectType, typename TRemoveReference<DeleterType>::Type, Mode>(Object, Forward<DeleterType>(Deleter));
	}
};