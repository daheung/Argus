# Sigma rules in ArgusAgent: protocol v1

`ArgusAgent.exe --validate-sigma <path>` reads a UTF-8, multi-document Sigma YAML file and reports `documents`, `encodable` and `rejected`. Each encodable rule passes serialization and wire size/stack limits. It lists rejected documents and their reasons. This mode needs no driver. Encodable does not mean the Kernel has the required event source or field data.

`ArgusAgent.exe --sigma <path>` loads the file, registers the Agent with the driver, then submits each eligible rule through `IoctlCodeRegisterRule`. The service looks for `%ProgramData%\Argus\sigma_windows_policy.yml` on startup. If that optional file is absent, it continues without Sigma rules. The explicit `--sigma` command exits with an error if registration fails. The service logs the error and continues collecting events.

## Current matching scope

This version accepts standalone rules whose complete detection can be evaluated against one event record. One event may supply several fields, selectors and values combined with And, Or and Not. For example, `Image ends with powershell.exe AND CommandLine contains -EncodedCommand` is a single-event rule. Multiple selectors and `1 of`/`all of` do not imply multiple events; they combine conditions on that same record.

Keep all 29 event source kinds, including NetworkConnection, DnsQuery, PowerShell and WindowsEventLog, when a rule meets this requirement. Agent eligibility is independent of which sources the current Kernel collects. Enum values describe the source of a single record and never introduce event history. The future Kernel must still acknowledge source and field availability during registration.

Reject `correlation` documents, `timeframe`, legacy aggregation expressions (`| count(...)`, `| sum(...)`, etc.), temporal sequences and references to other rules' match results. Also reject collection `action` directives such as global/reset: this version requires every document to be independently interpretable and does not inherit selectors or defaults from earlier documents. Once an action is encountered, that document and all following documents in the input are rejected because they may depend on collection defaults. Independent documents preceding the directive remain eligible. A document containing both `detection` and `correlation` is rejected in full. No stateful section is ignored or weakened into a simple match.

## Rule identity and field identity

`Core/Public/Policy/RuleTypes.h` defines the shared types. Event type occurs once in the rule record. A predicate contains a field type, match operator, case flag and value. Fields do not repeat their event type in their name.

| Sigma logsource / field | Serialized representation |
| --- | --- |
| `product: windows` | Validated by Agent; implicit in protocol v1 |
| `category: process_creation` | `EventType = ProcessCreate` |
| `category: registry_set` | `EventType = RegistrySet` |
| `category: image_load` | `EventType = ImageLoad` |
| `service: security` without category | `EventType = WindowsEventLog`, `ServiceType = Security` |
| `Image` in any event | `FieldType = Image` (the associated process executable) |
| `ImageLoaded` | `FieldType = ImageLoaded` (the loaded module) |
| `CommandLine`, `TargetObject` | `FieldType = CommandLine`, `FieldType = TargetObject` |

`EEventType` has 29 nonzero IDs: 28 categories in the provided Windows policy bundle and `WindowsEventLog`. `EEventFieldType` has 212 nonzero IDs from that bundle. `Invalid = 0` is never valid on the wire. All IDs are written directly inside their enums in `RuleTypes.h`. Append new IDs before the `Count` sentinel without renumbering existing ones. `Count` is not a valid wire ID. YAML name lookup is defined in `Agent/Private/Rules.cpp`. Unknown categories or fields are rejected. Similar names from different log sources, such as `ImagePath` and `Image`, keep separate IDs until their equivalence is established. Fields are case-sensitive names; `Payload` and `payload` are separate catalog entries.

For a ProcessCreate rule with `Image|endswith: powershell.exe` and `CommandLine|contains: -EncodedCommand`, the record carries `EventType = ProcessCreate`. Its predicate table carries the two field IDs, operators and values. Its postfix program combines them with `And`. A RegistrySet rule that checks its originating executable reuses the same `Image` field ID. The Kernel resolves fields in the context of `EventType` and the optional `ServiceType` qualifier.

## Discriminators

All discriminator enums are defined once in `Core/Public/Policy/RuleTypes.h`; the IOCTL header contains structures and protocol limits.

| Field | Enum |
| --- | --- |
| EventType | EEventType |
| ServiceType | EEventServiceType |
| FieldType | EEventFieldType |
| Match | EEventMatch |
| CaseSensitivity | EEventCaseSensitivity |
| Operation | EEventMatchOperation |
| Status | ERegisterRuleStatus |

