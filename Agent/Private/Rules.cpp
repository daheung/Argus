#include "Agent/Private/Rules.h"
#include "Core/Public/DeviceControl/Protocols/RegisterRuleProtocol.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <cstdio>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace Argus::Rules
{
namespace
{
	struct FLine { size_t Indent; std::string Text; size_t Number; };
	struct FNode
	{
		std::string Scalar;
		std::map<std::string, FNode> Map;
		std::vector<FNode> List;
		enum class EKind { Scalar, Map, List } Kind = EKind::Scalar;
	};

	std::string Trim(const std::string& Text)
	{
		const auto First = Text.find_first_not_of(" \t\r\n");
		if (First == std::string::npos) return {};
		return Text.substr(First, Text.find_last_not_of(" \t\r\n") - First + 1);
	}

	std::string Lower(std::string Text)
	{
		std::transform(Text.begin(), Text.end(), Text.begin(), [](unsigned char C) { return static_cast<char>(std::tolower(C)); });
		return Text;
	}

	std::string Unquote(const std::string& Raw)
	{
		const std::string Text = Trim(Raw);
		if (Text.size() < 2 || (Text.front() != '\'' && Text.front() != '"')) return Text;
		if (Text.back() != Text.front()) throw std::runtime_error("unclosed quoted scalar");
		std::string Out;
		for (size_t I = 1; I + 1 < Text.size(); ++I)
		{
			const char C = Text[I];
			if (Text.front() == '\'' && C == '\'' && I + 2 < Text.size() && Text[I + 1] == '\'') { Out += '\''; ++I; }
			else if (Text.front() == '"' && C == '\\' && I + 2 < Text.size())
			{
				const char Next = Text[++I];
				switch (Next)
				{
				case 'n': Out += '\n'; break;
				case 'r': Out += '\r'; break;
				case 't': Out += '\t'; break;
				case '"': Out += '"'; break;
				case '\\': Out += '\\'; break;
				default: throw std::runtime_error("unsupported YAML escape");
				}
			}
			else Out += C;
		}
		return Out;
	}

	std::string RemoveComment(const std::string& Text)
	{
		char Quote = 0;
		for (size_t I = 0; I < Text.size(); ++I)
		{
			if (Quote == '\'' && Text[I] == '\'' && I + 1 < Text.size() && Text[I + 1] == '\'') { ++I; continue; }
			if (Text[I] == '\\' && Quote == '"' && I + 1 < Text.size()) { ++I; continue; }
			if (Text[I] == Quote && Quote != 0) Quote = 0;
			else if (Quote == 0 && (Text[I] == '\'' || Text[I] == '"')) Quote = Text[I];
			else if (Quote == 0 && Text[I] == '#' && (I == 0 || std::isspace(static_cast<unsigned char>(Text[I - 1])))) return Trim(Text.substr(0, I));
		}
		return Trim(Text);
	}

	size_t MappingColon(const std::string& Text)
	{
		char Quote = 0;
		for (size_t I = 0; I < Text.size(); ++I)
		{
			if (Text[I] == '\\' && Quote == '"' && I + 1 < Text.size()) { ++I; continue; }
			if (Text[I] == Quote && Quote != 0) Quote = 0;
			else if (Quote == 0 && (Text[I] == '\'' || Text[I] == '"')) Quote = Text[I];
			else if (Quote == 0 && Text[I] == ':' && (I + 1 == Text.size() || Text[I + 1] == ' ')) return I;
		}
		return std::string::npos;
	}

	FNode ParseScalar(const std::string& Text)
	{
		FNode Node;
		const std::string Value = Trim(Text);
		if (Value.empty() || Value == "|" || Value == ">" || Value[0] == '&' || Value[0] == '*' || Value[0] == '!')
			throw std::runtime_error("unsupported or empty YAML value");
		if (Value.front() == '[' && Value.back() == ']')
		{
			Node.Kind = FNode::EKind::List;
			const std::string Items = Value.substr(1, Value.size() - 2);
			char Quote = 0;
			size_t Start = 0;
			for (size_t I = 0; I <= Items.size(); ++I)
			{
				if (I < Items.size() && Items[I] == '\\' && Quote == '"' && I + 1 < Items.size()) { ++I; continue; }
				if (I < Items.size() && Items[I] == Quote && Quote != 0) Quote = 0;
				else if (I < Items.size() && Quote == 0 && (Items[I] == '\'' || Items[I] == '"')) Quote = Items[I];
				else if (I == Items.size() || (Quote == 0 && Items[I] == ','))
				{
					const std::string Item = Trim(Items.substr(Start, I - Start));
					if (!Item.empty()) Node.List.push_back(ParseScalar(Item));
					Start = I + 1;
				}
			}
			if (Quote != 0) throw std::runtime_error("unclosed flow sequence");
			return Node;
		}
		if (Value.front() == '{' || Value.front() == '[') throw std::runtime_error("unsupported flow YAML value");
		Node.Scalar = Unquote(Value);
		return Node;
	}

	FNode ParseBlock(const std::vector<FLine>& Lines, size_t& Index, size_t Indent)
	{
		if (Index >= Lines.size() || Lines[Index].Indent != Indent) throw std::runtime_error("invalid YAML indentation");
		FNode Node;
		const bool IsList = Lines[Index].Text.rfind("- ", 0) == 0;
		Node.Kind = IsList ? FNode::EKind::List : FNode::EKind::Map;
		while (Index < Lines.size() && Lines[Index].Indent == Indent)
		{
			const FLine& Line = Lines[Index];
			if (IsList)
			{
				if (Line.Text.rfind("- ", 0) != 0) throw std::runtime_error("mixed YAML mapping and sequence");
				const std::string Item = Trim(Line.Text.substr(2));
				++Index;
				if (Item.empty())
				{
					if (Index >= Lines.size() || Lines[Index].Indent <= Indent) throw std::runtime_error("empty YAML sequence item");
					Node.List.push_back(ParseBlock(Lines, Index, Lines[Index].Indent));
				}
				else if (MappingColon(Item) != std::string::npos)
				{
					// Sequence of selector mappings: - Field: value, followed by peer fields.
					std::vector<FLine> Nested{ { Indent + 2, Item, Line.Number } };
					while (Index < Lines.size() && Lines[Index].Indent > Indent) Nested.push_back(Lines[Index++]);
					size_t NestedIndex = 0;
					FNode Child = ParseBlock(Nested, NestedIndex, Indent + 2);
					if (NestedIndex != Nested.size()) throw std::runtime_error("invalid YAML list mapping");
					Node.List.push_back(std::move(Child));
				}
				else
				{
					if (Index < Lines.size() && Lines[Index].Indent > Indent) throw std::runtime_error("scalar has nested YAML value at line " + std::to_string(Line.Number));
					Node.List.push_back(ParseScalar(Item));
				}
			}
			else
			{
				const size_t Colon = MappingColon(Line.Text);
				if (Colon == std::string::npos) throw std::runtime_error("invalid YAML mapping");
				const std::string Key = Unquote(Line.Text.substr(0, Colon));
				if (Key.empty() || Node.Map.count(Key)) throw std::runtime_error("empty or duplicate YAML key");
				const std::string Value = Trim(Line.Text.substr(Colon + 1));
				++Index;
				if (Value.empty())
				{
					if (Index >= Lines.size() || Lines[Index].Indent <= Indent) throw std::runtime_error("empty YAML mapping value");
					Node.Map.emplace(Key, ParseBlock(Lines, Index, Lines[Index].Indent));
				}
				else
				{
					if (Index < Lines.size() && Lines[Index].Indent > Indent) throw std::runtime_error("scalar has nested YAML value at line " + std::to_string(Line.Number));
					Node.Map.emplace(Key, ParseScalar(Value));
				}
			}
		}
		return Node;
	}

	const FNode& Required(const FNode& Node, const char* Key)
	{
		if (Node.Kind != FNode::EKind::Map) throw std::runtime_error("expected YAML mapping");
		const auto It = Node.Map.find(Key);
		if (It == Node.Map.end()) throw std::runtime_error(std::string("missing ") + Key);
		return It->second;
	}

	std::string Scalar(const FNode& Node)
	{
		if (Node.Kind != FNode::EKind::Scalar) throw std::runtime_error("expected scalar value");
		return Node.Scalar;
	}

	void AppendOp(FRule& Rule, EEventMatchOperation Op)
	{
		if (Rule.Instructions.size() >= Argus::DeviceControl::RuleMaxInstructions)
			throw std::runtime_error("too many condition operations");
		Argus::DeviceControl::FRegisterRuleExpTokenRecordRP Token{};
		Token.ExpTokenType = Argus::Policy::EEventMatchExpTokenType::Operator;
		Token.OperatorType = Op;
		Rule.Instructions.push_back(Token);
	}

	void AppendPredicate(FRule& Rule)
	{
		if (Rule.Instructions.size() >= Argus::DeviceControl::RuleMaxInstructions)
			throw std::runtime_error("too many condition operations");
		Argus::DeviceControl::FRegisterRuleExpTokenRecordRP Token{};
		Token.ExpTokenType = Argus::Policy::EEventMatchExpTokenType::Predicate;
		Token.PredicateType = Argus::Policy::EEventMatchExpPredicate::Bool;
		Rule.Instructions.push_back(Token);
	}

	EEventFieldType ResolveField(const std::string& Name)
	{
		static const std::map<std::string, EEventFieldType> Fields = {
			{ "AccessList", Argus::Policy::EEventFieldType::AccessList },
			{ "AccessMask", Argus::Policy::EEventFieldType::AccessMask },
			{ "AccountName", Argus::Policy::EEventFieldType::AccountName },
			{ "Action", Argus::Policy::EEventFieldType::Action },
			{ "AdditionalInfo", Argus::Policy::EEventFieldType::AdditionalInfo },
			{ "Address", Argus::Policy::EEventFieldType::Address },
			{ "AllowedToDelegateTo", Argus::Policy::EEventFieldType::AllowedToDelegateTo },
			{ "AppID", Argus::Policy::EEventFieldType::AppID },
			{ "AppName", Argus::Policy::EEventFieldType::AppName },
			{ "AppVersion", Argus::Policy::EEventFieldType::AppVersion },
			{ "Application", Argus::Policy::EEventFieldType::Application },
			{ "ApplicationPath", Argus::Policy::EEventFieldType::ApplicationPath },
			{ "AttributeLDAPDisplayName", Argus::Policy::EEventFieldType::AttributeLDAPDisplayName },
			{ "AttributeValue", Argus::Policy::EEventFieldType::AttributeValue },
			{ "Attributes", Argus::Policy::EEventFieldType::Attributes },
			{ "AuditPolicyChanges", Argus::Policy::EEventFieldType::AuditPolicyChanges },
			{ "AuditSourceName", Argus::Policy::EEventFieldType::AuditSourceName },
			{ "AuthenticationPackageName", Argus::Policy::EEventFieldType::AuthenticationPackageName },
			{ "Binary", Argus::Policy::EEventFieldType::Binary },
			{ "CallTrace", Argus::Policy::EEventFieldType::CallTrace },
			{ "CallerProcessName", Argus::Policy::EEventFieldType::CallerProcessName },
			{ "CallingProcess", Argus::Policy::EEventFieldType::CallingProcess },
			{ "Caption", Argus::Policy::EEventFieldType::Caption },
			{ "CertThumbprint", Argus::Policy::EEventFieldType::CertThumbprint },
			{ "Channel", Argus::Policy::EEventFieldType::Channel },
			{ "ClassName", Argus::Policy::EEventFieldType::ClassName },
			{ "ClientAddress", Argus::Policy::EEventFieldType::ClientAddress },
			{ "ClientProcessId", Argus::Policy::EEventFieldType::ClientProcessId },
			{ "CommandLine", Argus::Policy::EEventFieldType::CommandLine },
			{ "Company", Argus::Policy::EEventFieldType::Company },
			{ "Configuration", Argus::Policy::EEventFieldType::Configuration },
			{ "Contents", Argus::Policy::EEventFieldType::Contents },
			{ "ContextInfo", Argus::Policy::EEventFieldType::ContextInfo },
			{ "CreationUtcTime", Argus::Policy::EEventFieldType::CreationUtcTime },
			{ "CurrentDirectory", Argus::Policy::EEventFieldType::CurrentDirectory },
			{ "Data", Argus::Policy::EEventFieldType::Data },
			{ "Description", Argus::Policy::EEventFieldType::Description },
			{ "DestAddress", Argus::Policy::EEventFieldType::DestAddress },
			{ "DestPort", Argus::Policy::EEventFieldType::DestPort },
			{ "Destination", Argus::Policy::EEventFieldType::Destination },
			{ "DestinationHostname", Argus::Policy::EEventFieldType::DestinationHostname },
			{ "DestinationIp", Argus::Policy::EEventFieldType::DestinationIp },
			{ "DestinationIsIpv6", Argus::Policy::EEventFieldType::DestinationIsIpv6 },
			{ "DestinationPort", Argus::Policy::EEventFieldType::DestinationPort },
			{ "Details", Argus::Policy::EEventFieldType::Details },
			{ "Device", Argus::Policy::EEventFieldType::Device },
			{ "DeviceDescription", Argus::Policy::EEventFieldType::DeviceDescription },
			{ "DeviceName", Argus::Policy::EEventFieldType::DeviceName },
			{ "DistinguishedName", Argus::Policy::EEventFieldType::DistinguishedName },
			{ "EncyptionUsed", Argus::Policy::EEventFieldType::EncyptionUsed },
			{ "ErrorCode", Argus::Policy::EEventFieldType::ErrorCode },
			{ "EventID", Argus::Policy::EEventFieldType::EventID },
			{ "EventType", Argus::Policy::EEventFieldType::EventType },
			{ "ExceptionCode", Argus::Policy::EEventFieldType::ExceptionCode },
			{ "Feature_Name", Argus::Policy::EEventFieldType::Feature_Name },
			{ "FileName", Argus::Policy::EEventFieldType::FileName },
			{ "FileNameBuffer", Argus::Policy::EEventFieldType::FileNameBuffer },
			{ "FileVersion", Argus::Policy::EEventFieldType::FileVersion },
			{ "FilterName", Argus::Policy::EEventFieldType::FilterName },
			{ "FilterOrigin", Argus::Policy::EEventFieldType::FilterOrigin },
			{ "Flags", Argus::Policy::EEventFieldType::Flags },
			{ "GrandParentImage", Argus::Policy::EEventFieldType::GrandParentImage },
			{ "GrantedAccess", Argus::Policy::EEventFieldType::GrantedAccess },
			{ "HasFullTrust", Argus::Policy::EEventFieldType::HasFullTrust },
			{ "Hash", Argus::Policy::EEventFieldType::Hash },
			{ "Hashes", Argus::Policy::EEventFieldType::Hashes },
			{ "HiveName", Argus::Policy::EEventFieldType::HiveName },
			{ "Image", Argus::Policy::EEventFieldType::Image },
			{ "ImageLoaded", Argus::Policy::EEventFieldType::ImageLoaded },
			{ "ImageName", Argus::Policy::EEventFieldType::ImageName },
			{ "ImagePath", Argus::Policy::EEventFieldType::ImagePath },
			{ "ImpersonationLevel", Argus::Policy::EEventFieldType::ImpersonationLevel },
			{ "Initiated", Argus::Policy::EEventFieldType::Initiated },
			{ "IntegrityLevel", Argus::Policy::EEventFieldType::IntegrityLevel },
			{ "IpAddress", Argus::Policy::EEventFieldType::IpAddress },
			{ "IpPort", Argus::Policy::EEventFieldType::IpPort },
			{ "IsatapRouter", Argus::Policy::EEventFieldType::IsatapRouter },
			{ "KeyLength", Argus::Policy::EEventFieldType::KeyLength },
			{ "Keywords", Argus::Policy::EEventFieldType::Keywords },
			{ "LayerRTID", Argus::Policy::EEventFieldType::LayerRTID },
			{ "Level", Argus::Policy::EEventFieldType::Level },
			{ "LocalName", Argus::Policy::EEventFieldType::LocalName },
			{ "LogonId", Argus::Policy::EEventFieldType::LogonId },
			{ "LogonProcessName", Argus::Policy::EEventFieldType::LogonProcessName },
			{ "LogonType", Argus::Policy::EEventFieldType::LogonType },
			{ "Message", Argus::Policy::EEventFieldType::Message },
			{ "ModifyingApplication", Argus::Policy::EEventFieldType::ModifyingApplication },
			{ "ModifyingUser", Argus::Policy::EEventFieldType::ModifyingUser },
			{ "ModuleName", Argus::Policy::EEventFieldType::ModuleName },
			{ "Name", Argus::Policy::EEventFieldType::Name },
			{ "NewName", Argus::Policy::EEventFieldType::NewName },
			{ "NewTargetUserName", Argus::Policy::EEventFieldType::NewTargetUserName },
			{ "NewTemplateContent", Argus::Policy::EEventFieldType::NewTemplateContent },
			{ "NewUacValue", Argus::Policy::EEventFieldType::NewUacValue },
			{ "NewValue", Argus::Policy::EEventFieldType::NewValue },
			{ "ObjectClass", Argus::Policy::EEventFieldType::ObjectClass },
			{ "ObjectDN", Argus::Policy::EEventFieldType::ObjectDN },
			{ "ObjectName", Argus::Policy::EEventFieldType::ObjectName },
			{ "ObjectServer", Argus::Policy::EEventFieldType::ObjectServer },
			{ "ObjectType", Argus::Policy::EEventFieldType::ObjectType },
			{ "ObjectValueName", Argus::Policy::EEventFieldType::ObjectValueName },
			{ "OldTargetUserName", Argus::Policy::EEventFieldType::OldTargetUserName },
			{ "OldUacValue", Argus::Policy::EEventFieldType::OldUacValue },
			{ "OldValue", Argus::Policy::EEventFieldType::OldValue },
			{ "Operation", Argus::Policy::EEventFieldType::Operation },
			{ "Origin", Argus::Policy::EEventFieldType::Origin },
			{ "OriginalFileName", Argus::Policy::EEventFieldType::OriginalFileName },
			{ "PackageFullName", Argus::Policy::EEventFieldType::PackageFullName },
			{ "PackagePath", Argus::Policy::EEventFieldType::PackagePath },
			{ "PackageSourceUri", Argus::Policy::EEventFieldType::PackageSourceUri },
			{ "ParentCommandLine", Argus::Policy::EEventFieldType::ParentCommandLine },
			{ "ParentImage", Argus::Policy::EEventFieldType::ParentImage },
			{ "ParentProcessId", Argus::Policy::EEventFieldType::ParentProcessId },
			{ "ParentUser", Argus::Policy::EEventFieldType::ParentUser },
			{ "Path", Argus::Policy::EEventFieldType::Path },
			{ "Payload", Argus::Policy::EEventFieldType::Payload },
			{ "PipeName", Argus::Policy::EEventFieldType::PipeName },
			{ "PossibleCause", Argus::Policy::EEventFieldType::PossibleCause },
			{ "PreAuthType", Argus::Policy::EEventFieldType::PreAuthType },
			{ "PreviousCreationUtcTime", Argus::Policy::EEventFieldType::PreviousCreationUtcTime },
			{ "PrivilegeList", Argus::Policy::EEventFieldType::PrivilegeList },
			{ "ProcessId", Argus::Policy::EEventFieldType::ProcessId },
			{ "ProcessName", Argus::Policy::EEventFieldType::ProcessName },
			{ "ProcessNameBuffer", Argus::Policy::EEventFieldType::ProcessNameBuffer },
			{ "ProcessPath", Argus::Policy::EEventFieldType::ProcessPath },
			{ "Product", Argus::Policy::EEventFieldType::Product },
			{ "Properties", Argus::Policy::EEventFieldType::Properties },
			{ "Protocol", Argus::Policy::EEventFieldType::Protocol },
			{ "Provider", Argus::Policy::EEventFieldType::Provider },
			{ "ProviderContextName", Argus::Policy::EEventFieldType::ProviderContextName },
			{ "Provider_Name", Argus::Policy::EEventFieldType::Provider_Name },
			{ "QNAME", Argus::Policy::EEventFieldType::QNAME },
			{ "Query", Argus::Policy::EEventFieldType::Query },
			{ "QueryName", Argus::Policy::EEventFieldType::QueryName },
			{ "Reason", Argus::Policy::EEventFieldType::Reason },
			{ "RelativeTargetName", Argus::Policy::EEventFieldType::RelativeTargetName },
			{ "RemoteAddress", Argus::Policy::EEventFieldType::RemoteAddress },
			{ "RemoteName", Argus::Policy::EEventFieldType::RemoteName },
			{ "RequestedPolicy", Argus::Policy::EEventFieldType::RequestedPolicy },
			{ "SamAccountName", Argus::Policy::EEventFieldType::SamAccountName },
			{ "ScriptBlockText", Argus::Policy::EEventFieldType::ScriptBlockText },
			{ "SearchFilter", Argus::Policy::EEventFieldType::SearchFilter },
			{ "ServerAddress", Argus::Policy::EEventFieldType::ServerAddress },
			{ "ServerName", Argus::Policy::EEventFieldType::ServerName },
			{ "Service", Argus::Policy::EEventFieldType::Service },
			{ "ServiceFileName", Argus::Policy::EEventFieldType::ServiceFileName },
			{ "ServiceName", Argus::Policy::EEventFieldType::ServiceName },
			{ "ServicePrincipalNames", Argus::Policy::EEventFieldType::ServicePrincipalNames },
			{ "ServiceStartType", Argus::Policy::EEventFieldType::ServiceStartType },
			{ "ServiceType", Argus::Policy::EEventFieldType::ServiceType },
			{ "ShareLocalPath", Argus::Policy::EEventFieldType::ShareLocalPath },
			{ "ShareName", Argus::Policy::EEventFieldType::ShareName },
			{ "SidHistory", Argus::Policy::EEventFieldType::SidHistory },
			{ "SidList", Argus::Policy::EEventFieldType::SidList },
			{ "Signature", Argus::Policy::EEventFieldType::Signature },
			{ "SignatureStatus", Argus::Policy::EEventFieldType::SignatureStatus },
			{ "Signed", Argus::Policy::EEventFieldType::Signed },
			{ "SigningUsed", Argus::Policy::EEventFieldType::SigningUsed },
			{ "SourceAddress", Argus::Policy::EEventFieldType::SourceAddress },
			{ "SourceCommandLine", Argus::Policy::EEventFieldType::SourceCommandLine },
			{ "SourceFilename", Argus::Policy::EEventFieldType::SourceFilename },
			{ "SourceHostname", Argus::Policy::EEventFieldType::SourceHostname },
			{ "SourceImage", Argus::Policy::EEventFieldType::SourceImage },
			{ "SourceIp", Argus::Policy::EEventFieldType::SourceIp },
			{ "SourceIsIpv6", Argus::Policy::EEventFieldType::SourceIsIpv6 },
			{ "SourceName", Argus::Policy::EEventFieldType::SourceName },
			{ "SourceParentImage", Argus::Policy::EEventFieldType::SourceParentImage },
			{ "SourcePort", Argus::Policy::EEventFieldType::SourcePort },
			{ "SourceUser", Argus::Policy::EEventFieldType::SourceUser },
			{ "StartAddress", Argus::Policy::EEventFieldType::StartAddress },
			{ "StartFunction", Argus::Policy::EEventFieldType::StartFunction },
			{ "StartModule", Argus::Policy::EEventFieldType::StartModule },
			{ "State", Argus::Policy::EEventFieldType::State },
			{ "Status", Argus::Policy::EEventFieldType::Status },
			{ "SubStatus", Argus::Policy::EEventFieldType::SubStatus },
			{ "SubcategoryGuid", Argus::Policy::EEventFieldType::SubcategoryGuid },
			{ "SubjectDomainName", Argus::Policy::EEventFieldType::SubjectDomainName },
			{ "SubjectLogonId", Argus::Policy::EEventFieldType::SubjectLogonId },
			{ "SubjectUserName", Argus::Policy::EEventFieldType::SubjectUserName },
			{ "SubjectUserSid", Argus::Policy::EEventFieldType::SubjectUserSid },
			{ "TargetFilename", Argus::Policy::EEventFieldType::TargetFilename },
			{ "TargetImage", Argus::Policy::EEventFieldType::TargetImage },
			{ "TargetInfo", Argus::Policy::EEventFieldType::TargetInfo },
			{ "TargetLogonId", Argus::Policy::EEventFieldType::TargetLogonId },
			{ "TargetName", Argus::Policy::EEventFieldType::TargetName },
			{ "TargetObject", Argus::Policy::EEventFieldType::TargetObject },
			{ "TargetOutboundUserName", Argus::Policy::EEventFieldType::TargetOutboundUserName },
			{ "TargetParentProcessId", Argus::Policy::EEventFieldType::TargetParentProcessId },
			{ "TargetServerName", Argus::Policy::EEventFieldType::TargetServerName },
			{ "TargetSid", Argus::Policy::EEventFieldType::TargetSid },
			{ "TargetUserName", Argus::Policy::EEventFieldType::TargetUserName },
			{ "TargetUserSid", Argus::Policy::EEventFieldType::TargetUserSid },
			{ "TaskContent", Argus::Policy::EEventFieldType::TaskContent },
			{ "TaskContentNew", Argus::Policy::EEventFieldType::TaskContentNew },
			{ "TaskName", Argus::Policy::EEventFieldType::TaskName },
			{ "TemplateContent", Argus::Policy::EEventFieldType::TemplateContent },
			{ "ThreatName", Argus::Policy::EEventFieldType::ThreatName },
			{ "TicketEncryptionType", Argus::Policy::EEventFieldType::TicketEncryptionType },
			{ "TicketOptions", Argus::Policy::EEventFieldType::TicketOptions },
			{ "Type", Argus::Policy::EEventFieldType::Type },
			{ "User", Argus::Policy::EEventFieldType::User },
			{ "UserName", Argus::Policy::EEventFieldType::UserName },
			{ "Value", Argus::Policy::EEventFieldType::Value },
			{ "Workstation", Argus::Policy::EEventFieldType::Workstation },
			{ "WorkstationName", Argus::Policy::EEventFieldType::WorkstationName },
			{ "param1", Argus::Policy::EEventFieldType::param1 },
			{ "param2", Argus::Policy::EEventFieldType::param2 },
			{ "param3", Argus::Policy::EEventFieldType::param3 },
			{ "payload", Argus::Policy::EEventFieldType::payload },
			{ "process", Argus::Policy::EEventFieldType::process },
			{ "processPath", Argus::Policy::EEventFieldType::processPath },
			{ "subjectName", Argus::Policy::EEventFieldType::subjectName },
		};
		const auto It = Fields.find(Name);
		if (It == Fields.end()) throw std::runtime_error("unknown Sigma field: " + Name);
		return It->second;
	}

	EEventServiceType ResolveServiceType(const std::string& Name)
	{
		static const std::map<std::string, EEventServiceType> Services = {
			{ "application", EEventServiceType::Application },
			{ "applocker", EEventServiceType::AppLocker },
			{ "appmodel-runtime", EEventServiceType::AppModelRuntime },
			{ "appxdeployment-server", EEventServiceType::AppXDeploymentServer },
			{ "appxpackaging-om", EEventServiceType::AppXPackagingOm },
			{ "bits-client", EEventServiceType::BitsClient },
			{ "capi2", EEventServiceType::Capi2 },
			{ "certificateservicesclient-lifecycle-system", EEventServiceType::CertificateServicesClientLifecycleSystem },
			{ "codeintegrity-operational", EEventServiceType::CodeIntegrityOperational },
			{ "diagnosis-scripted", EEventServiceType::DiagnosisScripted },
			{ "dns-client", EEventServiceType::DnsClient },
			{ "dns-server", EEventServiceType::DnsServer },
			{ "dns-server-analytic", EEventServiceType::DnsServerAnalytic },
			{ "driver-framework", EEventServiceType::DriverFramework },
			{ "firewall-as", EEventServiceType::FirewallAs },
			{ "iis-configuration", EEventServiceType::IisConfiguration },
			{ "ldap", EEventServiceType::Ldap },
			{ "lsa-server", EEventServiceType::LsaServer },
			{ "microsoft-servicebus-client", EEventServiceType::MicrosoftServiceBusClient },
			{ "msexchange-management", EEventServiceType::MsExchangeManagement },
			{ "ntlm", EEventServiceType::Ntlm },
			{ "openssh", EEventServiceType::OpenSsh },
			{ "powershell-classic", EEventServiceType::PowerShellClassic },
			{ "printservice-admin", EEventServiceType::PrintServiceAdmin },
			{ "printservice-operational", EEventServiceType::PrintServiceOperational },
			{ "security", EEventServiceType::Security },
			{ "security-mitigations", EEventServiceType::SecurityMitigations },
			{ "shell-core", EEventServiceType::ShellCore },
			{ "smbclient-connectivity", EEventServiceType::SmbClientConnectivity },
			{ "smbclient-security", EEventServiceType::SmbClientSecurity },
			{ "smbserver-connectivity", EEventServiceType::SmbServerConnectivity },
			{ "sysmon", EEventServiceType::Sysmon },
			{ "system", EEventServiceType::System },
			{ "taskscheduler", EEventServiceType::TaskScheduler },
			{ "terminalservices-localsessionmanager", EEventServiceType::TerminalServicesLocalSessionManager },
			{ "windefend", EEventServiceType::WinDefend },
			{ "wmi", EEventServiceType::Wmi }
		};
		const auto It = Services.find(Name);
		if (It == Services.end()) throw std::runtime_error("unsupported logsource.service: " + Name);
		return It->second;
	}

	EEventType ResolveEventType(const std::string& Category, EEventServiceType Service)
	{
		static const std::map<std::string, EEventType> Events = {
			{ "process_creation", Argus::Policy::EEventType::ProcessCreate },
			{ "registry_set", Argus::Policy::EEventType::RegistrySet },
			{ "image_load", Argus::Policy::EEventType::ImageLoad },
			{ "registry_event", Argus::Policy::EEventType::RegistryEvent },
			{ "registry_add", Argus::Policy::EEventType::RegistryAdd },
			{ "registry_delete", Argus::Policy::EEventType::RegistryDelete },
			{ "file_event", Argus::Policy::EEventType::FileEvent },
			{ "file_access", Argus::Policy::EEventType::FileAccess },
			{ "file_change", Argus::Policy::EEventType::FileChange },
			{ "file_delete", Argus::Policy::EEventType::FileDelete },
			{ "file_rename", Argus::Policy::EEventType::FileRename },
			{ "file_executable_detected", Argus::Policy::EEventType::FileExecutableDetected },
			{ "create_remote_thread", Argus::Policy::EEventType::CreateRemoteThread },
			{ "create_stream_hash", Argus::Policy::EEventType::CreateStreamHash },
			{ "dns_query", Argus::Policy::EEventType::DnsQuery },
			{ "driver_load", Argus::Policy::EEventType::DriverLoad },
			{ "network_connection", Argus::Policy::EEventType::NetworkConnection },
			{ "pipe_created", Argus::Policy::EEventType::PipeCreated },
			{ "process_access", Argus::Policy::EEventType::ProcessAccess },
			{ "process_tampering", Argus::Policy::EEventType::ProcessTampering },
			{ "ps_classic_provider_start", Argus::Policy::EEventType::PsClassicProviderStart },
			{ "ps_classic_start", Argus::Policy::EEventType::PsClassicStart },
			{ "ps_module", Argus::Policy::EEventType::PsModule },
			{ "ps_script", Argus::Policy::EEventType::PsScript },
			{ "raw_access_thread", Argus::Policy::EEventType::RawAccessThread },
			{ "sysmon_error", Argus::Policy::EEventType::SysmonError },
			{ "sysmon_status", Argus::Policy::EEventType::SysmonStatus },
			{ "wmi_event", Argus::Policy::EEventType::WmiEvent },
		};
		if (Category.empty())
		{
			if (Service == EEventServiceType::None) throw std::runtime_error("logsource needs category or service");
			return Argus::Policy::EEventType::WindowsEventLog;
		}
		const auto It = Events.find(Category);
		if (It == Events.end()) throw std::runtime_error("unsupported logsource.category: " + Category);
		return It->second;
	}

	void AddField(FRule& Rule, const std::string& RawField, const FNode& Values)
	{
		const size_t Pipe = RawField.find('|');
		const std::string Field = RawField.substr(0, Pipe);
		if (Field.empty()) throw std::runtime_error("empty field name");
		const EEventFieldType FieldType = ResolveField(Field);
		EEventMatch Match = Argus::Policy::EEventMatch::Equals;
		bool All = false;
		EEventCaseSensitivity CaseSensitivity = EEventCaseSensitivity::IgnoreCase;
		if (Pipe != std::string::npos)
		{
			size_t Begin = Pipe + 1;
			while (Begin < RawField.size())
			{
				const size_t End = RawField.find('|', Begin);
				const std::string Modifier = Lower(RawField.substr(Begin, End - Begin));
				if (Modifier == "contains") Match = Argus::Policy::EEventMatch::Contains;
				else if (Modifier == "startswith") Match = Argus::Policy::EEventMatch::StartsWith;
				else if (Modifier == "endswith") Match = Argus::Policy::EEventMatch::EndsWith;
				else if (Modifier == "all") All = true;
				else if (Modifier == "cased") CaseSensitivity = EEventCaseSensitivity::MatchCase;
				else throw std::runtime_error("unsupported field modifier: " + Modifier);
				if (End == std::string::npos) break;
				Begin = End + 1;
			}
		}
		const auto AddValue = [&](const FNode& Item)
		{
			const std::string Value = Scalar(Item);
			if (Value.empty() || Value == "null" || Value == "~") throw std::runtime_error("empty/null match value");
			const EEventMatch EffectiveMatch = Match == Argus::Policy::EEventMatch::Equals &&
				(Value.find('*') != std::string::npos || Value.find('?') != std::string::npos)
				? Argus::Policy::EEventMatch::Glob : Match;
			if (Match != Argus::Policy::EEventMatch::Equals && (Value.find('*') != std::string::npos || Value.find('?') != std::string::npos))
				throw std::runtime_error("wildcards with field modifier are unsupported");
			if (Rule.Predicates.size() >= Argus::DeviceControl::RuleMaxPredicates) throw std::runtime_error("too many predicates");
			AppendPredicate(Rule);
			Rule.Predicates.push_back({ FieldType, Value, EffectiveMatch, CaseSensitivity });
		};
		if (Values.Kind == FNode::EKind::List)
		{
			if (Values.List.empty()) throw std::runtime_error("empty match list");
			for (size_t I = 0; I < Values.List.size(); ++I)
			{
				AddValue(Values.List[I]);
				if (I) AppendOp(Rule, All ? Argus::Policy::EEventMatchExpOperator::And : Argus::Policy::EEventMatchExpOperator::Or);
			}
		}
		else AddValue(Values);
	}

	void AddSelector(FRule& Rule, const FNode& Node)
	{
		if (Node.Kind == FNode::EKind::List)
		{
			if (Node.List.empty()) throw std::runtime_error("empty selector list");
			for (size_t I = 0; I < Node.List.size(); ++I)
			{
				AddSelector(Rule, Node.List[I]);
				if (I) AppendOp(Rule, Argus::Policy::EEventMatchExpOperator::Or);
			}
			return;
		}
		if (Node.Kind != FNode::EKind::Map || Node.Map.empty()) throw std::runtime_error("selector must be a nonempty mapping");
		size_t Count = 0;
		for (const auto& Pair : Node.Map)
		{
			AddField(Rule, Pair.first, Pair.second);
			if (Count++) AppendOp(Rule, Argus::Policy::EEventMatchExpOperator::And);
		}
	}

	struct FConditionParser
	{
		FRule& Rule;
		const FNode& Detection;
		std::vector<std::string> Tokens;
		size_t At = 0;

		FConditionParser(FRule& InRule, const FNode& InDetection, const std::string& Condition) : Rule(InRule), Detection(InDetection)
		{
			// The program can only read predicates from the current event.
			// Legacy aggregation syntax requires counters/history across events.
			if (Condition.find('|') != std::string::npos)
				throw std::runtime_error("aggregation requires stateful matching");
			for (size_t I = 0; I < Condition.size();)
			{
				if (std::isspace(static_cast<unsigned char>(Condition[I]))) { ++I; continue; }
				if (Condition[I] == '(' || Condition[I] == ')') { Tokens.emplace_back(1, Condition[I++]); continue; }
				const size_t Start = I;
				while (I < Condition.size() && !std::isspace(static_cast<unsigned char>(Condition[I])) && Condition[I] != '(' && Condition[I] != ')') ++I;
				Tokens.push_back(Condition.substr(Start, I - Start));
			}
		}

		bool Eat(const char* Token)
		{
			if (At < Tokens.size() && Lower(Tokens[At]) == Token) { ++At; return true; }
			return false;
		}

		void Selector(const std::string& Name)
		{
			const auto It = Detection.Map.find(Name);
			if (It == Detection.Map.end() || Name == "condition") throw std::runtime_error("unknown selector: " + Name);
			AddSelector(Rule, It->second);
		}

		void Primary()
		{
			if (Eat("(")) { Or(); if (!Eat(")")) throw std::runtime_error("unclosed condition parenthesis"); return; }
			if (At >= Tokens.size()) throw std::runtime_error("incomplete condition");
			const std::string Word = Tokens[At++];
			if (Lower(Word) == "1" || Lower(Word) == "all")
			{
				if (!Eat("of") || At >= Tokens.size()) throw std::runtime_error("invalid condition quantifier");
				const std::string Pattern = Tokens[At++];
				std::vector<std::string> Names;
				for (const auto& Pair : Detection.Map)
				{
					if (Pair.first == "condition" || Pair.first == "timeframe") continue;
					if (Pattern == "them" || (Pattern.back() == '*' && Pair.first.rfind(Pattern.substr(0, Pattern.size() - 1), 0) == 0)) Names.push_back(Pair.first);
				}
				if (Names.empty()) throw std::runtime_error("condition quantifier matches no selectors");
				for (size_t I = 0; I < Names.size(); ++I) { Selector(Names[I]); if (I) AppendOp(Rule, Lower(Word) == "all" ? Argus::Policy::EEventMatchExpOperator::And : Argus::Policy::EEventMatchExpOperator::Or); }
				return;
			}
			Selector(Word);
		}

		void Not() { if (Eat("not")) { Not(); AppendOp(Rule, Argus::Policy::EEventMatchExpOperator::Not); } else Primary(); }
		void And() { Not(); while (Eat("and")) { Not(); AppendOp(Rule, Argus::Policy::EEventMatchExpOperator::And); } }
		void Or() { And(); while (Eat("or")) { And(); AppendOp(Rule, Argus::Policy::EEventMatchExpOperator::Or); } }
		void Parse() { Or(); if (At != Tokens.size()) throw std::runtime_error("unsupported condition syntax"); }
	};

	FRule Convert(const std::vector<std::string>& Document)
	{
		FRule Rule;
		std::vector<FLine> DetectionLines;
		bool InDetection = false;
		for (size_t I = 0; I < Document.size(); ++I)
		{
			const std::string& Raw = Document[I];
			if (Raw.find('\t') != std::string::npos) throw std::runtime_error("YAML tabs are unsupported");
			const size_t Indent = Raw.find_first_not_of(' ');
			if (Indent == std::string::npos || Raw[Indent] == '#') continue;
			const std::string Text = RemoveComment(Raw.substr(Indent));
			if (Text.empty()) continue;
			if (Indent == 0)
			{
				InDetection = Text == "detection:";
				if (!InDetection)
				{
					const size_t Colon = MappingColon(Text);
					if (Colon == std::string::npos) continue;
					const std::string Key = Unquote(Trim(Text.substr(0, Colon)));
					const std::string Value = Text.substr(Colon + 1);
					// Check before conversion even when detection is present: ignoring
					// these keys would turn a dependent rule into an independent one.
					if (Key == "correlation") throw std::runtime_error("correlation requires stateful matching; only single-event rules are supported");
					if (Key == "action") throw std::runtime_error("rule collection actions are unsupported; each document must be a standalone single-event rule");
					if (Key == "id") Rule.Id = Unquote(Value);
					else if (Key == "title") Rule.Title = Unquote(Value);
					else if (Key == "status" && (Unquote(Value) == "deprecated" || Unquote(Value) == "unsupported")) throw std::runtime_error("inactive rule");
				}
			}
			else if (InDetection) DetectionLines.push_back({ Indent, Text, I + 1 });
		}
		if (Rule.Id.empty() || Rule.Title.empty() || DetectionLines.empty()) throw std::runtime_error("missing id, title or detection");
		size_t Index = 0;
		FNode Detection = ParseBlock(DetectionLines, Index, DetectionLines[0].Indent);
		if (Index != DetectionLines.size()) throw std::runtime_error("invalid detection indentation");
		if (Detection.Map.count("timeframe")) throw std::runtime_error("timeframe requires stateful matching");
		FConditionParser(Rule, Detection, Scalar(Required(Detection, "condition"))).Parse();
		// The logsource is parsed independently so its fields cannot be treated as detection predicates.
		std::vector<FLine> LogsourceLines;
		bool InLogsource = false;
		size_t LogsourceIndent = 0;
		for (size_t I = 0; I < Document.size(); ++I)
		{
			const std::string& Raw = Document[I];
			const size_t Indent = Raw.find_first_not_of(' ');
			if (Indent == std::string::npos || Raw[Indent] == '#') continue;
			const std::string Text = RemoveComment(Raw.substr(Indent));
			if (Text.empty()) continue;
			if (Indent == 0) { InLogsource = Text == "logsource:"; LogsourceIndent = 0; continue; }
			if (InLogsource)
			{
				if (LogsourceIndent == 0) LogsourceIndent = Indent;
				if (Indent == LogsourceIndent &&
					(Text.rfind("product:", 0) == 0 || Text.rfind("category:", 0) == 0 || Text.rfind("service:", 0) == 0))
					LogsourceLines.push_back({ Indent, Text, I + 1 });
			}
		}
		if (LogsourceLines.empty()) throw std::runtime_error("missing logsource");
		Index = 0;
		const FNode Logsource = ParseBlock(LogsourceLines, Index, LogsourceLines[0].Indent);
		if (Index != LogsourceLines.size()) throw std::runtime_error("invalid logsource indentation");
		const std::string Product = Scalar(Required(Logsource, "product"));
		std::string Category;
		if (const auto It = Logsource.Map.find("category"); It != Logsource.Map.end()) Category = Lower(Scalar(It->second));
		if (const auto It = Logsource.Map.find("service"); It != Logsource.Map.end())
			Rule.ServiceType = ResolveServiceType(Lower(Scalar(It->second)));
		if (Lower(Product) != "windows") throw std::runtime_error("non-Windows logsource");
		Rule.EventType = ResolveEventType(Category, Rule.ServiceType);
		return Rule;
	}

	void WriteBytes(std::vector<uint8_t>& Bytes, const void* Data, size_t Length)
	{
		if (Length == 0) return;
		if (Length > Argus::DeviceControl::RuleMaxRequestBytes - Bytes.size())
			throw std::runtime_error("serialized rule exceeds 64 KiB");
		const auto* Ptr = static_cast<const uint8_t*>(Data);
		Bytes.insert(Bytes.end(), Ptr, Ptr + Length);
	}

	Argus::DeviceControl::FStringDescriptor WriteString(std::vector<uint8_t>& Bytes, const std::string& String)
	{
		if (String.find('\0') != std::string::npos) throw std::runtime_error("embedded null in rule string");
		if (String.empty()) return {};
		const auto Offset = static_cast<uint64>(Bytes.size());
		WriteBytes(Bytes, String.data(), String.size());
		return { Offset, static_cast<uint64>(String.size()) };
	}

	void ValidateRule(const FRule& Rule)
	{
		using namespace Argus::DeviceControl;
		if (!Argus::Policy::IsEventKind(Rule.EventType)) throw std::runtime_error("invalid event kind");
		if (!Argus::Policy::IsEventServiceType(Rule.ServiceType)) throw std::runtime_error("invalid service type");
		if (Rule.EventType == EEventType::WindowsEventLog && Rule.ServiceType == EEventServiceType::None) throw std::runtime_error("event log requires service");
		if (Rule.Id.empty() || Rule.Title.empty()) throw std::runtime_error("missing rule id or title");
		if (Rule.Predicates.empty() || Rule.Predicates.size() > RuleMaxPredicates) throw std::runtime_error("invalid predicate count");
		if (Rule.Instructions.empty() || Rule.Instructions.size() > RuleMaxInstructions) throw std::runtime_error("invalid instruction count");
		for (const auto& Predicate : Rule.Predicates)
		{
			if (!Argus::Policy::IsEventFieldType(Predicate.FieldType)) throw std::runtime_error("invalid field id");
			if (static_cast<uint8>(Predicate.Match) > static_cast<uint8>(Argus::Policy::EEventMatch::Glob)) throw std::runtime_error("invalid match operator");
			if (Predicate.CaseSensitivity != EEventCaseSensitivity::IgnoreCase &&
				Predicate.CaseSensitivity != EEventCaseSensitivity::MatchCase) throw std::runtime_error("invalid case sensitivity");
			if (Predicate.Value.empty()) throw std::runtime_error("empty match value");
		}
		uint32_t Depth = 0;
		size_t NextPredicate = 0;
		for (const auto& Token : Rule.Instructions)
		{
			switch (Token.ExpTokenType)
			{
			case Argus::Policy::EEventMatchExpTokenType::Predicate:
				if (Token.PredicateType != Argus::Policy::EEventMatchExpPredicate::Bool)
					throw std::runtime_error("invalid predicate token");
				if (NextPredicate >= Rule.Predicates.size()) throw std::runtime_error("too many predicate operations");
				++NextPredicate;
				++Depth;
				break;
			case Argus::Policy::EEventMatchExpTokenType::Operator:
				switch (Token.OperatorType)
				{
				case Argus::Policy::EEventMatchExpOperator::And:
				case Argus::Policy::EEventMatchExpOperator::Or:
					if (Depth < 2) throw std::runtime_error("invalid binary instruction");
					--Depth;
					break;
				case Argus::Policy::EEventMatchExpOperator::Not:
					if (Depth < 1) throw std::runtime_error("invalid unary instruction");
					break;
				default: throw std::runtime_error("invalid instruction operator");
				}
				break;
			default: throw std::runtime_error("invalid expression token type");
			}
		}
		if (Depth != 1) throw std::runtime_error("invalid final stack depth");
		if (NextPredicate != Rule.Predicates.size()) throw std::runtime_error("unreferenced predicates");
	}
}

FLoadResult LoadRules(const std::string& Utf8Yaml)
{
	FLoadResult Result;
	std::vector<std::string> Document;
	std::set<std::string> Ids;
	bool CollectionActionSeen = false;
	const auto Finish = [&]()
	{
		if (Document.empty()) return;
		bool HasContent = false;
		for (const auto& Line : Document)
		{
			const std::string Clean = Trim(Line);
			if (!Clean.empty() && Clean[0] != '#') { HasContent = true; break; }
		}
		if (!HasContent) { Document.clear(); return; }
		++Result.Documents;
		try
		{
			for (const auto& Line : Document)
			{
				if (Line.empty() || std::isspace(static_cast<unsigned char>(Line[0]))) continue;
				const std::string Text = RemoveComment(Line);
				const size_t Colon = MappingColon(Text);
				if (Colon != std::string::npos && Unquote(Trim(Text.substr(0, Colon))) == "action")
					CollectionActionSeen = true;
			}
			// Later documents may inherit defaults from a global directive. They
			// cannot be treated as independent even if they look self-contained.
			if (CollectionActionSeen) throw std::runtime_error("rule collection actions are unsupported; dependent documents are not standalone single-event rules");
			FRule Rule = Convert(Document);
			// Validate wire limits as part of loading, so validation and sending agree.
			SerializeRule(Rule);
			if (!Ids.insert(Rule.Id).second) throw std::runtime_error("duplicate rule id");
			Result.Rules.push_back(std::move(Rule));
		}
		catch (const std::exception& Ex)
		{
			Result.Rejected.push_back("document " + std::to_string(Result.Documents) + ": " + Ex.what());
		}
		Document.clear();
	};
	size_t Start = 0;
	while (Start < Utf8Yaml.size())
	{
		const size_t End = Utf8Yaml.find('\n', Start);
		std::string Line = Utf8Yaml.substr(Start, End == std::string::npos ? End : End - Start);
		if (!Line.empty() && Line.back() == '\r') Line.pop_back();
		if (Line == "---" || Line == "...") Finish();
		else Document.push_back(std::move(Line));
		if (End == std::string::npos) break;
		Start = End + 1;
	}
	Finish();
	return Result;
}

bool LoadRulesFile(const std::wstring& Path, FLoadResult& Result, std::string& Error)
{
	FILE* File = nullptr;
	if (_wfopen_s(&File, Path.c_str(), L"rb") != 0 || File == nullptr) { Error = "cannot open YAML file"; return false; }
	if (std::fseek(File, 0, SEEK_END) != 0) { std::fclose(File); Error = "cannot size YAML file"; return false; }
	const long Size = std::ftell(File);
	if (Size < 0 || Size > 64L * 1024 * 1024) { std::fclose(File); Error = "YAML file exceeds 64 MiB"; return false; }
	std::rewind(File);
	std::string Data(static_cast<size_t>(Size), '\0');
	const size_t Read = std::fread(Data.data(), 1, Data.size(), File);
	std::fclose(File);
	if (Read != Data.size()) { Error = "cannot read YAML file"; return false; }
	if (Data.rfind("\xEF\xBB\xBF", 0) == 0) Data.erase(0, 3);
	Result = LoadRules(Data);
	return true;
}

std::vector<uint8_t> SerializeRule(const FRule& Rule)
{
	using namespace Argus::DeviceControl;
	ValidateRule(Rule);
	const auto AlignBytes = [](uint64 Bytes, uint32 Alignment) noexcept -> uint64
	{
		return (Bytes + Alignment - 1u) / Alignment * Alignment;
	};
	FRegisterRuleHeaderRP Header{};
	Header.Version = FRegisterRuleHeaderRP::HeaderVersion;
	Header.HeaderBytes = sizeof(Header);
	Header.RecordRuleCount = 1;
	Header.RecordOperationCount = static_cast<uint32>(Rule.Instructions.size());
	const uint64 RecordOffset = AlignBytes(Header.HeaderBytes, FRegisterRuleRecordRP::RecordAlignment);
	const uint64 RecordStride = AlignBytes(sizeof(FRegisterRuleRecordRP), FRegisterRuleRecordRP::RecordAlignment);
	const uint64 OperationOffset = AlignBytes(RecordOffset + Header.RecordRuleCount * RecordStride,
		FRegisterRuleExpTokenRecordRP::RecordAlignment);
	const uint64 OperationStride = AlignBytes(sizeof(FRegisterRuleExpTokenRecordRP),
		FRegisterRuleExpTokenRecordRP::RecordAlignment);
	FRegisterRuleRecordRP Record{};
	Record.EventType = Rule.EventType;
	Record.ServiceType = Rule.ServiceType;
	Record.PredicateOffset = AlignBytes(OperationOffset + Header.RecordOperationCount * OperationStride,
		static_cast<uint32>(alignof(FRulePredicateRP)));
	Record.PredicateCount = static_cast<uint64>(Rule.Predicates.size());
	const uint64 TablesEnd = Record.PredicateOffset + Record.PredicateCount * sizeof(FRulePredicateRP);
	if (TablesEnd > RuleMaxRequestBytes) throw std::runtime_error("serialized rule exceeds 64 KiB");
	std::vector<uint8_t> Bytes(static_cast<size_t>(TablesEnd), 0);
	Record.RuleId = WriteString(Bytes, Rule.Id);
	Record.RuleIdStatus = EStringStatus::Captured;
	for (size_t Index = 0; Index < Rule.Predicates.size(); ++Index)
	{
		const auto& Predicate = Rule.Predicates[Index];
		FRulePredicateRP Wire{};
		Wire.FieldType = Predicate.FieldType;
		Wire.Value = WriteString(Bytes, Predicate.Value);
		Wire.ValueStatus = EStringStatus::Captured;
		Wire.Match = Predicate.Match;
		Wire.CaseSensitivity = Predicate.CaseSensitivity;
		const size_t Offset = static_cast<size_t>(Record.PredicateOffset) + Index * sizeof(Wire);
		std::memcpy(Bytes.data() + Offset + offsetof(FRulePredicateRP, FieldType), &Wire.FieldType, sizeof(Wire.FieldType));
		std::memcpy(Bytes.data() + Offset + offsetof(FRulePredicateRP, Match), &Wire.Match, sizeof(Wire.Match));
		std::memcpy(Bytes.data() + Offset + offsetof(FRulePredicateRP, CaseSensitivity), &Wire.CaseSensitivity, sizeof(Wire.CaseSensitivity));
		std::memcpy(Bytes.data() + Offset + offsetof(FRulePredicateRP, ValueStatus), &Wire.ValueStatus, sizeof(Wire.ValueStatus));
		std::memcpy(Bytes.data() + Offset + offsetof(FRulePredicateRP, Value), &Wire.Value, sizeof(Wire.Value));
	}
	for (size_t Index = 0; Index < Rule.Instructions.size(); ++Index)
	{
		const FRegisterRuleExpTokenRecordRP& Wire = Rule.Instructions[Index];
		const size_t Offset = static_cast<size_t>(OperationOffset + Index * OperationStride);
		std::memcpy(Bytes.data() + Offset, &Wire, sizeof(Wire));
	}
	if (Bytes.size() > RuleMaxRequestBytes) throw std::runtime_error("serialized rule exceeds 64 KiB");
	Header.TotalBytes = static_cast<uint32>(Bytes.size());
	// Copy only members: compiler padding has no data and stays zero in Bytes.
	const auto WriteMember = [&Bytes](size_t Offset, const auto& Value)
	{
		std::memcpy(Bytes.data() + Offset, &Value, sizeof(Value));
	};
	WriteMember(offsetof(FRegisterRuleHeaderRP, Version), Header.Version);
	WriteMember(offsetof(FRegisterRuleHeaderRP, HeaderBytes), Header.HeaderBytes);
	WriteMember(offsetof(FRegisterRuleHeaderRP, TotalBytes), Header.TotalBytes);
	WriteMember(offsetof(FRegisterRuleHeaderRP, RecordRuleCount), Header.RecordRuleCount);
	WriteMember(offsetof(FRegisterRuleHeaderRP, RecordOperationCount), Header.RecordOperationCount);
	WriteMember(RecordOffset + offsetof(FRegisterRuleRecordRP, RuleIdStatus), Record.RuleIdStatus);
	WriteMember(RecordOffset + offsetof(FRegisterRuleRecordRP, RuleId), Record.RuleId);
	WriteMember(RecordOffset + offsetof(FRegisterRuleRecordRP, EventType), Record.EventType);
	WriteMember(RecordOffset + offsetof(FRegisterRuleRecordRP, ServiceType), Record.ServiceType);
	WriteMember(RecordOffset + offsetof(FRegisterRuleRecordRP, PredicateOffset), Record.PredicateOffset);
	WriteMember(RecordOffset + offsetof(FRegisterRuleRecordRP, PredicateCount), Record.PredicateCount);
	return Bytes;
}
}
