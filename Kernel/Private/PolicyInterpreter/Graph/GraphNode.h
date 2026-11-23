#pragma once
#if 0 // Previous FPGraphNode and builder implementation.

#include "Core/Public/Templates/Tuple.h"
#include "Core/Public/Memory/Memory.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlContainerAllocator.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Private/PolicyInterpreter/Graph/GraphNodeTypes.h"
#include "Kernel/Private/PolicyInterpreter/Graph/GraphNodeFunction.h"

namespace Argus::Graph
{
    class FPGraphExecutionContext;
    class FPGraphCompiler;
    class FPGraphExecutor;

    NODISCARD FORCEINLINE constexpr uint64 CreateGraphPinId(uint32 NodeId, uint32 PinIndex) noexcept
    {
        return (static_cast<uint64>(NodeId) << 32) | static_cast<uint64>(PinIndex);
    }

    template <EPGNodePinDirection InDirectionType, typename InValueType>
    struct FPGraphNodePinDefinition
    {
    public:
        using FPinValueType = typename TRemoveCVReference<InValueType>::Type;
        static constexpr EPGNodePinType PinType = TPGNodePinTypeTraits<FPinValueType>::NodeType;
        static constexpr EPGNodePinDirection Direction = InDirectionType;
    };

    //template <EPGNodePinDirection InDirectionType, typename InValueType>
    //struct FPGraphNodeConstantPinDefinition :
    //    public FPGraphNodePinDefinition<InDirectionType, InValueType>
    //{
    //public:
    //    static constexpr bool Constant = true;
    //    typedef FPGraphNodePinDefinition<InDirectionType, InValueType> Super;
    //    typedef typename FPGraphNodePinDefinition<InDirectionType, InValueType>::FPinValueType FPinValueType;
    //};

    //template <EPGNodePinDirection InDirectionType, typename InValueType>
    //struct FPGraphNodeVariablePinDefinition :
    //    public FPGraphNodePinDefinition<InDirectionType, InValueType>
    //{
    //public:
    //    static constexpr bool Constant = false;
    //    typedef FPGraphNodePinDefinition<InDirectionType, InValueType> Super;
    //};

    //template <EPGNodePinDirection DirectionType, typename PinListType>
    //struct TPGraphNodePinDefinitionTupleType;

    //template <EPGNodePinDirection DirectionType, typename... PinListType>
    //struct TPGraphNodePinDefinitionTupleType<DirectionType, TTypeList<PinListType...>>
    //{
    //    static_assert(((PinListType::Direction == DirectionType) && ...), "Pin direction does not match the pin list");
    //    using Type = TTuple<PinListType...>;
    //};
    
    template <auto InNodeFunction>
    struct FPGraphNodeDefinition
    {
    public:
        static constexpr auto NodeFunction = InNodeFunction;
        static constexpr EPGNodeType NodeType = TPGNodeFunctionTypeTraits<InNodeFunction>::NodeType;
        static_assert(NodeType != EPGNodeType::Invalid, "Unsupported graph node function");

        using FunctionTraits = TFunctionTraits<decltype(InNodeFunction)>;
        static_assert(TNot<TIsSame<typename FunctionTraits::ReturnType, void>>::Value, "A graph node function must return a value");
        
        //using TPGInputPinTupleType  = typename TPGraphNodePinDefinitionTupleType<EPGNodePinDirection::DirectionInput ,           typename FunctionTraits::ArgumentsTypes>::Type;
        //using TPGOutputPinTupleType = typename TPGraphNodePinDefinitionTupleType<EPGNodePinDirection::DirectionOutput, TTypeList<typename FunctionTraits::ReturnType>   >::Type;

        static constexpr uint32 InputPinCount = static_cast<uint32>(FunctionTraits::ArgumentCount);
        static constexpr uint32 OutputPinCount = 1;
    };

    //using FBooleanVariableInputPinDefinition = FPGraphNodeVariablePinDefinition<EPGNodePinDirection::DirectionInput, bool>;
    //using FBooleanOutputPinDefinition = FPGraphNodePinDefinition<EPGNodePinDirection::DirectionOutput, bool>;