`EEventServiceType` has 37 named services from the supplied policy bundle, plus None. Agent maps YAML service names to these IDs and rejects unknown or explicitly empty service names. A missing service selects None. Services can be extended by appending fixed IDs before Count. `EEventCaseSensitivity` uses IgnoreCase = 0 and MatchCase = 1. Unknown enum values are invalid.

Offsets, byte lengths, item counts, indexes, Rule IDs and comparison values remain data. Version remains a numeric protocol revision. These values are not event classifications and cannot be replaced by a fixed enum of supported kinds.

## Request buffer

`IoctlCodeRegisterRule` uses function 0x830, `METHOD_BUFFERED` and `FILE_WRITE_ACCESS`. Each call carries one complete rule. The Agent uses a readable/writable device handle. The layout is defined in `Core/Public/DeviceControl/Protocols/RegisterRuleProtocol.h`:

| Order | Region | Size |
| --- | --- | --- |
| 1 | `FRegisterRuleHeaderRP` | 20 bytes |
| 2 | `FRegisterRuleRecordRP` | 48 bytes, starts at offset 20 |
| 3 | `FRegisterRuleOperationRecordRP[RecordOperationCount]` | 4-byte wire stride per entry, starts at offset 68 |
| 4 | Alignment padding | 0 or 4 zero bytes, to align the predicate table to 8 bytes |
| 5 | `FRulePredicateRP[PredicateCount]` | 24 bytes per entry |
| 6 | Rule ID, predicate values | Variable UTF-8 bytes |

`FStringDescriptor { uint64 Offset, uint64 Bytes }` references UTF-8 bytes without null terminators. Every offset starts at the beginning of the request and every range must be checked against TotalBytes before use. `PredicateOffset` and `PredicateCount` are uint64 fields. Counts are element counts; multiply PredicateCount by 24 for the predicate table size. Rule and operation records use their declared RecordAlignment = 4: a wire stride is sizeof(record) rounded up to 4 bytes. A rule record therefore occupies 48 bytes; an operation record contains one byte of data followed by three zero bytes. RecordAlignment is a wire convention and does not change C++ alignof. Copy the rule record at offset 20 into an aligned local object before accessing its 64-bit members; do not cast that buffer address to a structure pointer. All implicit and explicit padding is zeroed by serializing members individually.

The 20-byte `FRegisterRuleHeaderRP` contains five uint32 fields: Version = 1, HeaderBytes = 20, TotalBytes, RecordRuleCount = 1 and RecordOperationCount. Version 1 registers one rule per IOCTL because its acknowledgement describes one rule. It does not define batch registration results. RecordOperationCount counts all Predicate, And, Or and Not operation records for that rule. TotalBytes includes the header, records, alignment padding, predicates and all strings.

The 48-byte `FRegisterRuleRecordRP` contains RuleIdStatus, RuleId, EventType, ServiceType, PredicateOffset and PredicateCount. Operations start immediately after the rule record at offset 68. PredicateOffset is align_up(68 + RecordOperationCount * 4, 8). Strings start immediately after the predicate array, first the rule ID and then each predicate value in array order. The record is fixed-size and contains descriptors rather than inline variable payloads, so it needs no RecordBytes field. RuleIdStatus is a separate EStringStatus and must be Captured for a valid nonempty Rule ID. Rule titles remain Agent metadata and are not transmitted. Product is implicitly Windows; the category is represented by EventType. ServiceType = None means no service restriction; WindowsEventLog requires a non-None service.

Each 24-byte `FRulePredicateRP` contains `EEventFieldType FieldType`, `EEventMatch Match`, `EEventCaseSensitivity CaseSensitivity`, a separate `EStringStatus ValueStatus`, and `FStringDescriptor Value`. ValueStatus must be Captured and Value must contain a nonempty UTF-8 comparison value. Numeric-looking YAML scalars remain textual; this version does not provide numeric comparisons.

Each `FRegisterRuleOperationRecordRP` contains one EEventMatchOperation enum value. There is no predicate-index operand. Every Predicate opcode reads the next predicate in array order and advances a cursor. And/Or combine two stack values; Not reverses one stack value. These operators do not advance the predicate cursor. Exactly PredicateCount Predicate opcodes must appear, and evaluation must finish with exactly one Boolean result. Repeated selector references are expanded into repeated predicates by the Agent, preserving their original expression semantics. YAML conditions use parentheses first, then Not, And and Or precedence; operation records already encode that order and are processed sequentially without short-circuit skipping.

