#pragma once

#include "Core/Public/Misc/Types.h"

namespace Argus::Policy
{
	// Explicit wire IDs: append new entries before Count; never renumber existing entries.
	// EventType occurs once per rule. FieldType identifies a field within that event.
	// All kinds describe individual event records. A rule reads one record only;
	// event sequences, aggregation and correlation are outside this version.
	// Encoding an ID does not imply the driver can supply or match that field.
	enum class EEventType : uint8
	{
		Invalid			= 0,
		WindowsEventLog = 1,
		ProcessCreate	= 2,
		RegistrySet		= 3,
		ImageLoad		= 4,
		RegistryEvent	= 5,
		RegistryAdd		= 6,
		RegistryDelete	= 7,
		FileEvent		= 8,
		FileAccess		= 9,
		FileChange		= 10,
		FileDelete		= 11,
		FileRename		= 12,
		FileExecutableDetected	= 13,
		CreateRemoteThread		= 14,
		CreateStreamHash		= 15,
		DnsQuery		= 16,
		DriverLoad		= 17,
		NetworkConnection		= 18,
		PipeCreated		= 19,
		ProcessAccess	= 20,
		ProcessTampering		= 21,
		PsClassicProviderStart	= 22,
		PsClassicStart	= 23,
		PsModule		= 24,
		PsScript		= 25,
		RawAccessThread = 26,
		SysmonError		= 27,
		SysmonStatus	= 28,
		WmiEvent		= 29,
		Count			= 30
	};

	// logsource.service identifies the event's provider/channel qualifier.
	// None means that the rule has no service restriction. Append new wire IDs.
	enum class EEventServiceType : uint8
	{
		None = 0,
		Application = 1,
		AppLocker = 2,
		AppModelRuntime = 3,
		AppXDeploymentServer = 4,
		AppXPackagingOm = 5,
		BitsClient = 6,
		Capi2 = 7,
		CertificateServicesClientLifecycleSystem = 8,
		CodeIntegrityOperational = 9,
		DiagnosisScripted = 10,
		DnsClient = 11,
		DnsServer = 12,
		DnsServerAnalytic = 13,
		DriverFramework = 14,
		FirewallAs = 15,
		IisConfiguration = 16,
		Ldap = 17,
		LsaServer = 18,
		MicrosoftServiceBusClient = 19,
		MsExchangeManagement = 20,
		Ntlm = 21,
		OpenSsh = 22,
		PowerShellClassic = 23,
		PrintServiceAdmin = 24,
		PrintServiceOperational = 25,
		Security = 26,
		SecurityMitigations = 27,
		ShellCore = 28,
		SmbClientConnectivity = 29,
		SmbClientSecurity = 30,
		SmbServerConnectivity = 31,
		Sysmon = 32,
		System = 33,
		TaskScheduler = 34,
		TerminalServicesLocalSessionManager = 35,
		WinDefend = 36,
		Wmi = 37,
		Count = 38
	};

