#pragma once

#include "Kernel/Public/Misc/KernelTypes.h"
#include "Kernel/Private/PolicyInterpreter/Graph.h"
#include "Kernel/Private/PolicyInterpreter/Graph/GraphExecutionContext.h"

namespace Argus::Graph
{
	class FPGraph;

	enum class EPGraphCompileStatus : uint8
	{
		Success,
		InvalidGraph,
		//InvalidOperator,
		AllocationFailed,
		LimitExceeded,
		NotImplemented,
	};

	class FPGraphCompiler
	{
	public:
		/**
		 * @brief Compiles an FPGraph into bytecode and prepares its execution context.
		 *
		 * Before returning Success, validate node and pin schemas, connections,
		 * required inputs, the single result root, and acyclic dependencies. Reject
		 * invalid literals or function data and enforce graph and storage limits.
		 * Emit instructions in dependency order so each input value is available
		 * before its consumer executes.
		 *
		 * On success, OutExecutableGraph owns the bytecode, constants, and value
		 * slot layout needed for execution. OutExecutableGraphContext owns the
		 * temporary value storage allocated for that layout and is initialized for
		 * OutExecutableGraph. The source Graph is not modified and need only remain
		 * valid during compilation. OutExecutableGraph must outlive its context;
		 * concurrent executions require separate contexts.
		 *
		 * On failure, leave both output objects unchanged. The caller must not
		 * modify Graph concurrently with compilation.
		 *
		 * @param Graph Source graph to compile.
		 * @param OutExecutableGraph Receives the independently owned bytecode and
		 *        execution layout on success.
		 * @param OutExecutableGraphContext Receives temporary value storage
		 *        initialized for OutExecutableGraph on success.
		 * @return Success, InvalidGraph, AllocationFailed, LimitExceeded, or NotImplemented.
		 * @warning Bytecode generation and context initialization are not implemented yet.
		 */
		static EPGraphCompileStatus Compile(const FPGraph& Graph, FPExecutableGraph& OutExecutableGraph, FPExecutableGraphContext& OutExecutableGraphContext) noexcept;

	private:
		using FNode = FPGraphNodeBase<EPoolType::NonPaged>;

		static bool IsRoot(const FPGraph& Graph, const FNode& Node) noexcept
		{
			for (int32 Index = 0; Index < Graph.Connections.Num(); ++Index)
			{
				if (static_cast<uint32>(Graph.Connections[Index].From >> 32) == Node.GetNodeId())
				{
					return false;
				}
			}
			return true;
		}

		static bool IsLeaf(const FPGraph& Graph, const FNode& Node) noexcept
		{
			for (int32 Index = 0; Index < Graph.Connections.Num(); ++Index)
			{
				if (static_cast<uint32>(Graph.Connections[Index].To >> 32) == Node.GetNodeId())
				{
					return false;
				}
			}
			return true;
		}

