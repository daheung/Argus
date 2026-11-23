#pragma once

#include "Kernel/Public/Misc/KernelTypes.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlSmartPointerFwd.h"
#include "Kernel/Public/Containers/KrnlSmartPointer.h"
#include "Kernel/Private/PolicyInterpreter/Graph/GraphNode.h"

namespace Argus::Graph
{
    class FPGraphCompiler;

    struct FPGraphNodeConnection
    {
        uint64 From; // Source output pin ID
        uint64 To;   // Target input pin ID
    };
}

template <>
struct TIsTriviallyRelocatable<Argus::Graph::FPGraphNode>
{
    // The owner contains only a node pointer; relocation transfers ownership.
    static constexpr bool Value = true;
};

namespace Argus::Graph
{
    class FPGraph
    {
        friend class FPGraphCompiler;

    public:
        FPGraph() noexcept = default;
        FPGraph(const FPGraph&) = delete;
        FPGraph& operator=(const FPGraph&) = delete;
        FPGraph(FPGraph&&) = delete;
        FPGraph& operator=(FPGraph&&) = delete;

        ~FPGraph() noexcept = default;

    public:
        NODISCARD FORCEINLINE bool IsCompiled() const noexcept { return bCompiled; }

        template <auto NodeFunction>
        NODISCARD FORCEINLINE bool CreateNodeFrom(uint32& OutNodeId, FPGraphNodeBase<EPoolType::NonPaged>*& OutGraphNode) noexcept
        {
            return CreateNodeImpl<TFunctionNode<NodeFunction>>(OutNodeId, OutGraphNode);
        }

        template <EPGNodeOperatorType Op>
        NODISCARD FORCEINLINE bool CreateNodeOperator(uint32& OutNodeId, FPGraphNodeBase<EPoolType::NonPaged>*& OutGraphNode) noexcept
        {
            return CreateNodeImpl<TOperatorNode<Op>>(OutNodeId, OutGraphNode);
        }

        template <typename T>
        NODISCARD FORCEINLINE bool CreateNodeLiteral(T Value, uint32& OutNodeId, FPGraphNodeBase<EPoolType::NonPaged>*& OutGraphNode) noexcept
        {
            using FValue = typename TRemoveCVReference<T>::Type;
            return CreateNodeImpl<TLiteralNode<FValue>>(OutNodeId, OutGraphNode, MoveTempIfPossible(Value));
        }

        // Target-first argument order matches ConnectNodeFromLiteralBy.
        NODISCARD FORCEINLINE bool ConnectNodeFromNodeBy(
            uint32 GraphNodeId, uint32 GraphNodePinIndex,
            uint32 SourceGraphNodeId, uint32 SourceGraphNodePinIndex) noexcept
        {
            return ConnectBy(SourceGraphNodeId, SourceGraphNodePinIndex, GraphNodeId, GraphNodePinIndex);
        }

        NODISCARD FORCEINLINE bool ConnectNodeFromLiteralBy(uint32 GraphNodeId, uint32 GraphNodePinIndex, uint32 LiteralGraphNodeId) noexcept
        {
            const FPGraphNodeBase<EPoolType::NonPaged>* LiteralNode = FindNode(LiteralGraphNodeId);
            if (LiteralNode == nullptr || LiteralNode->GetNodeType() != EPGNodeType::Literal)
            {
                return false;
            }

            return ConnectBy(LiteralGraphNodeId, 0, GraphNodeId, GraphNodePinIndex);
        }

        NODISCARD FORCEINLINE uint32 GetConnectionCount() const noexcept
        {
            return static_cast<uint32>(Connections.Num());
        }

        NODISCARD FORCEINLINE bool GetConnection(uint32 Index, FPGraphNodeConnection& OutConnection) const noexcept
        {
            const bool bIsValid = Index < GetConnectionCount();
            if (bIsValid)
            {
                OutConnection = Connections[Index];
                return true;
            }

            return  false;
        }

    public:
        NODISCARD FORCEINLINE bool GetNode(uint32 NodeId, const FPGraphNodeBase<EPoolType::NonPaged>*& OutNode) const noexcept
        {
            OutNode = FindNode(NodeId);
            return OutNode != nullptr;
        }

        NODISCARD FORCEINLINE bool GetNodeFrom(uint64 PinId, const FPGraphNodeBase<EPoolType::NonPaged>*& OutNode) const noexcept
        {
            return GetNode(static_cast<uint32>(PinId >> 32), OutNode);
        }

        NODISCARD FORCEINLINE uint64 FindOutputPinId(uint64 InputPinId) const noexcept
        {
            for (int32 Index = 0; Index < Connections.Num(); ++Index)
            {
                if (InputPinId == Connections[Index].To)
                {
                    return Connections[Index].From;
                }
            }

            return static_cast<uint64>(INDEX_NONE);
        }

        // Returns the first input pin connected to SourceOutputPinId, or INDEX_NONE.
        NODISCARD FORCEINLINE uint64 FindInputPinId(uint64 OutputPinId) const noexcept
        {
            for (int32 Index = 0; Index < Connections.Num(); ++Index)
            {
                if (OutputPinId == Connections[Index].From)
                {
                    return Connections[Index].To;
                }
            }

            return static_cast<uint64>(INDEX_NONE);
        }