	enum class EEventFieldType : uint16
	{
		Invalid = 0,
		AccessList = 1,
		AccessMask = 2,
		AccountName = 3,
		Action = 4,
		AdditionalInfo = 5,
		Address = 6,
		AllowedToDelegateTo = 7,
		AppID = 8,
		AppName = 9,
		AppVersion = 10,
		Application = 11,
		ApplicationPath = 12,
		AttributeLDAPDisplayName = 13,
		AttributeValue = 14,
		Attributes = 15,
		AuditPolicyChanges = 16,
		AuditSourceName = 17,
		AuthenticationPackageName = 18,
		Binary = 19,
		CallTrace = 20,
		CallerProcessName = 21,
		CallingProcess = 22,
		Caption = 23,
		CertThumbprint = 24,
		Channel = 25,
		ClassName = 26,
		ClientAddress = 27,
		ClientProcessId = 28,
		CommandLine = 29,
		Company = 30,
		Configuration = 31,
		Contents = 32,
		ContextInfo = 33,
		CreationUtcTime = 34,
		CurrentDirectory = 35,
		Data = 36,
		Description = 37,
		DestAddress = 38,
		DestPort = 39,
		Destination = 40,
		DestinationHostname = 41,
		DestinationIp = 42,
		DestinationIsIpv6 = 43,
		DestinationPort = 44,
		Details = 45,
		Device = 46,
		DeviceDescription = 47,
		DeviceName = 48,
		DistinguishedName = 49,
		EncyptionUsed = 50,
		ErrorCode = 51,
		EventID = 52,
		EventType = 53,
		ExceptionCode = 54,
		Feature_Name = 55,
		FileName = 56,
		FileNameBuffer = 57,
		FileVersion = 58,
		FilterName = 59,
		FilterOrigin = 60,
		Flags = 61,
		GrandParentImage = 62,
		GrantedAccess = 63,
		HasFullTrust = 64,
		Hash = 65,
		Hashes = 66,
		HiveName = 67,
		Image = 68,
		ImageLoaded = 69,
		ImageName = 70,
		ImagePath = 71,
		ImpersonationLevel = 72,
		Initiated = 73,
		IntegrityLevel = 74,
		IpAddress = 75,
		IpPort = 76,
		IsatapRouter = 77,
		KeyLength = 78,
		Keywords = 79,
		LayerRTID = 80,
		Level = 81,
		LocalName = 82,
		LogonId = 83,
		LogonProcessName = 84,
		LogonType = 85,
		Message = 86,
		ModifyingApplication = 87,
		ModifyingUser = 88,
		ModuleName = 89,
		Name = 90,
		NewName = 91,
		NewTargetUserName = 92,
		NewTemplateContent = 93,
		NewUacValue = 94,
		NewValue = 95,
		ObjectClass = 96,
		ObjectDN = 97,
		ObjectName = 98,
		ObjectServer = 99,
		ObjectType = 100,
		ObjectValueName = 101,
		OldTargetUserName = 102,
		OldUacValue = 103,
		OldValue = 104,
		Operation = 105,
		Origin = 106,
		OriginalFileName = 107,
		PackageFullName = 108,
		PackagePath = 109,
		PackageSourceUri = 110,
		ParentCommandLine = 111,
		ParentImage = 112,
		ParentProcessId = 113,
		ParentUser = 114,
		Path = 115,
		Payload = 116,
		PipeName = 117,
		PossibleCause = 118,
		PreAuthType = 119,
		PreviousCreationUtcTime = 120,
		PrivilegeList = 121,
		ProcessId = 122,
		ProcessName = 123,
		ProcessNameBuffer = 124,
		ProcessPath = 125,
		Product = 126,
		Properties = 127,
		Protocol = 128,
		Provider = 129,
		ProviderContextName = 130,
		Provider_Name = 131,
		QNAME = 132,
		Query = 133,
		QueryName = 134,
		Reason = 135,
		RelativeTargetName = 136,
		RemoteAddress = 137,
		RemoteName = 138,
		RequestedPolicy = 139,
		SamAccountName = 140,
		ScriptBlockText = 141,
		SearchFilter = 142,
		ServerAddress = 143,
		ServerName = 144,
		Service = 145,
		ServiceFileName = 146,
		ServiceName = 147,
		ServicePrincipalNames = 148,
		ServiceStartType = 149,
		ServiceType = 150,
		ShareLocalPath = 151,
		ShareName = 152,
		SidHistory = 153,
		SidList = 154,
		Signature = 155,
		SignatureStatus = 156,
		Signed = 157,
		SigningUsed = 158,
		SourceAddress = 159,
		SourceCommandLine = 160,
		SourceFilename = 161,
		SourceHostname = 162,
		SourceImage = 163,
		SourceIp = 164,
		SourceIsIpv6 = 165,
		SourceName = 166,
		SourceParentImage = 167,
		SourcePort = 168,
		SourceUser = 169,
		StartAddress = 170,
		StartFunction = 171,
		StartModule = 172,
		State = 173,
		Status = 174,
		SubStatus = 175,
		SubcategoryGuid = 176,
		SubjectDomainName = 177,
		SubjectLogonId = 178,
		SubjectUserName = 179,
		SubjectUserSid = 180,
		TargetFilename = 181,
		TargetImage = 182,
		TargetInfo = 183,
		TargetLogonId = 184,
		TargetName = 185,
		TargetObject = 186,
		TargetOutboundUserName = 187,
		TargetParentProcessId = 188,
		TargetServerName = 189,
		TargetSid = 190,
		TargetUserName = 191,
		TargetUserSid = 192,
		TaskContent = 193,
		TaskContentNew = 194,
		TaskName = 195,
		TemplateContent = 196,
		ThreatName = 197,
		TicketEncryptionType = 198,
		TicketOptions = 199,
		Type = 200,
		User = 201,
		UserName = 202,
		Value = 203,
		Workstation = 204,
		WorkstationName = 205,
		param1 = 206,
		param2 = 207,
		param3 = 208,
		payload = 209,
		process = 210,
		processPath = 211,
		subjectName = 212,
		Count = 213
	};

	enum class EEventMatch : uint8
	{
		Equals = 0,
		Contains = 1,
		StartsWith = 2,
		EndsWith = 3,
		Glob = 4
	};

	enum class EEventCaseSensitivity : uint8
	{
		IgnoreCase = 0,
		MatchCase = 1
	};

	enum class EEventMatchExpTokenType : uint8
	{
		Predicate,
		Operator,
	};

	enum class EEventMatchExpPredicate : uint8
	{
		Bool = 1,
	};

	enum class EEventMatchExpOperator : uint8
	{
		/* Pop two Boolean results and push true only if both are true; the predicate cursor is unchanged. */
		And = 1,

		/* Pop two Boolean results and push true if either is true; the predicate cursor is unchanged. */
		Or = 2,

		/* Pop one Boolean result and push its logical negation; the predicate cursor is unchanged. */
		Not = 3
	};

	enum class ERegisterRuleStatus : uint32
	{
		Success				 = 0,
		AlreadyRegistered	 = 1, // Same ID and identical serialized contents.
		InvalidRule			 = 2,
		UnsupportedEventType = 3,
		UnsupportedField	 = 4, // Includes a field unavailable for this event type.
		UnsupportedMatch	 = 5,
		LimitExceeded		 = 6,
		AllocationFailed	 = 7,
		DuplicateIdConflict  = 8
	};

	// IDs are contiguous. Count is a sentinel and cannot appear on the wire.
	inline constexpr bool IsEventKind(EEventType Kind) noexcept
	{
		return Kind > EEventType::Invalid && Kind < EEventType::Count;
	}

	inline constexpr bool IsEventFieldType(EEventFieldType Field) noexcept
	{
		return Field > EEventFieldType::Invalid && Field < EEventFieldType::Count;
	}

	inline constexpr bool IsEventServiceType(EEventServiceType Service) noexcept
	{
		return Service < EEventServiceType::Count;
	}
}
