#pragma once

#include "Kernel/Public/Misc/KernelTypes.h"

namespace Argus::Graph
{
    class FPGraph;
    class FPGraphCompiler;
    class FPExecutableGraph;

    /**
     * @brief Holds the values used by one execution context of a compiled graph.
     *
     * Each context owns its graph input values, node input values, intermediate
     * node output values, and final graph output value. Compilation determines
     * the required slot counts and storage sizes; they remain fixed while the
     * context is executed. Values may be updated or reset between executions
     * without changing the layout.
     *
     * A compiled FPGraph is immutable and may be shared by multiple contexts.
     * Each concurrent execution must use its own context. A context may be
     * reused for sequential executions after its values are reset. The Graph
     * reference is non-owning, so the graph must outlive every context bound
     * to it.
     *
     * @warning Value storage and layout initialization are not implemented yet.
     */
    class FPExecutableGraphContext
    {
        friend class FPGraphCompiler;

    public:
        NODISCARD FORCEINLINE bool IsReady() const noexcept { return Graph != nullptr; }
        NODISCARD FORCEINLINE const FPGraph* GetGraph() const noexcept { return Graph; }

    private:
        const FPGraph* Graph = nullptr;
    };
}