		// Returns the first node with no incoming connections.
		static bool FindLeaf(const FPGraph& Graph, const FNode*& OutLeaf, bool* bOutInvalidGraph = nullptr) noexcept
		{
			OutLeaf = nullptr;
			const FNode* Root = nullptr;
			const bool bRootFound = FindRoot(Graph, Root);
			if (!bRootFound)
			{
				return false;
			}
			if (Root == nullptr)
			{
				return false;
			}

			const auto FindChildFunc = [&Graph](const FNode* const Node, bool bInExceptLiteralNode, const FNode*& OutChild, bool* bOutIsInvalidGraph = nullptr) -> bool
			{
				bool bSawUnconnectedInput = false;
				bool bHasChild = false;
				const uint32 InputPinCount = Node->GetInputPinCount();
				for (uint32 Index = 0; Index < InputPinCount; ++Index)
				{
					FPGraphNodePin InputPin;
					const bool bInvalidPin = !Node->GetInputPin(Index, InputPin);
					if (bInvalidPin)
					{
						// Node의 상태가 유효하지 않음 (TOCTOU 가능성 존재)
						if (bOutIsInvalidGraph)
						{
							*bOutIsInvalidGraph = true;
						}

						return false;
					}

					const uint64 ChildPin = Graph.FindOutputPinId(InputPin.PinId);
					if (ChildPin != INDEX_NONE)
					{
						// An input may be left open only after all connected inputs.
						if (bSawUnconnectedInput)
						{
							if (bOutIsInvalidGraph)
							{
								*bOutIsInvalidGraph = true;
							}
							return false;
						}

						const FNode* ChildNode = Graph.FindNode(static_cast<uint32>(ChildPin >> 32));
						if (ChildNode == nullptr)
						{
							// Child Pin은 존재하는데, Pin을 소유하고 있는 Node가 존재하지 않음
							CORE_CHECK(false);

							if (bOutIsInvalidGraph)
							{
								*bOutIsInvalidGraph = true;
							}

							// Child Id가 존재하기 때문에 true 반환
							return true;
						}

						if (bInExceptLiteralNode && ChildNode->GetNodeType() == EPGNodeType::Literal)
						{
							continue;
						}
						else
						{
							if (OutChild == nullptr)
							{
								OutChild = ChildNode;
							}

							bHasChild = true;
						}
					}
					else
					{
						bSawUnconnectedInput = true;
					}
				}

				return bHasChild;
			};

			const FNode* Node = Root;
			const FNode* ChildNode = nullptr;
			while (true)
			{
				bool bIsInvalidGraph = false;
				const bool bFoundChild = FindChildFunc(Node, true, ChildNode, &bIsInvalidGraph);
				if (bIsInvalidGraph)
				{
					if (bOutInvalidGraph)
					{
						*bOutInvalidGraph = true;
					}

					return false;
				}

				if (!bFoundChild)
				{
					const bool bFoundLiteral = FindChildFunc(Node, false, ChildNode, &bIsInvalidGraph);
					if (bFoundLiteral)
					{
						OutLeaf = ChildNode;
					}
					else
					{
						OutLeaf = Node;
					}

					return true;
				}

				Node = ChildNode;
				ChildNode = nullptr;
			}

			return false;
		}

		// Follows the lowest connected input pin at each node. Literal nodes have no special priority.
		static bool FindLeafByInputPinOrder(const FPGraph& Graph, const FNode*& OutLeaf, bool* bOutInvalidGraph = nullptr) noexcept
		{
			OutLeaf = nullptr;
			if (bOutInvalidGraph)
			{
				*bOutInvalidGraph = false;
			}

			const auto FailInvalidGraph = [bOutInvalidGraph]() noexcept -> bool
			{
				if (bOutInvalidGraph)
				{
					*bOutInvalidGraph = true;
				}
				return false;
			};

			const FNode* Node = nullptr;
			if (!FindRoot(Graph, Node))
			{
				return FailInvalidGraph();
			}

			for (int32 Depth = 0; Depth < Graph.Nodes.Num(); ++Depth)
			{
				const FNode* FirstChild = nullptr;
				bool bSawUnconnectedInput = false;
				for (uint32 Index = 0; Index < Node->GetInputPinCount(); ++Index)
				{
					FPGraphNodePin InputPin{};
					if (!Node->GetInputPin(Index, InputPin))
					{
						return FailInvalidGraph();
					}

					const uint64 ChildPin = Graph.FindOutputPinId(InputPin.PinId);
					if (ChildPin == INDEX_NONE)
					{
						bSawUnconnectedInput = true;
						continue;
					}
					if (bSawUnconnectedInput)
					{
						return FailInvalidGraph();
					}

					const FNode* ChildNode = Graph.FindNode(static_cast<uint32>(ChildPin >> 32));
					if (ChildNode == nullptr)
					{
						return FailInvalidGraph();
					}
					if (FirstChild == nullptr)
					{
						FirstChild = ChildNode;
					}
				}

				if (FirstChild == nullptr)
				{
					OutLeaf = Node;
					return true;
				}
				Node = FirstChild;
			}

			return FailInvalidGraph();
		}

		// A compiled expression needs exactly one node with no outgoing connections.
		static bool FindRoot(const FPGraph& Graph, const FNode*& OutRoot) noexcept
		{
			OutRoot = nullptr;
			for (int32 Index = 0; Index < Graph.Nodes.Num(); ++Index)
			{
				const FNode* Candidate = Graph.Nodes[Index].Get();
				if (Candidate == nullptr)
				{
					OutRoot = nullptr;
					return false;
				}

				if (IsRoot(Graph, *Candidate))
				{
					if (OutRoot != nullptr)
					{
						OutRoot = nullptr;
						return false;
					}

					OutRoot = Candidate;
				}
			}

			return OutRoot != nullptr;
		}

