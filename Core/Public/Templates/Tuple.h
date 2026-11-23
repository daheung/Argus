#pragma once

#include "Core/Public/Templates/TypeTraits.h"
#include "Core/Public/Templates/Template.h"

namespace Core::Private::Tuple
{
    template <size_t Index, typename Type>
    struct TTupleLeaf
    {
    public:
        Type Value;

    public:
        TTupleLeaf() = default;

        template <typename ValueType>
        explicit TTupleLeaf(ValueType&& InValue) noexcept(noexcept(Type(Forward<ValueType>(InValue))))
            : Value(Forward<ValueType>(InValue))
        {}
    };

    template <typename IndexSequence, typename... Types>
    struct TTupleStorage;

    template <size_t... Indices, typename... Types>
    struct TTupleStorage<TIndexSequence<Indices...>, Types...> : TTupleLeaf<Indices, Types>...
    {
        TTupleStorage() = default;

        template <typename... ValueTypes>
        explicit TTupleStorage(ValueTypes&&... InValues) noexcept((noexcept(TTupleLeaf<Indices, Types>(Forward<ValueTypes>(InValues))) && ...))
            : TTupleLeaf<Indices, Types>(Forward<ValueTypes>(InValues))...
        {}
    };

    template <typename TupleType, typename... ValueTypes>
    struct TIsSingleTupleArgument
    {
        static constexpr bool Value = false;
    };

    template <typename TupleType, typename ValueType>
    struct TIsSingleTupleArgument<TupleType, ValueType>
    {
        static constexpr bool Value = TIsSame<typename TDecay<ValueType>::Type, TupleType>::Value;
    };
}

template <typename... ArgsType>
class TTuple : 
    public Core::Private::Tuple::TTupleStorage<TMakeIndexSequence<sizeof...(ArgsType)>, ArgsType...>
{
private:
    using Super = Core::Private::Tuple::TTupleStorage<TMakeIndexSequence<sizeof...(ArgsType)>, ArgsType...>;

public:
    TTuple() = default;

    template <
        typename... ValueTypes,
        typename = typename TEnableIf<sizeof...(ValueTypes) == sizeof...(ArgsType) && !Core::Private::Tuple::TIsSingleTupleArgument<TTuple, ValueTypes...>::Value>::Type
    >
    explicit TTuple(ValueTypes&&... InValues)
        noexcept(noexcept(Super(Forward<ValueTypes>(InValues)...)))
        : Super(Forward<ValueTypes>(InValues)...)
    {}
};

template <typename TupleType>
struct TTupleSize;

template <typename... ArgsType>
struct TTupleSize<TTuple<ArgsType...>>
{
    static constexpr uint32 Value = sizeof...(ArgsType);
};

template <size_t Index, typename TupleType>
struct TTupleElement;

template <size_t Index, typename... ArgsType>
struct TTupleElement<Index, TTuple<ArgsType...>>
{
    using Type = typename TTypeAt<Index, TTypeList<ArgsType...>>::Type;
};

namespace Argus
{
    template <size_t Index, typename... Types>
    NODISCARD FORCEINLINE decltype(auto) Get(TTuple<Types...>& Tuple) noexcept
    {
        static_assert(Index < sizeof...(Types), "Tuple index is out of range");
        using ElementType = typename TTupleElement<Index, TTuple<Types...>>::Type;
        using LeafType = Core::Private::Tuple::TTupleLeaf<Index, ElementType>;
        return (static_cast<LeafType&>(Tuple).Value);
    }

    template <size_t Index, typename... Types>
    NODISCARD FORCEINLINE decltype(auto) Get(const TTuple<Types...>& Tuple) noexcept
    {
        static_assert(Index < sizeof...(Types), "Tuple index is out of range");
        using ElementType = typename TTupleElement<Index, TTuple<Types...>>::Type;
        using LeafType = Core::Private::Tuple::TTupleLeaf<Index, ElementType>;
        return (static_cast<const LeafType&>(Tuple).Value);
    }

    template <size_t Index, typename... Types>
    NODISCARD FORCEINLINE decltype(auto) Get(TTuple<Types...>&& Tuple) noexcept
    {
        static_assert(Index < sizeof...(Types), "Tuple index is out of range");
        using ElementType = typename TTupleElement<Index, TTuple<Types...>>::Type;
        using LeafType = Core::Private::Tuple::TTupleLeaf<Index, ElementType>;
        return static_cast<ElementType&&>(static_cast<LeafType&>(Tuple).Value);
    }

    template <typename... Types>
    NODISCARD FORCEINLINE auto MakeTuple(Types&&... Args)
        noexcept(noexcept(TTuple<typename TDecay<Types>::Type...>(Forward<Types>(Args)...)))
    {
        return TTuple<typename TDecay<Types>::Type...>(Forward<Types>(Args)...);
    }
}