For `predicate 0 AND NOT predicate 1`, the operation array is `[Predicate, Predicate, Not, And]`. It consumes predicates 0 and 1 in order. The receiver must reject stack underflow, unknown opcodes, too many Predicate opcodes, or predicates left unused. The array has no counters, timers or references to results from earlier events or other rules.

All integers use fixed-width little-endian layout. The request contains no pointers, STL objects, field-name/category/product/service-name strings or Agent timestamps. Registration timestamps belong to Kernel-owned storage. The receiver computes its required Boolean stack depth from the operation array.

Limits remain 64 KiB per request, 4096 predicates and 8192 operations. Match operators remain Equals, Contains, StartsWith, EndsWith and Glob. Default matching uses IgnoreCase; `cased` selects MatchCase.

## Registration acknowledgement

The Agent supplies a 20-byte `FRegisterRule` output buffer containing Version = 1, TotalBytes, Status, FailedPredicateIndex and FailedInstructionIndex. It checks the returned byte count, version, TotalBytes, status and success index markers. IOCTL success without a valid acknowledgement is a registration failure.

| Status | Meaning |
| --- | --- |
| `Success` | Complete rule validated and copied into Kernel-owned storage |
| `AlreadyRegistered` | Same ID and identical serialized contents already installed |
| `InvalidRule` | Invalid request contents or condition program |
| `UnsupportedEventType` | Driver cannot provide this event source/service |
| `UnsupportedField` | Driver cannot provide a requested field for this event |
| `UnsupportedMatch` | Driver cannot evaluate the requested comparison |
| `LimitExceeded`, `AllocationFailed` | Resource constraint |
| `DuplicateIdConflict` | Same ID already exists with different contents |

The response includes a failing predicate/instruction index where applicable; unused indexes are `0xFFFFFFFF`. Successful responses use that marker for both indexes. The Agent logs a transport or protocol error and stops submitting rules after a registration failure. Earlier successful registrations are not rolled back. Registration must be atomic for each rule: a rejected rule must not leave a partial installation. Rule replacement, deletion and atomic replacement of a complete policy are not defined by this IOCTL.

For a well-formed request unsupported by the driver, return a valid acknowledgement with the appropriate unsupported status. Bad buffer lengths, unavailable devices and unauthorized callers can fail the IOCTL itself. The Kernel must verify the registered Agent's identity, version, all byte ranges/count arithmetic, table alignment, IDs, operators, value encodings and the condition stack. It must also validate event/field/operator availability. Merely recognizing a field enum value does not establish data availability; an unavailable field must reject the rule, including when used under `Not`. Convert strings to the Kernel's comparison representation at registration time and keep no pointers into the temporary IOCTL buffer. Pre/post event selection and path normalization must be established in the Kernel event adapter before acknowledging support.

This Header/Record protocol v1 replaces the earlier experimental layouts and is not wire compatible with them. Check the version, fixed header size, RecordRuleCount = 1 and operation count limits before interpreting the records or their descriptors. Existing event/field/match/operation/status numeric IDs and the IOCTL number are unchanged. The Agent does not evaluate matches. Kernel storage and matching are the intended consumer of the serialized rule. Rule matches describe detections; this contract does not define blocking actions.

**Current driver limitation:** The Kernel does not handle `IoctlCodeRegisterRule`. Consequently, the Agent cannot install rules through this IOCTL yet. The Agent reports registration failure (typically `ERROR_INVALID_FUNCTION`) and stops submitting the remaining rules. Connecting this protocol to Kernel-owned rule storage and matching requires a receiver, request validation and conversion to the internal rule representation.

The Agent accepts Windows rules with simple selector mappings, lists of selector mappings, scalar or list values, `contains`, `startswith`, `endswith`, `all`, `cased`, `*`/`?` glob values and Boolean `condition` expressions using selector names, `and`, `or`, `not`, parentheses, `1 of` and `all of`. It rejects stateful rules, unsupported modifiers and other YAML/Sigma constructs instead of silently changing their meaning. The parser is intentionally a bounded subset of YAML, not a general YAML implementation.
