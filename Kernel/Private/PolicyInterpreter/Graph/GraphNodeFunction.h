#pragma once

#include "Kernel/Public/Misc/KernelTypes.h"
#include "Kernel/Private/PolicyInterpreter/Graph/GraphNodeTypes.h"

namespace Argus::Graph
{
	bool And(bool, bool) noexcept;
	bool Or (bool, bool) noexcept;
	bool Not(bool) noexcept;

	template <auto NodeFunction>
	struct TPGNodeFunctionTypeTraits
	{
		static constexpr EPGNodeType NodeType = EPGNodeType::Unknown;
	};

	template <> struct TPGNodeFunctionTypeTraits<&And> { static constexpr EPGNodeType NodeType = EPGNodeType::Operator; };
	template <> struct TPGNodeFunctionTypeTraits<&Or > { static constexpr EPGNodeType NodeType = EPGNodeType::Operator; };
	template <> struct TPGNodeFunctionTypeTraits<&Not> { static constexpr EPGNodeType NodeType = EPGNodeType::Operator; };
}
