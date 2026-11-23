#pragma once

#include "Core/Public/Policy/RuleTypes.h"
#include "Core/Public/DeviceControl/Protocols/RegisterRuleProtocol.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Argus::Rules
{
	using EEventType = Argus::Policy::EEventType;
	using EEventServiceType = Argus::Policy::EEventServiceType;
	using EEventFieldType = Argus::Policy::EEventFieldType;
	using EEventMatch = Argus::Policy::EEventMatch;
	using EEventCaseSensitivity = Argus::Policy::EEventCaseSensitivity;
	using EEventMatchOperation = Argus::Policy::EEventMatchExpOperator;

	struct FPredicate
	{
		EEventFieldType FieldType = EEventFieldType::Invalid;
		std::string Value;
		EEventMatch Match = EEventMatch::Equals;
		EEventCaseSensitivity CaseSensitivity = EEventCaseSensitivity::IgnoreCase;
	};

	struct FRule
	{
		std::string Id;
		std::string Title; // Agent metadata; not transmitted to Kernel.
		EEventType EventType = EEventType::Invalid;
		EEventServiceType ServiceType = EEventServiceType::None;
		std::vector<FPredicate> Predicates;
		// Predicate opcodes consume Predicates sequentially, without an index operand.
		std::vector<Argus::DeviceControl::FRegisterRuleExpTokenRecordRP> Instructions;
	};

	struct FLoadResult
	{
		std::vector<FRule> Rules;
		std::vector<std::string> Rejected;
		uint32_t Documents = 0;
	};

	// Accepts standalone single-event rules: predicates and Boolean operators
	// read one event record. Correlation, aggregation, timeframe and collection
	// actions are rejected. Multiple conditions on the same event are allowed.
	FLoadResult LoadRules(const std::string& Utf8Yaml);
	bool LoadRulesFile(const std::wstring& Path, FLoadResult& Result, std::string& Error);
	// Builds a version 1 request: header, one rule, aligned operation records, predicates, UTF-8 strings.
	// Predicate operations consume conditions sequentially; all offsets are request-relative.
	std::vector<uint8_t> SerializeRule(const FRule& Rule);
}