    //using FPGAndNodeDefinition = FPGraphNodeDefinition<&AndFunction, TTypeList<FBooleanVariableInputPinDefinition, FBooleanVariableInputPinDefinition>, TTypeList<FBooleanOutputPinDefinition>>;
    //using FPGOrNodeDefinition  = FPGraphNodeDefinition<&OrFunction , TTypeList<FBooleanVariableInputPinDefinition, FBooleanVariableInputPinDefinition>, TTypeList<FBooleanOutputPinDefinition>>;
    //using FPGNotNodeDefinition = FPGraphNodeDefinition<&NotFunction, TTypeList<FBooleanVariableInputPinDefinition>, TTypeList<FBooleanOutputPinDefinition>>;

    template <auto NodeFunction>
    struct TPGraphNodeBuilder;

    template <typename T>
    struct TPGraphNodeLiteralBuilder;

    class FPGraphNodePin
    {
        friend class FPGraphNode;

        template <auto NodeFunction>
        friend struct TPGraphNodeBuilder;

        template <typename T>
        friend struct TPGraphNodeLiteralBuilder;

    private:
        uint64 PinId;

        EPGNodePinDirection Direction;

        EPGNodePinType PinType;

    public:
        bool bEnableDefaultValue;
    };

    class FPGraphNode
    {
        template <auto NodeFunction>
        friend struct TPGraphNodeBuilder;

        template <typename T>
        friend struct TPGraphNodeLiteralBuilder;

    private:
        uint32 NodeId = 0;
        EPGNodeType NodeType = EPGNodeType::Invalid;

        TArray<EPoolType::NonPaged, FPGraphNodePin> InputPins;
        TArray<EPoolType::NonPaged, FPGraphNodePin> OutputPins;

        Argus::Allocator::FPoolBlock LiteralStorage{};
        const void* (*LiteralExecute)(const void*) noexcept = nullptr;
        void (*LiteralDestroy)(void*) noexcept = nullptr;

        FORCEINLINE void DestroyLiteral() noexcept
        {
            if (LiteralDestroy != nullptr)
            {
                LiteralDestroy(LiteralStorage.Allocation);
            }
            Argus::Allocator::FreePoolBlock(LiteralStorage);
            LiteralExecute = nullptr;
            LiteralDestroy = nullptr;
        }

    public:
        FPGraphNode() noexcept = default;
        FPGraphNode(const FPGraphNode&) = delete;
        FPGraphNode& operator=(const FPGraphNode&) = delete;

        FPGraphNode(FPGraphNode&& Other) noexcept
            : NodeId(Other.NodeId)
            , NodeType(Other.NodeType)
            , InputPins(MoveTempIfPossible(Other.InputPins))
            , OutputPins(MoveTempIfPossible(Other.OutputPins))
            , LiteralStorage(Other.LiteralStorage)
            , LiteralExecute(Other.LiteralExecute)
            , LiteralDestroy(Other.LiteralDestroy)
        {
            Other.NodeId = 0;
            Other.NodeType = EPGNodeType::Invalid;
            Other.LiteralStorage = {};
            Other.LiteralExecute = nullptr;
            Other.LiteralDestroy = nullptr;
        }

        FPGraphNode& operator=(FPGraphNode&&) = delete;
        ~FPGraphNode() noexcept { DestroyLiteral(); }

        NODISCARD FORCEINLINE uint32 GetNodeId() const noexcept { return NodeId; }
        NODISCARD FORCEINLINE EPGNodeType GetNodeType() const noexcept { return NodeType; }
        NODISCARD FORCEINLINE uint32 GetInputPinCount()  const noexcept { return static_cast<uint32>(InputPins.Num()); }
        NODISCARD FORCEINLINE uint32 GetOutputPinCount() const noexcept { return static_cast<uint32>(OutputPins.Num()); }
        NODISCARD FORCEINLINE bool IsLiteral() const noexcept
        {
            return NodeType == EPGNodeType::Literal && LiteralExecute != nullptr;
        }

        NODISCARD FORCEINLINE EPGNodePinType GetInputPinType(uint32 Index) const noexcept
        {
            CORE_CHECK(Index < GetInputPinCount());
            return InputPins[Index].PinType;
        }

