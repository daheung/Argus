# Registry callback draft

The current implementation collects audit events for requests made by an Agent,
Protected, or Isolation process. It does not block or modify registry operations.

## Flow

1. `HandleRegistryCallbackFunc` invokes `FRegistryCallback`, then
   `FRegistryNotifyQueue`, and returns the callback status. It does not decode
   the operation or classify the requester.
2. `FRegistryCallback` returns an `NTSTATUS`. Callback and Notify both use the
   shared registry operation decoder to classify the operation, phase, and
   requesting process. Roles are not mutually exclusive. The Pre policy
   handlers currently return `STATUS_SUCCESS`.
3. `FRegistryNotifyQueue` receives the callback status first, followed by the
   original callback arguments. It copies an audit record into its bounded
   queue. Like
   Process, Thread, and Image Notify, a full queue discards the oldest record
   and increments the dropped count.
4. A Pre record stores the callback decision. A Post record stores the registry
   operation's actual status. A rejected Pre request has no Post callback.

## Operation families

`CreateKey`, `SetKeyInformation`, `SetValue`, `DeleteKey`, and `DeleteValue`
have dedicated Pre policy extension points. `OpenKey` and other classes are
classified and audited, but currently have no policy handler. Rename and key
security changes remain separate operations; they are not folded into
`SetKeyInformation`.

`Argument2` must be interpreted using the exact notify class. Most Post classes
use `REG_POST_OPERATION_INFORMATION`; legacy Post Create/Open use
`REG_POST_CREATE_KEY_INFORMATION` / `REG_POST_OPEN_KEY_INFORMATION` instead.

## Name capture and remaining work

The audit queue currently copies available request names for Create/Open and
Set/Delete Value into `FRegistryNotifyInfo::KeyName` or `ValueName`. It uses the
source `UNICODE_STRING::Length`, and marks a name `Captured` only after copying
the entire string. An empty value name remains distinguishable from an absent
name by `ValueNameStatus`.

The name from Create/Open may be relative to a root key. Object-based operations
still report `KeyNameStatus::Unavailable`; resolving a full path requires a
separate key-object lookup with the callback cookie and object-validity checks.
The name-copy helper currently writes through `FString::GetCharArray()` because
`FString::InitializeWithLength` still has a forwarding error recorded in
`FKrnlString_KnownIssues.md`.

Pre and Post are separate records. `OperationId` remains zero, so they cannot
yet be paired reliably. The queue is not exposed through a registry IOCTL or
Agent reader. Add that transport before expecting Agent-side registry logs.

Before introducing blocking, define registry-specific isolation flags and a
target-key policy. Requester role alone does not establish whether a registry
path is protected. Any future policy must record its Pre decision before
returning the blocking status.

The shared Registry notify types now define both requester and target role
enums. Target classification is reserved for the future key-path policy and is
not inferred from the requester.
