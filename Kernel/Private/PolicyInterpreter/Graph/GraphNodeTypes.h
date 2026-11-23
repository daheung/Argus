#pragma once

#include "Core/Public/Templates/TypeTraits.h"
#include "Kernel/Public/Misc/KernelTypes.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"

namespace Argus::Graph
{
    enum class EPGNodeType : uint8
    {
        Unknown,
        Literal,
        Function,
        Operator
    };

    enum class EPGNodeOperatorType : uint8
    {
        And,
        Or,
        Not
    };

    enum class EPGNodePinType : uint8
    {
        Bool,
        Int8, 
        UInt8,
        Int16, 
        UInt16,
        Int32, 
        UInt32,
        Int64, 
        UInt64,
        PageString,
    };

    enum class EPGNodePinDirection : uint8
    {
        DirectionInput,
        DirectionOutput
    };

    template <typename T>
    struct TPGNodePinTypeTraits;

    template <> struct TPGNodePinTypeTraits<bool       > { using Type = bool       ; static constexpr EPGNodePinType NodeType = EPGNodePinType::Bool; };
    template <> struct TPGNodePinTypeTraits<int8       > { using Type = int8       ; static constexpr EPGNodePinType NodeType = EPGNodePinType::Int8; };
    template <> struct TPGNodePinTypeTraits<uint8      > { using Type = uint8      ; static constexpr EPGNodePinType NodeType = EPGNodePinType::UInt8; };
    template <> struct TPGNodePinTypeTraits<int16      > { using Type = int16      ; static constexpr EPGNodePinType NodeType = EPGNodePinType::Int16; };
    template <> struct TPGNodePinTypeTraits<uint16     > { using Type = uint16     ; static constexpr EPGNodePinType NodeType = EPGNodePinType::UInt16; };
    template <> struct TPGNodePinTypeTraits<int32      > { using Type = int32      ; static constexpr EPGNodePinType NodeType = EPGNodePinType::Int32; };
    template <> struct TPGNodePinTypeTraits<uint32     > { using Type = uint32     ; static constexpr EPGNodePinType NodeType = EPGNodePinType::UInt32; };
    template <> struct TPGNodePinTypeTraits<int64      > { using Type = int64      ; static constexpr EPGNodePinType NodeType = EPGNodePinType::Int64; };
    template <> struct TPGNodePinTypeTraits<uint64     > { using Type = uint64     ; static constexpr EPGNodePinType NodeType = EPGNodePinType::UInt64; };
    template <> struct TPGNodePinTypeTraits<FPageString> { using Type = FPageString; static constexpr EPGNodePinType NodeType = EPGNodePinType::PageString; };

    template<typename FuncType>
    struct TFunctionInvoker
    {
    public:
        using FunctionTraits = TFunctionTraits<FuncType>;
        using ReturnType = typename FunctionTraits::ReturnType;
        using ArgumentTypes = typename FunctionTraits::ArgumentTypes;

        template <typename TInputs>
        static FORCEINLINE ReturnType Invoke(FuncType Func, TInputs& Inputs)
        {
            return TFunctionInvoker<FuncType>::InvokeImpl(Func, Inputs, TMakeIndexSequence<FunctionTraits::ArgumentCount>{});
        }

    private:
        template <typename TInputs, size_t... Indices>
        static FORCEINLINE ReturnType InvokeImpl(FuncType Func, TInputs& Inputs, TIndexSequence<Indices...>)
        {
            return ::Invoke<FuncType>(Func, Inputs.template GetValue<typename TTypeAt<Indices, ArgumentTypes>::Type>(Indices)...);
        }
    };
}
