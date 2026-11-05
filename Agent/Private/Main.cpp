#include <Windows.h>

#include "Core/Public/DeviceControl/DeviceControlDefines.h"
#include "Core/Public/DeviceControl/Protocols/ImageNotifyProtorol.h"
#include "Core/Public/DeviceControl/Protocols/ObjectOperationNotifyProtocol.h"
#include "Core/Public/DeviceControl/Protocols/ProcessNotifyProtocol.h"
#include "Core/Public/DeviceControl/Protocols/RegistryNotifyProtocol.h"
#include "Core/Public/DeviceControl/Protocols/RegisterAgentProtocol.h"
#include "Core/Public/DeviceControl/Protocols/RegisterProcessProtocol.h"
#include "Core/Public/DeviceControl/Protocols/ThreadNotifyProtocol.h"

#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{
	constexpr DWORD PollIntervalMs = 1000;
	constexpr int32 SlotsPerRequest = 64;
	constexpr int32 MaxBatchesPerPoll = 16;
	constexpr size_t ProcessInitialBufferBytes = 64u * 1024u;
	// The callback supplies at most two UNICODE_STRING payloads per record.
	constexpr size_t ProcessMaxBufferBytes = 1024u * 1024u;
	constexpr size_t RegistryInitialBufferBytes = 64u * 1024u;
	// A record holds at most two UNICODE_STRING names, each with a 16-bit byte length.
	constexpr size_t RegistryMaxBufferBytes = 256u * 1024u;
	constexpr uint32 AgentMajorVersion = 0;
	constexpr uint32 AgentMinorVersion = 1;
	constexpr uint32 AgentPatchVersion = 1;
	constexpr WCHAR ProtectedExecutableName[] = L"maruexe.local.exe";

	struct FTrackedProtectedProcess
	{
		int64 CreatedAt;
		uint32 ParentProcessId;
	};

	using FProtectedProcessMap = std::unordered_map<uint32, FTrackedProtectedProcess>;

	void Log(FILE* Stream, int64 TimeTicks, const char* Level, const char* Event,
		const char* Format, ...);

	enum class ERegisterProtectedResult : uint8
	{
		Failed,
		Registered,
		AlreadyRegistered,
	};

	const char* GetKnownProcessRole(uint32 ProcessId, const FProtectedProcessMap& ProtectedProcesses) noexcept
	{
		if (ProcessId == GetCurrentProcessId())
		{
			return "agent";
		}
		return ProtectedProcesses.find(ProcessId) != ProtectedProcesses.end() ? "protected" : "unknown";
	}

	bool MatchesProtectedExecutable(const uint8* Buffer,
		const Argus::DeviceControl::FProcessNotifyRecord& Record)
	{
		if (Record.NotifyType != Argus::Notify::EProcessNotifyType::Create ||
			static_cast<int32>(Record.CreationStatus) < 0)
		{
			return false;
		}
		const size_t NameBytes = static_cast<size_t>(Record.ImageFileNameBytes);
		if (NameBytes == 0)
		{
			return false;
		}
		constexpr int TargetNameChars = static_cast<int>(sizeof(ProtectedExecutableName) / sizeof(WCHAR) - 1);
		if (NameBytes / sizeof(WCHAR) != TargetNameChars)
		{
			return false;
		}
		WCHAR ImageName[TargetNameChars]{};
		std::memcpy(ImageName, Buffer + static_cast<size_t>(Record.ImageFileNameOffset), NameBytes);
		return CompareStringOrdinal(ImageName, TargetNameChars,
			ProtectedExecutableName, TargetNameChars, TRUE) == CSTR_EQUAL;
	}

	ERegisterProtectedResult RegisterProtectedProcess(HANDLE Device, uint32 ProcessId, int64 CreatedAt)
	{
		Argus::DeviceControl::FInRegisterProcess Request{ CreatedAt, ProcessId };
		Argus::DeviceControl::FOutRegisterProcess Response{};
		DWORD BytesReturned = 0;
		if (!DeviceIoControl(Device, Argus::DeviceControl::IoctlCodeRegisterProcess,
			&Request, static_cast<DWORD>(sizeof(Request)),
			&Response, static_cast<DWORD>(sizeof(Response)),
			&BytesReturned, nullptr))
		{
			Log(stderr, 0, "ERROR", "process.register_failed",
				"role=protected pid=%u reason=ioctl_failed win32=%lu", ProcessId, GetLastError());
			return ERegisterProtectedResult::Failed;
		}
		if (BytesReturned != sizeof(Response))
		{
			Log(stderr, 0, "ERROR", "process.register_failed",
				"role=protected pid=%u reason=invalid_byte_count bytes=%lu", ProcessId, BytesReturned);
			return ERegisterProtectedResult::Failed;
		}

		switch (Response.Status)
		{
		case Argus::DeviceControl::ERegisterProcessStatus::Success:
			return ERegisterProtectedResult::Registered;
		case Argus::DeviceControl::ERegisterProcessStatus::AlreadyRegistered:
			return ERegisterProtectedResult::AlreadyRegistered;
		case Argus::DeviceControl::ERegisterProcessStatus::ProcessNotFound:
			Log(stderr, 0, "ERROR", "process.register_failed",
				"role=protected pid=%u reason=process_not_found_or_changed", ProcessId);
			return ERegisterProtectedResult::Failed;
		case Argus::DeviceControl::ERegisterProcessStatus::AccessDenied:
			Log(stderr, 0, "ERROR", "process.register_failed",
				"role=protected pid=%u reason=access_denied", ProcessId);
			return ERegisterProtectedResult::Failed;
		case Argus::DeviceControl::ERegisterProcessStatus::AllocationFailed:
			Log(stderr, 0, "ERROR", "process.register_failed",
				"role=protected pid=%u reason=allocation_failed", ProcessId);
			return ERegisterProtectedResult::Failed;
		default:
			Log(stderr, 0, "ERROR", "process.register_failed",
				"role=protected pid=%u reason=unknown_status status=%u",
				ProcessId, static_cast<unsigned>(Response.Status));
			return ERegisterProtectedResult::Failed;
		}
	}

	std::string FormatTime(int64 TimeTicks)
	{
		if (TimeTicks <= 0)
		{
			FILETIME CurrentTime{};
			GetSystemTimePreciseAsFileTime(&CurrentTime);
			TimeTicks = static_cast<int64>((static_cast<uint64>(CurrentTime.dwHighDateTime) << 32) |
				CurrentTime.dwLowDateTime);
		}

		const uint64 Ticks = static_cast<uint64>(TimeTicks);
		FILETIME FileTime{};
		FileTime.dwLowDateTime = static_cast<DWORD>(Ticks);
		FileTime.dwHighDateTime = static_cast<DWORD>(Ticks >> 32);

		SYSTEMTIME Utc{};
		if (!FileTimeToSystemTime(&FileTime, &Utc))
		{
			return "<invalid-time>";
		}

		char Buffer[48]{};
		std::snprintf(Buffer, sizeof(Buffer), "%04u-%02u-%02uT%02u:%02u:%02u.%03lluZ",
			static_cast<unsigned>(Utc.wYear),
			static_cast<unsigned>(Utc.wMonth),
			static_cast<unsigned>(Utc.wDay),
			static_cast<unsigned>(Utc.wHour),
			static_cast<unsigned>(Utc.wMinute),
			static_cast<unsigned>(Utc.wSecond),
			static_cast<unsigned long long>((Ticks % 10000000ULL) / 10000ULL));
		return Buffer;
	}

	void Log(FILE* Stream, int64 TimeTicks, const char* Level, const char* Event,
		const char* Format, ...)
	{
		const std::string Time = FormatTime(TimeTicks);
		std::fprintf(Stream, "[Argus] %s %s agent %s ", Time.c_str(), Level, Event);
		va_list Args;
		va_start(Args, Format);
		std::vfprintf(Stream, Format, Args);
		va_end(Args);
		std::fputc('\n', Stream);
	}

	std::string EscapeForLog(const std::string& Text)
	{
		constexpr char Hex[] = "0123456789ABCDEF";
		std::string Result;
		Result.reserve(Text.size());
		for (unsigned char Byte : Text)
		{
			switch (Byte)
			{
			case '\\': Result += "\\\\"; break;
			case '"': Result += "\\\""; break;
			case '\n': Result += "\\n"; break;
			case '\r': Result += "\\r"; break;
			case '\t': Result += "\\t"; break;
			default:
				if (Byte < 0x20 || Byte == 0x7F)
				{
					Result += "\\x";
					Result += Hex[Byte >> 4];
					Result += Hex[Byte & 0x0F];
				}
				else
				{
					Result += static_cast<char>(Byte);
				}
				break;
			}
		}
		return Result;
	}

	const char* RegistryOperationName(Argus::Notify::ERegistryOperationType Type) noexcept
	{
		using TypeEnum = Argus::Notify::ERegistryOperationType;
		switch (Type)
		{
		case TypeEnum::CreateKey: return "create_key";
		case TypeEnum::OpenKey: return "open_key";
		case TypeEnum::DeleteKey: return "delete_key";
		case TypeEnum::RenameKey: return "rename_key";
		case TypeEnum::SetValue: return "set_value";
		case TypeEnum::DeleteValue: return "delete_value";
		case TypeEnum::QueryKey: return "query_key";
		case TypeEnum::QueryValue: return "query_value";
		case TypeEnum::QueryMultipleValues: return "query_multiple_values";
		case TypeEnum::EnumerateKey: return "enumerate_key";
		case TypeEnum::EnumerateValue: return "enumerate_value";
		case TypeEnum::SetKeyInformation: return "set_key_information";
		case TypeEnum::QueryKeySecurity: return "query_key_security";
		case TypeEnum::SetKeySecurity: return "set_key_security";
		case TypeEnum::FlushKey: return "flush_key";
		case TypeEnum::LoadKey: return "load_key";
		case TypeEnum::UnloadKey: return "unload_key";
		case TypeEnum::RestoreKey: return "restore_key";
		case TypeEnum::SaveKey: return "save_key";
		case TypeEnum::ReplaceKey: return "replace_key";
		case TypeEnum::CloseKey: return "close_key";
		case TypeEnum::QueryKeyName: return "query_key_name";
		case TypeEnum::SaveMergedKey: return "save_merged_key";
		default: return "unknown";
		}
	}

	const char* RegistryPhaseName(Argus::Notify::ERegistryNotifyPhase Phase) noexcept
	{
		switch (Phase)
		{
		case Argus::Notify::ERegistryNotifyPhase::Pre: return "pre";
		case Argus::Notify::ERegistryNotifyPhase::Post: return "post";
		default: return "unknown";
		}
	}

	const char* RegistryDecisionName(Argus::Notify::ERegistryDecision Decision) noexcept
	{
		switch (Decision)
		{
		case Argus::Notify::ERegistryDecision::Allowed: return "allowed";
		case Argus::Notify::ERegistryDecision::Blocked: return "blocked";
		default: return "unknown";
		}
	}

	const char* RegistryNameStatusName(Argus::Notify::ERegistryNameStatus Status) noexcept
	{
		switch (Status)
		{
		case Argus::Notify::ERegistryNameStatus::Captured: return "captured";
		case Argus::Notify::ERegistryNameStatus::Unavailable: return "unavailable";
		case Argus::Notify::ERegistryNameStatus::AllocationFailed: return "allocation_failed";
		case Argus::Notify::ERegistryNameStatus::NotApplicable: return "not_applicable";
		default: return "unknown";
		}
	}

	std::string RegistryRequesterRoles(Argus::Notify::ERegistryRequesterRole Roles)
	{
		const uint8 Bits = static_cast<uint8>(Roles);
		std::string Result;
		const auto Append = [&Result](const char* Name)
		{
			if (!Result.empty())
			{
				Result += '|';
			}
			Result += Name;
		};
		if ((Bits & static_cast<uint8>(Argus::Notify::ERegistryRequesterRole::Agent)) != 0) Append("agent");
		if ((Bits & static_cast<uint8>(Argus::Notify::ERegistryRequesterRole::Protected)) != 0) Append("protected");
		if ((Bits & static_cast<uint8>(Argus::Notify::ERegistryRequesterRole::Isolated)) != 0) Append("isolated");
		return Result.empty() ? "none" : Result;
	}

	std::string FormatRegistryName(const uint8* Buffer, uint64 Offset, uint64 ByteCount,
		Argus::Notify::ERegistryNameStatus Status)
	{
		if (Status != Argus::Notify::ERegistryNameStatus::Captured)
		{
			return std::string("<") + RegistryNameStatusName(Status) + ">";
		}
		if (ByteCount == 0)
		{
			return {};
		}

		const size_t CharCount = static_cast<size_t>(ByteCount / sizeof(WCHAR));
		std::wstring Wide(CharCount, L'\0');
		std::memcpy(Wide.data(), Buffer + static_cast<size_t>(Offset), static_cast<size_t>(ByteCount));
		const int WideLength = static_cast<int>(CharCount);
		const int Utf8Bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			Wide.data(), WideLength, nullptr, 0, nullptr, nullptr);
		if (Utf8Bytes <= 0)
		{
			return "<invalid-utf16>";
		}
		std::string Utf8(static_cast<size_t>(Utf8Bytes), '\0');
		if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			Wide.data(), WideLength, Utf8.data(), Utf8Bytes, nullptr, nullptr) != Utf8Bytes)
		{
			return "<invalid-utf16>";
		}
		return EscapeForLog(Utf8);
	}

	bool ParseRegistryBatch(const uint8* Buffer, size_t BytesReturned,
		Argus::DeviceControl::FRegistryNotifyHeader& Header,
		std::vector<Argus::DeviceControl::FRegistryNotifyRecord>& Records)
	{
		using FHeader = Argus::DeviceControl::FRegistryNotifyHeader;
		using FRecord = Argus::DeviceControl::FRegistryNotifyRecord;
		if (BytesReturned < sizeof(FHeader))
		{
			return false;
		}
		std::memcpy(&Header, Buffer, sizeof(Header));
		if (Header.Version != FHeader::HeaderVerion ||
			Header.HeaderBytes != sizeof(FHeader) ||
			Header.TotalBytes != BytesReturned ||
			Header.Reserved != 0)
		{
			return false;
		}

		uint64 Offset = Header.HeaderBytes;
		while (Offset < Header.TotalBytes)
		{
			if (Header.TotalBytes - Offset < sizeof(FRecord))
			{
				return false;
			}
			FRecord Record{};
			std::memcpy(&Record, Buffer + static_cast<size_t>(Offset), sizeof(Record));
			if (Record.RecordHeaderBytes != sizeof(FRecord) ||
				Record.RecordBytes < Record.RecordHeaderBytes ||
				Record.RecordBytes > Header.TotalBytes - Offset ||
				Record.RecordBytes % FRecord::RecordAlignment != 0 ||
				Record.PaddingBytes >= FRecord::RecordAlignment ||
				Record.KeyNameBytes % sizeof(WCHAR) != 0 ||
				Record.ValueNameBytes % sizeof(WCHAR) != 0)
			{
				return false;
			}

			const uint64 PayloadBytes = Record.RecordBytes - Record.RecordHeaderBytes;
			if (Record.KeyNameBytes > PayloadBytes ||
				Record.ValueNameBytes > PayloadBytes - Record.KeyNameBytes ||
				Record.PaddingBytes != PayloadBytes - Record.KeyNameBytes - Record.ValueNameBytes)
			{
				return false;
			}
			const uint64 KeyOffset = Offset + Record.RecordHeaderBytes;
			const uint64 ValueOffset = KeyOffset + Record.KeyNameBytes;
			if (Record.KeyNameOffset != KeyOffset ||
				Record.ValueNameOffset != ValueOffset ||
				(Record.KeyNameStatus != Argus::Notify::ERegistryNameStatus::Captured && Record.KeyNameBytes != 0) ||
				(Record.ValueNameStatus != Argus::Notify::ERegistryNameStatus::Captured && Record.ValueNameBytes != 0))
			{
				return false;
			}
			const uint64 PaddingOffset = ValueOffset + Record.ValueNameBytes;
			for (uint64 Index = 0; Index < Record.PaddingBytes; ++Index)
			{
				if (Buffer[static_cast<size_t>(PaddingOffset + Index)] != 0)
				{
					return false;
				}
			}
			Records.push_back(Record);
			Offset += Record.RecordBytes;
		}
		return Offset == Header.TotalBytes;
	}

	bool ParseObjectOperationBatch(const uint8* Buffer, size_t BytesReturned,
		const Argus::DeviceControl::FObjectOperationRP& Request,
		Argus::DeviceControl::FObjectOperationNotifyHeader& Header,
		std::vector<Argus::DeviceControl::FObjectOperationNotifyRecord>& Records)
	{
		using FHeader = Argus::DeviceControl::FObjectOperationNotifyHeader;
		using FRecord = Argus::DeviceControl::FObjectOperationNotifyRecord;
		if (BytesReturned < sizeof(FHeader))
		{
			return false;
		}
		std::memcpy(&Header, Buffer, sizeof(Header));
		if (Header.Version != FHeader::HeaderVersion ||
			Header.HeaderBytes != sizeof(FHeader) ||
			Header.TotalBytes != BytesReturned ||
			Header.Reserved != 0)
		{
			return false;
		}

		const size_t PayloadBytes = BytesReturned - sizeof(FHeader);
		if (PayloadBytes % sizeof(FRecord) != 0 ||
			Header.TotalCount != PayloadBytes / sizeof(FRecord) ||
			Header.TotalCount > Request.CurRecordCount ||
			Header.NextRecordCount > Request.NextRecordCount ||
			Header.NextRecordBytes != static_cast<uint64>(Header.NextRecordCount) * sizeof(FRecord))
		{
			return false;
		}

		const size_t Count = PayloadBytes / sizeof(FRecord);
		Records.reserve(Count);
		for (size_t Index = 0; Index < Count; ++Index)
		{
			FRecord Record{};
			std::memcpy(&Record, Buffer + sizeof(FHeader) + Index * sizeof(FRecord), sizeof(Record));
			Records.push_back(Record);
		}
		return true;
	}

	const char* ObjectOperationName(Argus::Notify::EObjectOperationType Type) noexcept
	{
		switch (Type)
		{
		case Argus::Notify::EObjectOperationType::Create: return "create";
		case Argus::Notify::EObjectOperationType::Duplicate: return "duplicate";
		default: return "unknown";
		}
	}

	const char* ObjectTargetName(Argus::Notify::EObjectTargetType Type) noexcept
	{
		switch (Type)
		{
		case Argus::Notify::EObjectTargetType::Process: return "process";
		case Argus::Notify::EObjectTargetType::Thread: return "thread";
		default: return "unknown";
		}
	}

	void LogObjectOperationRecord(const Argus::DeviceControl::FObjectOperationNotifyRecord& Record)
	{
		const uint32 RemovedAccess = Record.DesiredAccessBeforePolicy & ~Record.DesiredAccessAfterPolicy;
		if (RemovedAccess == 0)
		{
			return;
		}

		const std::string PreTime = FormatTime(Record.PreOperationAt);
		Log(stdout, Record.PostOperationAt, "AUDIT", "object.handle_operation",
			"operation=%s target_type=%s requester_pid=%u requester_tid=%u owner_pid=%u "
			"source_pid=%u target_pid=%u target_tid=%u pre=%s "
			"original_access=0x%08X desired_before=0x%08X desired_after=0x%08X "
			"removed_access=0x%08X granted_access=0x%08X status=0x%08X",
			ObjectOperationName(Record.OperationType), ObjectTargetName(Record.TargetType),
			Record.RequesterProcessId, Record.RequesterThreadId, Record.HandleOwnerProcessId,
			Record.SourceProcessId, Record.TargetProcessId, Record.TargetThreadId, PreTime.c_str(),
			Record.OriginalDesiredAccess, Record.DesiredAccessBeforePolicy,
			Record.DesiredAccessAfterPolicy, RemovedAccess, Record.GrantedAccess, Record.ReturnStatus);
	}

	void LogRegistryRecord(const uint8* Buffer,
		const Argus::DeviceControl::FRegistryNotifyRecord& Record)
	{
		const std::string Key = FormatRegistryName(Buffer, Record.KeyNameOffset,
			Record.KeyNameBytes, Record.KeyNameStatus);
		const std::string Value = FormatRegistryName(Buffer, Record.ValueNameOffset,
			Record.ValueNameBytes, Record.ValueNameStatus);
		const std::string Roles = RegistryRequesterRoles(Record.RequesterRoles);
		const bool Blocked = Record.Decision == Argus::Notify::ERegistryDecision::Blocked;
		Log(stdout, Record.OccurredAt, Blocked ? "BLOCK" : "AUDIT",
			Blocked ? "registry.block" : "registry.operation",
			"operation=%s phase=%s requester_roles=%s requester_mask=0x%02X pid=%u tid=%u operation_id=%llu "
			"decision=%s status=0x%08X key_status=%s key=\"%s\" value_status=%s value=\"%s\" isolation_flags=0x%08X",
			RegistryOperationName(Record.OperationType), RegistryPhaseName(Record.Phase), Roles.c_str(),
			static_cast<unsigned>(Record.RequesterRoles), Record.ProcessId, Record.ThreadId,
			static_cast<unsigned long long>(Record.OperationId),
			RegistryDecisionName(Record.Decision), static_cast<uint32>(Record.Status),
			RegistryNameStatusName(Record.KeyNameStatus), Key.c_str(),
			RegistryNameStatusName(Record.ValueNameStatus), Value.c_str(), Record.IsolationFlags);
	}

	bool ParseProcessBatch(const uint8* Buffer, size_t BytesReturned,
		Argus::DeviceControl::FProcessNotifyHeader& Header,
		std::vector<Argus::DeviceControl::FProcessNotifyRecord>& Records)
	{
		using FHeader = Argus::DeviceControl::FProcessNotifyHeader;
		using FRecord = Argus::DeviceControl::FProcessNotifyRecord;
		if (BytesReturned < sizeof(FHeader))
		{
			return false;
		}
		std::memcpy(&Header, Buffer, sizeof(Header));
		if (Header.Version != FHeader::HeaderVersion ||
			Header.HeaderBytes != sizeof(FHeader) ||
			Header.TotalBytes != BytesReturned || Header.Reserved != 0 ||
			(Header.NextRecordCount == 0) != (Header.NextRecordBytes == 0) ||
			Header.NextRecordBytes < static_cast<uint64>(Header.NextRecordCount) * sizeof(FRecord))
		{
			return false;
		}

		size_t Offset = sizeof(FHeader);
		while (Offset < BytesReturned)
		{
			const size_t Remaining = BytesReturned - Offset;
			if (Remaining < sizeof(FRecord))
			{
				return false;
			}
			FRecord Record{};
			std::memcpy(&Record, Buffer + Offset, sizeof(Record));
			if (Record.RecordHeaderBytes != sizeof(FRecord) ||
				Record.CommandLineBytes % sizeof(WCHAR) != 0 ||
				Record.ImageFileNameBytes % sizeof(WCHAR) != 0 ||
				Record.PaddingBytes >= FRecord::RecordAlignment)
			{
				return false;
			}
			const size_t PayloadBytes = Remaining - sizeof(FRecord);
			if (Record.CommandLineBytes > PayloadBytes ||
				Record.ImageFileNameBytes > PayloadBytes - static_cast<size_t>(Record.CommandLineBytes) ||
				Record.PaddingBytes > PayloadBytes - static_cast<size_t>(Record.CommandLineBytes) -
					static_cast<size_t>(Record.ImageFileNameBytes))
			{
				return false;
			}
			const size_t CommandOffset = Offset + sizeof(FRecord);
			const size_t ImageOffset = CommandOffset + static_cast<size_t>(Record.CommandLineBytes);
			const size_t PaddingOffset = ImageOffset + static_cast<size_t>(Record.ImageFileNameBytes);
			const size_t RecordBytes = PaddingOffset + static_cast<size_t>(Record.PaddingBytes) - Offset;
			if (Record.CommandLineOffset != CommandOffset ||
				Record.ImageFileNameOffset != ImageOffset ||
				RecordBytes % FRecord::RecordAlignment != 0)
			{
				return false;
			}
			for (size_t Index = 0; Index < Record.PaddingBytes; ++Index)
			{
				if (Buffer[PaddingOffset + Index] != 0)
				{
					return false;
				}
			}
			Records.push_back(Record);
			Offset += RecordBytes;
		}
		return Records.size() == Header.TotalCount;
	}

	std::string FormatProcessText(const uint8* Buffer, uint64 Offset, uint64 ByteCount,
		const char* EmptyText)
	{
		if (ByteCount == 0)
		{
			return EmptyText;
		}
		const size_t CharCount = static_cast<size_t>(ByteCount / sizeof(WCHAR));
		std::wstring Wide(CharCount, L'\0');
		std::memcpy(Wide.data(), Buffer + static_cast<size_t>(Offset), static_cast<size_t>(ByteCount));
		const int CharacterCount = static_cast<int>(CharCount);
		const int Utf8Bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			Wide.data(), CharacterCount, nullptr, 0, nullptr, nullptr);
		if (Utf8Bytes <= 0)
		{
			return "<invalid-utf16>";
		}
		std::string Utf8(static_cast<size_t>(Utf8Bytes), '\0');
		if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, Wide.data(), CharacterCount,
			Utf8.data(), Utf8Bytes, nullptr, nullptr) != Utf8Bytes)
		{
			return "<invalid-utf16>";
		}
		return EscapeForLog(Utf8);
	}

	const char* ProcessPolicyRoleName(Argus::Policy::EPolicyRole Role) noexcept
	{
		switch (Role)
		{
		case Argus::Policy::EPolicyRole::None: return "none";
		case Argus::Policy::EPolicyRole::AgentProcess: return "agent";
		case Argus::Policy::EPolicyRole::IsolatedProcess: return "isolated";
		case Argus::Policy::EPolicyRole::ProtectedProcess: return "protected";
		default: return "unknown";
		}
	}

	const char* ProcessPolicyStatusName(Argus::Policy::EPolicyStatus Status) noexcept
	{
		switch (Status)
		{
		case Argus::Policy::EPolicyStatus::AccessNone: return "none";
		case Argus::Policy::EPolicyStatus::AccessPass: return "pass";
		case Argus::Policy::EPolicyStatus::AccessAllowed: return "allowed";
		case Argus::Policy::EPolicyStatus::AccessBlocked: return "blocked";
		case Argus::Policy::EPolicyStatus::AccessReduced: return "reduced";
		default: return "unknown";
		}
	}

	void LogProcessNotify(const uint8* Buffer,
		const Argus::DeviceControl::FProcessNotifyRecord& Record,
		FProtectedProcessMap& ProtectedProcesses)
	{
		switch (Record.NotifyType)
		{
		case Argus::Notify::EProcessNotifyType::Create:
		case Argus::Notify::EProcessNotifyType::Both:
		{
			// A missed exit notification must not make a reused PID look registered.
			ProtectedProcesses.erase(Record.ProcessId);
			const bool Blocked = Record.PolicyStatus == Argus::Policy::EPolicyStatus::AccessBlocked;
			const bool Failed = static_cast<int32>(Record.CreationStatus) < 0;
			const bool Both = Record.NotifyType == Argus::Notify::EProcessNotifyType::Both;
			const std::string Image = FormatProcessText(Buffer, Record.ImageFileNameOffset,
				Record.ImageFileNameBytes, "<unavailable>");
			const std::string CommandLine = FormatProcessText(Buffer, Record.CommandLineOffset,
				Record.CommandLineBytes, "");
			const std::string ExitedAt = Both ? FormatTime(Record.ExitedAt) : "<unavailable>";
			Log(stdout, Record.CreatedAt, Blocked ? "BLOCK" : "AUDIT",
				Blocked ? "process.create_denied" : Failed ? "process.create_failed" :
				Both ? "process.lifecycle" : "process.create",
				"pid=%u parent=%u creator_pid=%u creator_tid=%u creator_role=%s "
				"policy_role=%s policy_status=%s status=0x%08X image=\"%s\" command=\"%s\" "
				"has_exit=%u exited=%s exit_code=0x%08X",
				Record.ProcessId, Record.ParentProcessId,
				Record.CreatingProcessId, Record.CreatingThreadId,
				GetKnownProcessRole(Record.CreatingProcessId, ProtectedProcesses),
				ProcessPolicyRoleName(Record.PolicyRole), ProcessPolicyStatusName(Record.PolicyStatus),
				Record.CreationStatus, Image.c_str(), CommandLine.c_str(),
				Both ? 1u : 0u, ExitedAt.c_str(), Record.ExitCode);
			break;
		}
		case Argus::Notify::EProcessNotifyType::Exit:
		{
			Log(stdout, Record.ExitedAt, "AUDIT", "process.exit",
				"pid=%u exit_code=0x%08X", Record.ProcessId, Record.ExitCode);
			const auto It = ProtectedProcesses.find(Record.ProcessId);
			if (It != ProtectedProcesses.end())
			{
				if (Record.CreatedAt != 0 && Record.CreatedAt != It->second.CreatedAt)
				{
					break;
				}
				const FTrackedProtectedProcess Tracked = It->second;
				ProtectedProcesses.erase(It);
				const std::string CreatedAt = FormatTime(Tracked.CreatedAt);
				Log(stdout, Record.ExitedAt, "AUDIT", "process.unregister",
					"role=protected pid=%u created=%s parent=%u inferred_from=process_exit exit_code=0x%08X",
					Record.ProcessId, CreatedAt.c_str(), Tracked.ParentProcessId, Record.ExitCode);
			}
			break;
		}
		default:
			Log(stderr, 0, "ERROR", "process.unknown_notification", "pid=%u type=%u",
				Record.ProcessId, static_cast<unsigned>(Record.NotifyType));
			break;
		}
	}
}

