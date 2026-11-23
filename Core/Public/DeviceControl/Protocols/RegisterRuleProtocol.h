#pragma once

#include "Core/Public/DeviceControl/DeviceControlTypes.h"
#include "Core/Public/Policy/RuleTypes.h"

namespace Argus::DeviceControl
{
	/**
	 * Agent-to-Kernel request for one standalone rule evaluated against a single event.
	 *
	 * [Header]
	 * [Rule Record] [Strings UTF-8] [Predicate records x N] [Padding]
	 * [ExpToken record 1] [Padding]
	 * [ExpToken record 2] [Padding]
	 * ...
	 *
	 * Offsets are measured in bytes from the beginning of the request buffer.
	 * UTF-8 strings have no trailing null terminator; padding bytes are zero-filled.
	 * ExpToken records form a postfix expression. Predicate tokens consume conditions
	 * in array order and push Boolean results. And/Or consume two results; Not negates
	 * one result. A valid expression consumes every predicate and leaves one result.
	 */

	/* Maximum complete request size in bytes, including records, strings and padding. */
	inline constexpr uint32 RuleMaxRequestBytes = 64u * 1024u;
	/* Maximum number of predicate records permitted in a single rule. */
	inline constexpr uint32 RuleMaxPredicates = 4096;
	/* Maximum number of ExpToken records, counting both Predicate and Operator tokens. */
	inline constexpr uint32 RuleMaxInstructions = 8192;
	/* Sentinel used when a registration result does not identify a failed item. */
	inline constexpr uint32 RuleNoIndex = 0xFFFFFFFFu;

	/* Fixed request header describing one rule and its postfix expression token count. */
	struct FRegisterRuleHeaderRP
	{
	public:
		/* Protocol revision used to populate and validate request and response versions. */
		static constexpr uint32 HeaderVersion = 1u;

	public:
		/* Registration protocol version; must be FRegisterRuleHeaderRP::HeaderVersion. */
		uint32 Version;
		/* Fixed header size in bytes; excludes the rule, expression tokens and payloads. */
		uint32 HeaderBytes;
		/* Complete request size in bytes, including the header, all records, strings and padding. */
		uint32 TotalBytes;
		/* Number of rule records; version 1 requires exactly one per request. */
		uint32 RecordRuleCount;
		/* Total ExpToken count for this rule; includes Predicate tokens and logical Operator tokens. */
		uint32 RecordOperationCount;
	};

	/* Rule identity, single-event source constraints and the location of its predicate array. */
	struct FRegisterRuleRecordRP
	{
	public:
		/* Wire alignment in bytes; copy wire data into an aligned local object before access. */
		static constexpr uint32 RecordAlignment = 4u;

	public:
		/* Rule identifier status; a valid registration requires Captured. */
		EStringStatus RuleIdStatus;
		/* Request-relative byte offset and byte length of the nonempty UTF-8 rule ID; excludes a terminator. */
		FStringDescriptor RuleId;

		/* Event category against which every predicate in this rule is evaluated. */
		Argus::Policy::EEventType EventType;
		/* Log-source qualifier; None applies no service restriction. */
		Argus::Policy::EEventServiceType ServiceType;

		/* Request-relative byte offset of the FRulePredicateRP array. */
		uint64 PredicateOffset;
		/* Number of predicate array entries; Predicate tokens consume these entries sequentially. */
		uint64 PredicateCount;
	};

	/* Tagged postfix expression token; ExpTokenType selects the active one-byte union payload. */
	struct FRegisterRuleExpTokenRecordRP
	{
	public:
		/* Wire alignment in bytes; round sizeof(this record) up to this value and zero-fill trailing padding. */
		static constexpr uint32 RecordAlignment = 4u;

	public:
		/* Token category: Predicate selects PredicateType; Operator selects OperatorType. */
		Argus::Policy::EEventMatchExpTokenType ExpTokenType;

		/* One-byte payload; only the member selected by ExpTokenType is meaningful. */
		union 
		{
			/* Active for Predicate tokens; Bool evaluates the next predicate and pushes its Boolean result. */
			Argus::Policy::EEventMatchExpPredicate PredicateType;

			/* Active for Operator tokens; And/Or combine two Boolean results, while Not negates one. */
			Argus::Policy::EEventMatchExpOperator OperatorType;
		};
	};

	/* One comparison between a field of the current event and a single UTF-8 value. */
	struct FRulePredicateRP
	{
		/* Event field read by this comparison. */
		Argus::Policy::EEventFieldType FieldType;
		/* Comparison method applied to the selected field and the supplied value. */
		Argus::Policy::EEventMatch Match;
		/* Whether the comparison preserves or ignores character case. */
		Argus::Policy::EEventCaseSensitivity CaseSensitivity;

		/* Comparison value status; a valid registration requires Captured. */
		EStringStatus ValueStatus;
		/* Request-relative byte offset and byte length of one nonempty UTF-8 comparison value; excludes a terminator. */
		FStringDescriptor Value;
	};

	/* Fixed acknowledgement for the single rule; successful registration leaves both failure indexes unused. */
	struct FRegisterRule
	{
		/* Protocol revision; must match the request and FRegisterRuleHeaderRP::HeaderVersion. */
		uint32 Version;
		/* Complete acknowledgement size in bytes; equals sizeof(FRegisterRule). */
		uint32 TotalBytes;
		
		/* Registration result for the single rule in the request. */
		Argus::Policy::ERegisterRuleStatus Status;
		
		/* Zero-based index in the rule's predicate array, or RuleNoIndex when no predicate is identified. */
		uint32 FailedPredicateIndex;
		/* Zero-based index in the ExpToken sequence, excluding padding, or RuleNoIndex when unused. */
		uint32 FailedInstructionIndex;
	};

	static_assert(sizeof(FRegisterRuleHeaderRP) == 20);
	static_assert(sizeof(FRegisterRuleRecordRP) == 48);
	static_assert(sizeof(FRegisterRuleExpTokenRecordRP) == 2);
	static_assert(sizeof(FRulePredicateRP) == 24);
	static_assert(sizeof(Argus::Policy::EEventMatchExpOperator) == 1);
	static_assert(sizeof(FRegisterRule) == 20);
}
