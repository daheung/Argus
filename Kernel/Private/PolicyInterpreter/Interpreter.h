#if 0
#pragma once

#include "Core/Public/Memory/Memory.h"
#include "Kernel/Public/Misc/KernelTypes.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"

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

namespace Argus::Graph
{
    struct FPGraphNodePin
    {
        uint64 PinId;
        EPGNodePinDirection Direction;
        EPGNodePinType PinType;
        bool bEnableDefaultValue;
    };

    constexpr FORCEINLINE uint64 CreateGraphPinId(uint32 NodeId, uint32 PinIndex) noexcept
    {
        return (static_cast<uint64>(NodeId) << 32) | PinIndex;
    }

    template <EPoolType InPoolType>
    class FPGraphNodeBase
    {
    public:
        static constexpr EPoolType PoolType = InPoolType;

        virtual ~FPGraphNodeBase() noexcept = default;

        FPGraphNodeBase(const FPGraphNodeBase&) = delete;
        FPGraphNodeBase& operator=(const FPGraphNodeBase&) = delete;

    public:
        uint32 GetNodeId() const noexcept { return NodeId; }
        EPGNodeType GetNodeType() const noexcept { return NodeType; }

        FORCEINLINE uint32 GetInputPinCount()  const noexcept { return static_cast<uint32>(InputPins .Num()); }
        FORCEINLINE uint32 GetOutputPinCount() const noexcept { return static_cast<uint32>(OutputPins.Num()); }

        FORCEINLINE bool GetInputPin(uint32 Index, FPGraphNodePin& OutPin) const noexcept
        {
            if (Index >= GetInputPinCount())
            {
                return false;
            }

            OutPin = InputPins[Index];
            return true;
        }

        FORCEINLINE bool GetOutputPin(uint32 Index, FPGraphNodePin& OutPin) const noexcept
        {
            if (Index >= GetOutputPinCount())
            {
                return false;
            }

            OutPin = OutputPins[Index];
            return true;
        }

    protected:
        FPGraphNodeBase(uint32 InNodeId, EPGNodeType InNodeType) noexcept
            : NodeId(InNodeId)
            , NodeType(InNodeType)
        {
        }

    private:
        uint32 NodeId;
        EPGNodeType NodeType;

        TArray<PoolType, FPGraphNodePin> InputPins;
        TArray<PoolType, FPGraphNodePin> OutputPins;
    };

    template <typename T>
    struct TDefaultLiteralConstructor
    {
        template <typename... ArgsType>
        T operator()(ArgsType&&... Args) const noexcept
        {
            return T{ Forward<ArgsType>(Args)... };
        }
    };

    template <typename ConstructorType>
    class TLiteralNodeObjectController
    {
    protected:
        template <typename T, typename... ArgsType>
        static T InvokeConstructor(ArgsType&&... Args) noexcept
        {
            return Invoke(ConstructorType{}, Forward<ArgsType>(Args)...);
        }
    };

    template <typename T, EPoolType InPoolType, typename ConstructorType>
    class TLiteralNode final : 
        public FPGraphNodeBase<InPoolType>, private TLiteralNodeObjectController<ConstructorType>
    {
    private:
        using FBase = FPGraphNodeBase<InPoolType>;
        using FController = TLiteralNodeObjectController<ConstructorType>;

    public:
        template <typename... ArgsType>
        TLiteralNode(uint32 InNodeId, ArgsType&&... Args) noexcept
            : FBase(InNodeId, EPGNodeType::Literal)
            , Value(FController::template InvokeConstructor<T>(Forward<ArgsType>(Args)...))
        {
        }

        const T& GetValue() const noexcept
        {
            return Value;
        }

    private:
        T Value;
    };

    template <auto Fn, EPoolType InPoolType = EPoolType::NonPaged>
    class TFunctionNode final : 
        public FPGraphNodeBase<InPoolType>
    {
        using FBase = FPGraphNodeBase<InPoolType>;

    public:
        using FTraits = TFunctionTraits<decltype(Fn)>;
        static constexpr auto Function = Fn;

        explicit TFunctionNode(uint32 InNodeId) noexcept
            : FBase(InNodeId, EPGNodeType::Function)
        {
        }
    };

    template <EPGNodeOperatorType Op>
    struct TOperatorNodeTraits;

    template <EPGNodeOperatorType Op, EPoolType InPoolType>
    class TOperatorNode final
        : public FPGraphNodeBase<InPoolType>
    {
        using FBase = FPGraphNodeBase<InPoolType>;

    public:
        using FTraits = TOperatorNodeTraits<Op>;
        static constexpr EPGNodeOperatorType Operator = Op;

        explicit TOperatorNode(uint32 InNodeId) noexcept
            : FBase(InNodeId, EPGNodeType::Operator)
        {
        }
    };
}

#endif