        NODISCARD FORCEINLINE EPGNodePinType GetOutputPinType(uint32 Index) const noexcept
        {
            CORE_CHECK(Index < GetOutputPinCount());
            return OutputPins[Index].PinType;
        }

        NODISCARD FORCEINLINE uint64 GetInputPinId(uint32 Index) const noexcept
        {
            if (Index >= GetInputPinCount())
            {
                return static_cast<uint64>(INDEX_NONE);
            }

            return this->InputPins[Index].PinId;
        }

        NODISCARD FORCEINLINE uint64 GetOutputPinId(uint32 Index) const noexcept 
        { 
            if (Index >= GetOutputPinCount())
            {
                return static_cast<uint64>(INDEX_NONE);
            }

            return this->OutputPins[Index].PinId; 
        }

        template <typename T>
        NODISCARD FORCEINLINE const typename TRemoveCVReference<T>::Type* GetLiteralValue() const noexcept
        {
            using TValue = typename TRemoveCVReference<T>::Type;
            if (!IsLiteral() || OutputPins.Num() != 1 ||
                OutputPins[0].PinType != TPGNodePinTypeTraits<TValue>::NodeType)
            {
                return nullptr;
            }
            return static_cast<const TValue*>(LiteralExecute(LiteralStorage.Allocation));
        }
        

        // 연결이 없는 입력에 대해 Node가 기본값을 제공
        //virtual bool TryGetDefaultInput(uint16 InputIndex, FValue& Out) const noexcept = 0;

        //// Graph가 해석해서 전달한 입력값으로 결과 생성
        //// 실제로 실행할 때, 함수를 제공. Node는 함수 시그니쳐만 들고 있음
        //virtual EGraphStatus Evaluate(
        //EGraphStatus Evaluate(
        //    const FEventContext& Event,
        //    const FValue* Inputs,
        //    FValue* Outputs) const noexcept = 0;
    };

    template <auto NodeFunction>
    struct TPGraphNodeBuilder
    {
        using FGraphNodeDefinition = FPGraphNodeDefinition<NodeFunction>;
        using FunctionTraits = typename FGraphNodeDefinition::FunctionTraits;

        template <size_t Index>
        static FORCEINLINE bool AddInputPin(uint32 NodeId, FPGraphNode& Node) noexcept
        {
            using ArgumentType = typename FunctionTraits::template ArgumentType<Index>;
            using FPinDefinition = FPGraphNodePinDefinition<EPGNodePinDirection::DirectionInput, ArgumentType>;

            FPGraphNodePin Pin{};
            Pin.PinId     = CreateGraphPinId(NodeId, static_cast<uint32>(Index));
            Pin.Direction = FPinDefinition::Direction;
            Pin.PinType   = FPinDefinition::PinType;
            Pin.bEnableDefaultValue = false;
            return Node.InputPins.Push(MoveTempIfPossible(Pin)) != INDEX_NONE;
        }

        template <size_t... Indices>
        static FORCEINLINE bool AddInputPins(uint32 NodeId, FPGraphNode& Node, TIndexSequence<Indices...>) noexcept
        {
            return (AddInputPin<Indices>(NodeId, Node) && ...);
        }

        static FORCEINLINE bool Build(uint32 NodeId, FPGraphNode& OutGraphNode) noexcept
        {
            if (OutGraphNode.NodeType != EPGNodeType::Invalid ||
                !OutGraphNode.InputPins.IsEmpty() || !OutGraphNode.OutputPins.IsEmpty())
            {
                return false;
            }

            FPGraphNode Node{};
            if (!AddInputPins(NodeId, Node, TMakeIndexSequence<FGraphNodeDefinition::InputPinCount>{}))
            {
                return false;
            }

            using FOutputPinDefinition = FPGraphNodePinDefinition<EPGNodePinDirection::DirectionOutput, typename FunctionTraits::ReturnType>;

            FPGraphNodePin OutputPin{};
            OutputPin.PinId     = CreateGraphPinId(NodeId, FGraphNodeDefinition::InputPinCount);
            OutputPin.Direction = FOutputPinDefinition::Direction;
            OutputPin.PinType   = FOutputPinDefinition::PinType;
            OutputPin.bEnableDefaultValue = false;
            if (Node.OutputPins.Push(MoveTempIfPossible(OutputPin)) == INDEX_NONE)
            {
                return false;
            }

            TArray<EPoolType::NonPaged, FPGraphNodePin>::Swap(OutGraphNode.InputPins , Node.InputPins);
            TArray<EPoolType::NonPaged, FPGraphNodePin>::Swap(OutGraphNode.OutputPins, Node.OutputPins);
            OutGraphNode.NodeId = NodeId;
            OutGraphNode.NodeType = FGraphNodeDefinition::NodeType;
            return true;
        }
    };

