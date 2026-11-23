#include "GraphCompiler.h"
#include "Kernel/Private/PolicyInterpreter/Graph.h"

Argus::Graph::EPGraphCompileStatus Argus::Graph::FPGraphCompiler::Compile(const FPGraph& Graph, FPExecutableGraph& OutExecutableGraph, FPExecutableGraphContext& OutExecutableGraphContext) noexcept
{
    UNREFERENCED_PARAMETER(Graph);
    UNREFERENCED_PARAMETER(OutExecutableGraph);
    UNREFERENCED_PARAMETER(OutExecutableGraphContext);

    /* 1. Check graph invalid. */
    EPGraphCompileStatus Status = CheckGraph(Graph);
    if (Status != EPGraphCompileStatus::Success)
    {
        return Status;
    }
    
    /* 2. Start compile graph. */
    const FNode* LhsLeaf = nullptr;
    const bool bValieLhsLeaf = FindLeafByInputPinOrder(Graph, LhsLeaf, nullptr);
    if (!bValieLhsLeaf)
    {
        return EPGraphCompileStatus::InvalidGraph;
    }

    

    return EPGraphCompileStatus::NotImplemented;
}