    private:
        NODISCARD FORCEINLINE const FPGraphNodeBase<EPoolType::NonPaged>* FindNode(uint32 NodeId) const noexcept
        {
            for (int32 Index = 0; Index < Nodes.Num(); ++Index)
            {
                const auto* Node = Nodes[Index].Get();
                if (Node == nullptr)
                {
                    // Unexpected node.
                    CORE_CHECK(false);
                    return nullptr;
                }

                if (Node->GetNodeId() == NodeId)
                {
                    return Node;
                }
            }

            return nullptr;
        }

        // A new Source -> Target edge is cyclic when Target already reaches Source.
        NODISCARD FORCEINLINE bool HasPath(uint32 StartNodeId, uint32 EndNodeId, bool& OutHasPath) const noexcept
        {
            OutHasPath = false;
            TNonPageArray<uint32> Reachable;
            if (Reachable.Push(StartNodeId) == INDEX_NONE)
            {
                return false;
            }

            for (int32 Index = 0; Index < Reachable.Num(); ++Index)
            {
                const uint32 CurrentNodeId = Reachable[Index];
                if (CurrentNodeId == EndNodeId)
                {
                    OutHasPath = true;
                    return true;
                }

                for (int32 EdgeIndex = 0; EdgeIndex < Connections.Num(); ++EdgeIndex)
                {
                    const FPGraphNodeConnection& Edge = Connections[EdgeIndex];
                    if (static_cast<uint32>(Edge.From >> 32) != CurrentNodeId)
                    {
                        continue;
                    }

                    const uint32 NextNode = static_cast<uint32>(Edge.To >> 32);
                    bool bSeen = false;
                    for (int32 SeenIndex = 0; SeenIndex < Reachable.Num(); ++SeenIndex)
                    {
                        if (Reachable[SeenIndex] == NextNode)
                        {
                            bSeen = true;
                            break;
                        }
                    }
                    if (!bSeen && Reachable.Push(NextNode) == INDEX_NONE)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        template <typename NodeType, typename... ArgsType>
        NODISCARD bool CreateNodeImpl(uint32& OutNodeId, FPGraphNodeBase<EPoolType::NonPaged>*& OutNode, ArgsType&&... Args) noexcept
        {
            if (bCompiled || NextNodeId == 0)
            {
                return false;
            }

            NodeType* NewNode = FKrnlMemory::MallocAlignment<EPoolType::NonPaged, NodeType>('NGPg');
            if (NewNode == nullptr)
            {
                return false;
            }

            ConstructItem(NewNode, NextNodeId, Forward<ArgsType>(Args)...);

            // TODO: Verify that the base pointer equals NewNode before a stateless deleter frees it.
            using FConcreteNode = TUniquePtr<NodeType, EPoolType::NonPaged, ESPMode::ThreadSafe>;
            FConcreteNode ConcreteOwner(NewNode);
            if (!TGraphNodeBuilder<NodeType>::Build(*NewNode))
            {
                return false;
            }

            FPGraphNode Owner(MoveTemp(ConcreteOwner));
            FPGraphNodeBase<EPoolType::NonPaged>* NodeView = Owner.Get();
            if (Nodes.Push(FPGraphNode(MoveTemp(Owner))) == INDEX_NONE)
            {
                return false;
            }

            OutNodeId = NextNodeId++;
            OutNode = NodeView;
            return true;
        }

        NODISCARD FORCEINLINE bool ConnectBy(
            uint32 SourceNodeId, uint32 SourcePinIndex,
            uint32 TargetNodeId, uint32 TargetPinIndex) noexcept
        {
            if (bCompiled || SourceNodeId == TargetNodeId)
            {
                return false;
            }

            const FPGraphNodeBase<EPoolType::NonPaged>* Source = FindNode(SourceNodeId);
            const FPGraphNodeBase<EPoolType::NonPaged>* Target = FindNode(TargetNodeId);
            FPGraphNodePin SourcePin{};
            FPGraphNodePin TargetPin{};
            if (Source == nullptr || Target == nullptr ||
                !Source->GetOutputPin(SourcePinIndex, SourcePin) ||
                !Target->GetInputPin(TargetPinIndex, TargetPin) ||
                SourcePin.PinType != TargetPin.PinType)
            {
                return false;
            }

            const uint64 SourcePinId = SourcePin.PinId;
            const uint64 TargetPinId = TargetPin.PinId;
            for (int32 Index = 0; Index < Connections.Num(); ++Index)
            {
                if (Connections[Index].To == TargetPinId)
                {
                    return Connections[Index].From == SourcePinId;
                }
            }

            bool bCreatesCycle = false;
            if (!HasPath(TargetNodeId, SourceNodeId, bCreatesCycle) || bCreatesCycle)
            {
                return false;
            }

            return Connections.Push({ SourcePinId, TargetPinId }) != INDEX_NONE;
        }


    private:
        TNonPageArray<FPGraphNode> Nodes;
        TNonPageArray<FPGraphNodeConnection> Connections;
        uint32 NextNodeId = 1;
        mutable bool bCompiled = false;
    };
}
