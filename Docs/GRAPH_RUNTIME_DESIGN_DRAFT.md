# Graph runtime design draft

Status: design only. No implementation is implied by this document.

## Confirmed direction

- A graph represents one expression or rule as typed nodes connected by input and output pins. It is a directed acyclic data-flow graph; a tree is a special case.
- `FNodeBase` contains the common node identity and pin interface. The node families are `TLiteralNode<T>`, `TFunctionNode<Fn>`, and `TOperatorNode<Op>`.
- `Fn` is a C++ template argument known when the kernel binary is built. Runtime graph compilation selects only pre-registered node kinds; it does not create or compile new C++ functions.
- A function node's pin count may differ between functions, but each `TFunctionNode<Fn>` has a fixed signature. Its input types and count come from `TFunctionTraits<decltype(Fn)>`; its output type comes from the return type. Instance-specific variadic pins are outside the first version.
- A literal node has no input pins and one output pin of type `T`. An operator node uses a fixed schema selected by `Op`. `And`, `Or`, and `Not` are the first operators.
- `FGraph` owns nodes and edges. No shared ownership is needed between nodes: edges use IDs, and the graph owns every node until all users of its compiled form have finished.

## Node lifetime and execution

- Graph node creation must allocate the size and alignment of the concrete derived type, construct that type, and record its owning pool block. The current `sizeof(FPGraphNode)` allocation cannot hold derived nodes.
- Destruction through the base must invoke the concrete node destructor before releasing its pool block. `TLiteralNode<T>` owns `T`, so destroying the derived node invokes the correct `T` destructor. The implementation must verify this path with the project's manual construction and pool allocator.
- Template node types establish schemas and typed execution adapters at C++ compile time. The graph compiler binds those adapters to a runtime execution plan. Runtime input values remain in an execution context, not in shared graph nodes.
- `TOperatorNode<Op>` remains separate from function nodes where operator semantics or validation differ. Version 1 preserves the rule protocol's eager postfix evaluation of `And`, `Or`, and `Not`; short-circuit evaluation is a separate semantic change.

## Class responsibilities

1. `FGraph`: owns nodes and connections, identifies one result output, and remains mutable only before successful compilation. It retains the immutable execution order and value-slot layout after compilation.
2. `FGraphCompiler`: validates node schemas, pins, connections, required inputs, the result output, acyclicity, supported operations, and resource bounds. It prepares an execution order and slot layout off to the side, then commits them only on success.
3. `FGraphExecutionContext`: owns per-execution input, intermediate, and output values using the compiled layout. Compilation may create the first context, but further contexts must be creatable from the same compiled graph for concurrent executions. A context never owns the graph.
4. `FGraphExecutor`: takes a compiled graph, one context, and an event input, then invokes nodes in the compiled order and returns the result. It does not change the graph or allocate an execution plan during an event callback.

## Runtime rule boundary

- The current Agent-to-Kernel protocol sends bounded postfix predicate/operator records, not C++ objects or function pointers. The kernel must validate and translate these records into graph nodes using an allowlist of registered node kinds.
- Existing wire limits are 64 KiB per request, 4,096 predicates, and 8,192 instructions. Graph compilation must also bound nodes, pins, edges, value-slot bytes, and execution work.
- Rule registration performs decoding, validation, graph construction, compilation, and publication outside the event evaluation path. A failed registration must leave the previously published rule intact.
- A compiled graph can be shared, but an execution context cannot be used concurrently. Rule replacement must keep the old graph alive until its current executions finish.
- Node objects and values accessed by an event callback must use memory and operations valid at that callback's IRQL.

## Current code constraints

- `GraphNode.h` already uses function traits to derive pin schemas and has literal-specific storage and destruction callbacks, but function nodes do not yet retain an execution adapter.
- `Graph.h` allocates and constructs only `FPGraphNode`, so its factory and destruction paths must change before adding derived node objects.
- `GraphCompiler.cpp` returns `NotImplemented`; `GraphExecutionContext.h` currently stores only a non-owning graph pointer; there is no `GraphExecutor` implementation.
- `RegisterRuleProtocol.h` defines the current postfix wire format. The design should adapt this format internally rather than put kernel pointers or C++ template types on the wire.

## Implementation order after design review

1. Define pin schema and node contracts, including the result output and fixed function/operator signatures.
2. Refactor graph-owned allocation, typed construction, and derived destruction; add literal, function, and operator node families.
3. Implement graph validation and compilation into a stable execution order and slot layout, committing only after all checks and allocations succeed.
4. Implement context creation and reset from the compiled layout, then the executor for one Boolean result.
5. Add postfix-to-graph translation at rule registration, bounded validation, atomic publication, and concurrent lifetime handling.
6. Verify typed destruction, schema rejection, connection/cycle handling, compile rollback, repeated and concurrent execution, and registration failure behavior.