    template <auto NodeFunction>
    NODISCARD FORCEINLINE bool BuildGraphNodeFrom(uint32 NodeId, FPGraphNode& OutGraphNode) noexcept
    {
        return TPGraphNodeBuilder<NodeFunction>::Build(NodeId, OutGraphNode);
    }

    // 나중에 구조 고처야함 (동적할당 없이)
    template <typename T>
    struct TPGraphNodeLiteralBuilder
    {
        using TValue = typename TRemoveCVReference<T>::Type;

        static FORCEINLINE const void* Execute(const void* Storage) noexcept
        {
            return Storage;
        }

        static FORCEINLINE void Destroy(void* Storage) noexcept
        {
            DestructItems(static_cast<TValue*>(Storage), 1);
        }

        static FORCEINLINE bool Build(T Value, uint32 NodeId, FPGraphNode& OutNode) noexcept
        {
            if (OutNode.NodeType != EPGNodeType::Invalid ||
                !OutNode.InputPins.IsEmpty() || !OutNode.OutputPins.IsEmpty() ||
                OutNode.LiteralStorage.PoolPointer != nullptr)
            {
                return false;
            }

            FPGraphNode Node{};
            FPGraphNodePin OutputPin{};
            OutputPin.PinId = CreateGraphPinId(NodeId, 0);
            OutputPin.Direction = EPGNodePinDirection::DirectionOutput;
            OutputPin.PinType = TPGNodePinTypeTraits<TValue>::NodeType;
            OutputPin.bEnableDefaultValue = false;
            if (Node.OutputPins.Push(MoveTempIfPossible(OutputPin)) == INDEX_NONE)
            {
                return false;
            }

            if (!Argus::Allocator::AllocatePoolBlock<EPoolType::NonPaged>(
                    sizeof(TValue), alignof(TValue), 'LGPg', Node.LiteralStorage))
            {
                return false;
            }
            ConstructItem(static_cast<TValue*>(Node.LiteralStorage.Allocation), MoveTempIfPossible(Value));
            Node.LiteralExecute = &Execute;
            Node.LiteralDestroy = &Destroy;

            TArray<EPoolType::NonPaged, FPGraphNodePin>::Swap(OutNode.OutputPins, Node.OutputPins);
            OutNode.LiteralStorage = Node.LiteralStorage;
            OutNode.LiteralExecute = Node.LiteralExecute;
            OutNode.LiteralDestroy = Node.LiteralDestroy;
            OutNode.NodeId = NodeId;
            OutNode.NodeType = EPGNodeType::Literal;

            Node.LiteralStorage = {};
            Node.LiteralExecute = nullptr;
            Node.LiteralDestroy = nullptr;
            return true;
        }
    };

    template <typename T>
    NODISCARD FORCEINLINE bool BuildGraphNodeLiteral(T Value, uint32 NodeId, FPGraphNode& OutNode) noexcept
    {
        return TPGraphNodeLiteralBuilder<T>::Build(MoveTempIfPossible(Value), NodeId, OutNode);
    }
}
#endif

#include "Core/Public/Memory/Memory.h"
#include "Core/Public/Templates/TypeTraits.h"
#include "Core/Public/Templates/IsTriviallyRelocatable.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlSmartPointer.h"
#include "Kernel/Private/PolicyInterpreter/Graph/GraphNodeFunction.h"