int main()
{
	SetConsoleOutputCP(CP_UTF8);
	std::setvbuf(stdout, nullptr, _IONBF, 0);

	HANDLE Device = CreateFileW(LR"(\\.\Argus)", GENERIC_READ, 0, nullptr,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (Device == INVALID_HANDLE_VALUE)
	{
		Log(stderr, 0, "ERROR", "device.open_failed", "win32=%lu", GetLastError());
		return 1;
	}

	Argus::DeviceControl::FRegisterAgent Registration{
		AgentMajorVersion, AgentMinorVersion, AgentPatchVersion
	};
	DWORD RegistrationBytesReturned = 0;
	if (!DeviceIoControl(Device, Argus::DeviceControl::IoctlCodeRegisterAgent,
		&Registration, static_cast<DWORD>(sizeof(Registration)),
		nullptr, 0,
		&RegistrationBytesReturned, nullptr))
	{
		Log(stderr, 0, "ERROR", "process.register_failed",
			"role=agent pid=%lu reason=ioctl_failed win32=%lu", GetCurrentProcessId(), GetLastError());
		CloseHandle(Device);
		return 1;
	}

	Log(stdout, 0, "INFO", "process.register", "role=agent pid=%lu", GetCurrentProcessId());
	static_assert(ProcessMaxBufferBytes <= MAXDWORD, "Process IOCTL output size must fit in DWORD");
	std::vector<uint8> ProcessBuffer(ProcessInitialBufferBytes);
	Argus::DeviceControl::FThreadNotifyRecord ThreadRecords[SlotsPerRequest];
	static_assert(sizeof(ThreadRecords) <= MAXDWORD, "IOCTL output size must fit in DWORD");
	Argus::DeviceControl::FImageNotifySlot ImageSlots[SlotsPerRequest];
	static_assert(sizeof(ImageSlots) <= MAXDWORD, "IOCTL output size must fit in DWORD");
	static_assert(RegistryMaxBufferBytes <= MAXDWORD, "Registry IOCTL output size must fit in DWORD");
	std::vector<uint8> RegistryBuffer(RegistryInitialBufferBytes);
	constexpr size_t ObjectBufferBytes = sizeof(Argus::DeviceControl::FObjectOperationNotifyHeader) +
		static_cast<size_t>(SlotsPerRequest) * sizeof(Argus::DeviceControl::FObjectOperationNotifyRecord);
	static_assert(ObjectBufferBytes <= MAXDWORD, "Object operation IOCTL output size must fit in DWORD");
	std::vector<uint8> ObjectBuffer(ObjectBufferBytes);
	FProtectedProcessMap ProtectedProcesses;
	uint64 LastProcessDropCount = 0;
	bool bHasProcessDropCount = false;
	uint64 LastRegistryDropCount = 0;
	bool bHasRegistryDropCount = false;
	uint64 LastObjectDropCount = 0;
	bool bHasObjectDropCount = false;

	for (;;)
	{
		for (int32 Batch = 0; Batch < MaxBatchesPerPoll; ++Batch)
		{
			Argus::DeviceControl::FProcessNotifyRP Request{
				Argus::DeviceControl::FProcessNotifyHeader::HeaderVersion,
				static_cast<uint32>(SlotsPerRequest), static_cast<uint32>(SlotsPerRequest)
			};
			DWORD BytesReturned = 0;
			if (!DeviceIoControl(Device, Argus::DeviceControl::IoctlCodeGetProcessNotify,
				&Request, static_cast<DWORD>(sizeof(Request)),
				ProcessBuffer.data(), static_cast<DWORD>(ProcessBuffer.size()),
				&BytesReturned, nullptr))
			{
				const DWORD Error = GetLastError();
				if (Error == ERROR_NO_SYSTEM_RESOURCES)
				{
					Log(stderr, 0, "WARN", "ioctl.get_process_notify_retry", "reason=resources_unavailable");
					break;
				}
				Log(stderr, 0, "ERROR", "ioctl.get_process_notify_failed", "win32=%lu", Error);
				CloseHandle(Device);
				return 1;
			}
			if (BytesReturned > ProcessBuffer.size())
			{
				Log(stderr, 0, "ERROR", "ioctl.get_process_notify_failed",
					"reason=invalid_byte_count bytes=%lu", BytesReturned);
				CloseHandle(Device);
				return 1;
			}

			Argus::DeviceControl::FProcessNotifyHeader Header{};
			std::vector<Argus::DeviceControl::FProcessNotifyRecord> Records;
			if (!ParseProcessBatch(ProcessBuffer.data(), BytesReturned, Header, Records))
			{
				Log(stderr, 0, "ERROR", "ioctl.get_process_notify_failed",
					"reason=invalid_payload bytes=%lu", BytesReturned);
				CloseHandle(Device);
				return 1;
			}
			for (const auto& Record : Records)
			{
				LogProcessNotify(ProcessBuffer.data(), Record, ProtectedProcesses);
				if (MatchesProtectedExecutable(ProcessBuffer.data(), Record))
				{
					const std::string Image = FormatProcessText(ProcessBuffer.data(),
						Record.ImageFileNameOffset, Record.ImageFileNameBytes, "<unavailable>");
					Log(stdout, Record.CreatedAt, "AUDIT", "process.detect",
						"role=protected pid=%u parent=%u image=\"%s\"",
						Record.ProcessId, Record.ParentProcessId, Image.c_str());
					const ERegisterProtectedResult Result = RegisterProtectedProcess(Device, Record.ProcessId, Record.CreatedAt);
					if (Result != ERegisterProtectedResult::Failed)
					{
						ProtectedProcesses.insert_or_assign(Record.ProcessId,
							FTrackedProtectedProcess{ Record.CreatedAt, Record.ParentProcessId });
						if (Result == ERegisterProtectedResult::Registered)
						{
							Log(stdout, 0, "INFO", "process.register",
								"role=protected pid=%u parent=%u image=\"%s\"",
								Record.ProcessId, Record.ParentProcessId, Image.c_str());
						}
					}
				}
			}
			if (!bHasProcessDropCount)
			{
				if (Header.DropCount != 0)
				{
					Log(stderr, 0, "WARN", "process.drop", "total=%llu",
						static_cast<unsigned long long>(Header.DropCount));
				}
				bHasProcessDropCount = true;
			}
			else if (Header.DropCount > LastProcessDropCount)
			{
				Log(stderr, 0, "WARN", "process.drop", "delta=%llu total=%llu",
					static_cast<unsigned long long>(Header.DropCount - LastProcessDropCount),
					static_cast<unsigned long long>(Header.DropCount));
			}
			else if (Header.DropCount < LastProcessDropCount)
			{
				Log(stderr, 0, "WARN", "process.drop_counter_reset",
					"previous_total=%llu current_total=%llu",
					static_cast<unsigned long long>(LastProcessDropCount),
					static_cast<unsigned long long>(Header.DropCount));
			}
			LastProcessDropCount = Header.DropCount;
			if (Header.NextRecordCount == 0)
			{
				break;
			}
			if (Records.empty())
			{
				if (ProcessBuffer.size() == ProcessMaxBufferBytes)
				{
					Log(stderr, 0, "ERROR", "ioctl.get_process_notify_failed",
						"reason=record_exceeds_buffer buffer_bytes=%zu pending=%u",
						ProcessBuffer.size(), Header.NextRecordCount);
					CloseHandle(Device);
					return 1;
				}
				const size_t DoubledSize = ProcessBuffer.size() * 2;
				ProcessBuffer.resize(DoubledSize < ProcessMaxBufferBytes
					? DoubledSize : ProcessMaxBufferBytes);
			}
		}
		for (int32 Batch = 0; Batch < MaxBatchesPerPoll; ++Batch)
		{
			DWORD BytesReturned = 0;
			if (!DeviceIoControl(Device, Argus::DeviceControl::IoctlCodeGetThreadNotify,
				nullptr, 0, ThreadRecords, static_cast<DWORD>(sizeof(ThreadRecords)), &BytesReturned, nullptr))
			{
				const DWORD Error = GetLastError();
				if (Error == ERROR_NO_SYSTEM_RESOURCES)
				{
					Log(stderr, 0, "WARN", "ioctl.get_thread_notify_retry", "reason=resources_unavailable");
					break;
				}
				Log(stderr, 0, "ERROR", "ioctl.get_thread_notify_failed", "win32=%lu", Error);
				CloseHandle(Device);
				return 1;
			}
			if (BytesReturned > sizeof(ThreadRecords) ||
				BytesReturned % sizeof(Argus::DeviceControl::FThreadNotifyRecord) != 0)
			{
				Log(stderr, 0, "ERROR", "ioctl.get_thread_notify_failed",
					"reason=invalid_byte_count bytes=%lu", BytesReturned);
				CloseHandle(Device);
				return 1;
			}

			const size_t Count = BytesReturned / sizeof(Argus::DeviceControl::FThreadNotifyRecord);
			if (Count < SlotsPerRequest)
			{
				break;
			}
		}
		for (int32 Batch = 0; Batch < MaxBatchesPerPoll; ++Batch)
		{
			DWORD BytesReturned = 0;
			if (!DeviceIoControl(Device, Argus::DeviceControl::IoctlCodeGetImageNotify,
				nullptr, 0, ImageSlots, static_cast<DWORD>(sizeof(ImageSlots)), &BytesReturned, nullptr))
			{
				const DWORD Error = GetLastError();
				if (Error == ERROR_NO_SYSTEM_RESOURCES)
				{
					Log(stderr, 0, "WARN", "ioctl.get_image_notify_retry", "reason=resources_unavailable");
					break;
				}
				Log(stderr, 0, "ERROR", "ioctl.get_image_notify_failed", "win32=%lu", Error);
				CloseHandle(Device);
				return 1;
			}
			if (BytesReturned > sizeof(ImageSlots) ||
				BytesReturned % sizeof(Argus::DeviceControl::FImageNotifySlot) != 0)
			{
				Log(stderr, 0, "ERROR", "ioctl.get_image_notify_failed",
					"reason=invalid_byte_count bytes=%lu", BytesReturned);
				CloseHandle(Device);
				return 1;
			}

			const size_t Count = BytesReturned / sizeof(Argus::DeviceControl::FImageNotifySlot);
			if (Count < SlotsPerRequest)
			{
				break;
			}
		}
		for (int32 Batch = 0; Batch < MaxBatchesPerPoll; ++Batch)
		{
			DWORD BytesReturned = 0;
			if (!DeviceIoControl(Device, Argus::DeviceControl::IoctlCodeGetRegistryNotify,
				nullptr, 0, RegistryBuffer.data(), static_cast<DWORD>(RegistryBuffer.size()),
				&BytesReturned, nullptr))
			{
				const DWORD Error = GetLastError();
				if (Error == ERROR_NO_SYSTEM_RESOURCES)
				{
					Log(stderr, 0, "WARN", "ioctl.get_registry_notify_retry",
						"reason=resources_unavailable");
					break;
				}
				Log(stderr, 0, "ERROR", "ioctl.get_registry_notify_failed", "win32=%lu", Error);
				CloseHandle(Device);
				return 1;
			}
			if (BytesReturned > RegistryBuffer.size())
			{
				Log(stderr, 0, "ERROR", "ioctl.get_registry_notify_failed",
					"reason=invalid_byte_count bytes=%lu", BytesReturned);
				CloseHandle(Device);
				return 1;
			}

			Argus::DeviceControl::FRegistryNotifyHeader Header{};
			std::vector<Argus::DeviceControl::FRegistryNotifyRecord> Records;
			if (!ParseRegistryBatch(RegistryBuffer.data(), BytesReturned, Header, Records))
			{
				Log(stderr, 0, "ERROR", "ioctl.get_registry_notify_failed",
					"reason=invalid_payload bytes=%lu", BytesReturned);
				CloseHandle(Device);
				return 1;
			}
			for (const auto& Record : Records)
			{
				LogRegistryRecord(RegistryBuffer.data(), Record);
			}
			if (!bHasRegistryDropCount)
			{
				if (Header.DropCount != 0)
				{
					Log(stderr, 0, "WARN", "registry.drop",
						"total=%llu", static_cast<unsigned long long>(Header.DropCount));
				}
				bHasRegistryDropCount = true;
			}
			else if (Header.DropCount > LastRegistryDropCount)
			{
				Log(stderr, 0, "WARN", "registry.drop",
					"delta=%llu total=%llu",
					static_cast<unsigned long long>(Header.DropCount - LastRegistryDropCount),
					static_cast<unsigned long long>(Header.DropCount));
			}
			else if (Header.DropCount < LastRegistryDropCount)
			{
				Log(stderr, 0, "WARN", "registry.drop_counter_reset",
					"previous_total=%llu current_total=%llu",
					static_cast<unsigned long long>(LastRegistryDropCount),
					static_cast<unsigned long long>(Header.DropCount));
			}
			LastRegistryDropCount = Header.DropCount;
			if (Header.NextRecordCount == 0)
			{
				break;
			}
			if (Records.empty())
			{
				if (RegistryBuffer.size() == RegistryMaxBufferBytes)
				{
					Log(stderr, 0, "ERROR", "ioctl.get_registry_notify_failed",
						"reason=record_exceeds_buffer buffer_bytes=%zu pending=%u",
						RegistryBuffer.size(), Header.NextRecordCount);
					CloseHandle(Device);
					return 1;
				}
				const size_t DoubledSize = RegistryBuffer.size() * 2;
				RegistryBuffer.resize(DoubledSize < RegistryMaxBufferBytes
					? DoubledSize : RegistryMaxBufferBytes);
			}
		}
		for (int32 Batch = 0; Batch < MaxBatchesPerPoll; ++Batch)
		{
			Argus::DeviceControl::FObjectOperationRP Request{
				Argus::DeviceControl::FObjectOperationNotifyHeader::HeaderVersion,
				static_cast<uint32>(SlotsPerRequest), static_cast<uint32>(SlotsPerRequest)
			};
			DWORD BytesReturned = 0;
			if (!DeviceIoControl(Device, Argus::DeviceControl::IoctlCodeGetObjectOperationNotify,
				&Request, static_cast<DWORD>(sizeof(Request)),
				ObjectBuffer.data(), static_cast<DWORD>(ObjectBuffer.size()),
				&BytesReturned, nullptr))
			{
				const DWORD Error = GetLastError();
				if (Error == ERROR_NO_SYSTEM_RESOURCES)
				{
					Log(stderr, 0, "WARN", "ioctl.get_object_operation_notify_retry",
						"reason=resources_unavailable");
					break;
				}
				Log(stderr, 0, "ERROR", "ioctl.get_object_operation_notify_failed", "win32=%lu", Error);
				CloseHandle(Device);
				return 1;
			}
			if (BytesReturned > ObjectBuffer.size())
			{
				Log(stderr, 0, "ERROR", "ioctl.get_object_operation_notify_failed",
					"reason=invalid_byte_count bytes=%lu", BytesReturned);
				CloseHandle(Device);
				return 1;
			}

			Argus::DeviceControl::FObjectOperationNotifyHeader Header{};
			std::vector<Argus::DeviceControl::FObjectOperationNotifyRecord> Records;
			if (!ParseObjectOperationBatch(ObjectBuffer.data(), BytesReturned, Request, Header, Records))
			{
				Log(stderr, 0, "ERROR", "ioctl.get_object_operation_notify_failed",
					"reason=invalid_payload bytes=%lu", BytesReturned);
				CloseHandle(Device);
				return 1;
			}
			for (const auto& Record : Records)
			{
				LogObjectOperationRecord(Record);
			}
			if (!bHasObjectDropCount)
			{
				if (Header.DropCount != 0)
				{
					Log(stderr, 0, "WARN", "object.drop",
						"total=%llu", static_cast<unsigned long long>(Header.DropCount));
				}
				bHasObjectDropCount = true;
			}
			else if (Header.DropCount > LastObjectDropCount)
			{
				Log(stderr, 0, "WARN", "object.drop", "delta=%llu total=%llu",
					static_cast<unsigned long long>(Header.DropCount - LastObjectDropCount),
					static_cast<unsigned long long>(Header.DropCount));
			}
			else if (Header.DropCount < LastObjectDropCount)
			{
				Log(stderr, 0, "WARN", "object.drop_counter_reset",
					"previous_total=%llu current_total=%llu",
					static_cast<unsigned long long>(LastObjectDropCount),
					static_cast<unsigned long long>(Header.DropCount));
			}
			LastObjectDropCount = Header.DropCount;
			if (Header.NextRecordCount == 0)
			{
				break;
			}
		}
		Sleep(PollIntervalMs);
	}
}