		static EPGraphCompileStatus CheckGraph(const FPGraph& Graph) noexcept
		{
			/* 1. Check that graph has root. */
			const FNode* Root = nullptr;
			if (!FindRoot(Graph, Root) || Root == nullptr)
			{
				return EPGraphCompileStatus::InvalidGraph;
			}

			/* 2. Check that graph's nodes are fully connected. */
			// Every connection must identify a real output and input pin. An input
			// pin may have only one source, while an output may feed several inputs.
			for (int32 EdgeIndex = 0; EdgeIndex < Graph.Connections.Num(); ++EdgeIndex)
			{
				const FPGraphNodeConnection& Edge = Graph.Connections[EdgeIndex];
				const FNode* Source = Graph.FindNode(static_cast<uint32>(Edge.From >> 32));
				const FNode* Target = Graph.FindNode(static_cast<uint32>(Edge.To   >> 32));
				FPGraphNodePin OutputPin{};
				FPGraphNodePin InputPin{};

				if (Source == nullptr || Target == nullptr)
				{
					return EPGraphCompileStatus::InvalidGraph;
				}
				if (!Target->GetInputPin (static_cast<uint32>(Edge.To  ), InputPin) ||
					!Source->GetOutputPin(static_cast<uint32>(Edge.From), OutputPin))
				{
					return EPGraphCompileStatus::InvalidGraph;
				}
				if (OutputPin.Direction != EPGNodePinDirection::DirectionOutput ||
					InputPin .Direction != EPGNodePinDirection::DirectionInput)
				{
					return EPGraphCompileStatus::InvalidGraph;
				}
				if (OutputPin.PinId != Edge.From || InputPin.PinId != Edge.To)
				{
					return EPGraphCompileStatus::InvalidGraph;
				}
				if (OutputPin.PinType != InputPin.PinType)
				{
					return EPGraphCompileStatus::InvalidGraph;
				}

				for (int32 PreviousIndex = 0; PreviousIndex < EdgeIndex; ++PreviousIndex)
				{
					if (Graph.Connections[PreviousIndex].To == Edge.To)
					{
						return EPGraphCompileStatus::InvalidGraph;
					}
				}
			}

			TArray<EPoolType::NonPaged, const FNode*> VisitedNodes;
			const EContainerStatus InitStatus = VisitedNodes.Initialize(Graph.Nodes.Num());
			if (InitStatus == EContainerStatus::SizeOverflow)
			{
				return EPGraphCompileStatus::LimitExceeded;
			}
			if (InitStatus != EContainerStatus::Success)
			{
				return EPGraphCompileStatus::AllocationFailed;
			}

			const bool bPushSucceed = VisitedNodes.Push(Root) == INDEX_NONE;
			if (!bPushSucceed)
			{
				return EPGraphCompileStatus::AllocationFailed;
			}

			// Walk every dependency, including branches that FindLeaf does not select.
			for (int32 VisitIndex = 0; VisitIndex < VisitedNodes.Num(); ++VisitIndex)
			{
				const FNode* Node = VisitedNodes[VisitIndex];
				bool bSawUnconnectedInput = false;
				for (uint32 PinIndex = 0; PinIndex < Node->GetInputPinCount(); ++PinIndex)
				{
					FPGraphNodePin InputPin{};
					if (!Node->GetInputPin(PinIndex, InputPin))
					{
						return EPGraphCompileStatus::InvalidGraph;
					}

					const uint64 SourcePinId = Graph.FindOutputPinId(InputPin.PinId);
					if (SourcePinId == INDEX_NONE)
					{
						bSawUnconnectedInput = true;
						continue;
					}
					if (bSawUnconnectedInput)
					{
						return EPGraphCompileStatus::InvalidGraph;
					}

					const FNode* Source = Graph.FindNode(static_cast<uint32>(SourcePinId >> 32));
					if (Source == nullptr)
					{
						return EPGraphCompileStatus::InvalidGraph;
					}

					bool bVisited = false;
					for (int32 SeenIndex = 0; SeenIndex < VisitedNodes.Num(); ++SeenIndex)
					{
						if (VisitedNodes[SeenIndex] == Source)
						{
							bVisited = true;
							break;
						}
					}
					if (!bVisited && VisitedNodes.Push(Source) == INDEX_NONE)
					{
						return EPGraphCompileStatus::AllocationFailed;
					}
				}
			}

			return VisitedNodes.Num() == Graph.Nodes.Num()
				? EPGraphCompileStatus::Success : EPGraphCompileStatus::InvalidGraph;
		}
	};
}