namespace Argus::Graph
{
    struct FPGraphNodePin
    {
        uint64 PinId = 0;
        EPGNodePinDirection Direction = EPGNodePinDirection::DirectionInput;
        EPGNodePinType PinType = EPGNodePinType::Bool;
        bool bEnableDefaultValue = false;
    };

    NODISCARD FORCEINLINE constexpr uint64 CreateGraphPinId(uint32 NodeId, uint32 PinIndex) noexcept
    {
        return (static_cast<uint64>(NodeId) << 32) | PinIndex;
    }

    template <typename NodeType>
    struct TGraphNodeBuilder;

    template <EPoolType InPoolType>
    class FPGraphNodeBase
    {
        template <typename NodeType> 
        friend struct TGraphNodeBuilder;

    public:
        static constexpr EPoolType PoolType = InPoolType;

        virtual ~FPGraphNodeBase() noexcept = default;
        FPGraphNodeBase(const FPGraphNodeBase&) = delete;
        FPGraphNodeBase& operator=(const FPGraphNodeBase&) = delete;

        NODISCARD FORCEINLINE uint32 GetNodeId() const noexcept 
        { 
            return NodeId; 
        }

        NODISCARD FORCEINLINE EPGNodeType GetNodeType() const noexcept 
        { 
            return NodeType; 
        }

        NODISCARD FORCEINLINE uint32 GetInputPinCount() const noexcept 
        { 
            return static_cast<uint32>(InputPins.Num()); 
        }

        NODISCARD FORCEINLINE uint32 GetOutputPinCount() const noexcept 
        { 
            return static_cast<uint32>(OutputPins.Num()); 
        }

        NODISCARD FORCEINLINE bool GetInputPin(uint32 Index, FPGraphNodePin& OutPin) const noexcept
        {
            if (Index >= GetInputPinCount())
            {
                return false;
            }

            OutPin = InputPins[Index];
            return true;
        }

        NODISCARD FORCEINLINE bool GetOutputPin(uint32 Index, FPGraphNodePin& OutPin) const noexcept
        {
            if (Index >= GetOutputPinCount())
            {
                return false;
            }

            OutPin = OutputPins[Index];
            return true;
        }

    protected:
        FORCEINLINE explicit FPGraphNodeBase(uint32 InNodeId, EPGNodeType InNodeType) noexcept
            : NodeId(InNodeId)
            , NodeType(InNodeType) 
        {}

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
        FORCEINLINE T operator()(ArgsType&&... Args) const noexcept
        {
            return T{ Forward<ArgsType>(Args)... };
        }
    };

    template <typename T, EPoolType InPoolType = EPoolType::NonPaged, typename ConstructorType = TDefaultLiteralConstructor<T>>
    class TLiteralNode final : 
        public FPGraphNodeBase<InPoolType>
    {
        using FBase = FPGraphNodeBase<InPoolType>;

    public:
        template <typename... ArgsType>
        explicit TLiteralNode(uint32 InNodeId, ArgsType&&... Args) noexcept
            : FBase(InNodeId, EPGNodeType::Literal)
            , Literal(MakeUnique<InPoolType, T>(Invoke(ConstructorType{}, Forward<ArgsType>(Args)...)))
        {
        }

        NODISCARD FORCEINLINE bool HasValue() const noexcept { return Literal.IsValid(); }
        NODISCARD FORCEINLINE const T* GetValue() const noexcept { return Literal.Get(); }

    private:
        TUniquePtr<T, InPoolType> Literal;
    };

    template <auto Fn, EPoolType InPoolType = EPoolType::NonPaged>
    class TFunctionNode final : 
        public FPGraphNodeBase<InPoolType>
    {
        using FBase = FPGraphNodeBase<InPoolType>;

    public:
        using FTraits = TFunctionTraits<decltype(Fn)>;
        static constexpr auto Function = Fn;

        FORCEINLINE explicit TFunctionNode(uint32 InNodeId) noexcept
            : FBase(InNodeId, EPGNodeType::Function) 
        {}
    };

    template <EPGNodeOperatorType Op> struct TOperatorNodeTraits;
    template <> struct TOperatorNodeTraits<EPGNodeOperatorType::And> : TFunctionTraits<decltype(&And)> { static constexpr auto Function = &And; };
    template <> struct TOperatorNodeTraits<EPGNodeOperatorType::Or > : TFunctionTraits<decltype(&Or) > { static constexpr auto Function = &Or; };
    template <> struct TOperatorNodeTraits<EPGNodeOperatorType::Not> : TFunctionTraits<decltype(&Not)> { static constexpr auto Function = &Not; };

    template <EPGNodeOperatorType Op, EPoolType InPoolType = EPoolType::NonPaged>
    class TOperatorNode final : 
        public FPGraphNodeBase<InPoolType>
    {
        using FBase = FPGraphNodeBase<InPoolType>;

    public:
        using FTraits = TOperatorNodeTraits<Op>;
        static constexpr EPGNodeOperatorType Operator = Op;

        FORCEINLINE explicit TOperatorNode(uint32 InNodeId) noexcept
            : FBase(InNodeId, EPGNodeType::Operator) 
        {}
    };

    template <typename NodeType>
    struct TGraphNodeBuilder
    {
        using FTraits = typename NodeType::FTraits;
        using FReturnType = typename FTraits::ReturnType;
        static_assert(!TIsSame<FReturnType, void>::Value, "A graph node function must return a value");

        template <size_t Index>
        static bool AddInputPin(NodeType& Node) noexcept
        {
            using FArgument = typename TRemoveCVReference<typename FTraits::template ArgumentType<Index>>::Type;
            const FPGraphNodePin Pin{ CreateGraphPinId(Node.GetNodeId(), static_cast<uint32>(Index)),
                EPGNodePinDirection::DirectionInput, TPGNodePinTypeTraits<FArgument>::NodeType, false };
            return Node.InputPins.Push(Pin) != INDEX_NONE;
        }

        template <size_t... Indices>
        static bool AddInputPins(NodeType& Node, TIndexSequence<Indices...>) noexcept
        {
            return (TGraphNodeBuilder<NodeType>::AddInputPin<Indices>(Node) && ...);
        }

        static bool Build(NodeType& Node) noexcept
        {
            if (Node.GetInputPinCount() != 0 || Node.GetOutputPinCount() != 0 ||
                !AddInputPins(Node, TMakeIndexSequence<FTraits::ArgumentCount>{}))
            {
                return false;
            }

            using FOutput = typename TRemoveCVReference<FReturnType>::Type;
            const FPGraphNodePin Pin{ CreateGraphPinId(Node.GetNodeId(), static_cast<uint32>(FTraits::ArgumentCount)),
                EPGNodePinDirection::DirectionOutput, TPGNodePinTypeTraits<FOutput>::NodeType, false };
            return Node.OutputPins.Push(Pin) != INDEX_NONE;
        }
    };

    template <typename T, EPoolType InPoolType, typename ConstructorType>
    struct TGraphNodeBuilder<TLiteralNode<T, InPoolType, ConstructorType>>
    {
        using FNode = TLiteralNode<T, InPoolType, ConstructorType>;

        static bool Build(FNode& Node) noexcept
        {
            if (!Node.HasValue() || Node.GetInputPinCount() != 0 || Node.GetOutputPinCount() != 0)
            {
                return false;
            }

            const FPGraphNodePin Pin{ CreateGraphPinId(Node.GetNodeId(), 0),
                EPGNodePinDirection::DirectionOutput, TPGNodePinTypeTraits<T>::NodeType, false };
            return Node.OutputPins.Push(Pin) != INDEX_NONE;
        }
    };

#if 0
    struct FGraphNodeDeleter
    {
        using FBase = FPGraphNodeBase<EPoolType::NonPaged>;

        FORCEINLINE void operator()(FBase* Node) const noexcept
        {
            if (Node != nullptr)
            {
                Node->~FBase();
                FKrnlMemory::Free(Node);
            }
        }
    };

    using FGraphNodePtr = TUniquePtr<FPGraphNodeBase<EPoolType::NonPaged>,
        EPoolType::NonPaged, ESPMode::ThreadSafe, FGraphNodeDeleter>;
#endif

    using FPGraphNode = TUniquePtr<FPGraphNodeBase<EPoolType::NonPaged>, EPoolType::NonPaged, ESPMode::ThreadSafe>;
}
