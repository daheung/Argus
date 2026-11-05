#include <Windows.h>
#include <Aclapi.h>
#include <Sddl.h>
#include <ShlObj.h>
#include <newdev.h>
#include <lm.h>
#include <lmerr.h>

#include "Core/Public/Security/ArgusIdentity.h"

#include <cstdio>
#include <cwchar>
#include <string>
#include <vector>

namespace
{
	constexpr auto& AgentServiceName = Argus::Identity::AgentServiceName;
	constexpr wchar_t DriverServiceName[] = L"ArgusKernel";
	constexpr wchar_t AgentFileName[] = L"ArgusAgent.exe";
	constexpr wchar_t DriverFileNames[][20] = {
		L"ArgusKernel.inf", L"ArgusKernel.sys", L"ArgusKernel.cat"
	};

	class FServiceHandle
	{
	public:
		explicit FServiceHandle(SC_HANDLE Handle = nullptr) : Value(Handle) {}
		~FServiceHandle() { if (Value != nullptr) CloseServiceHandle(Value); }
		FServiceHandle(const FServiceHandle&) = delete;
		FServiceHandle& operator=(const FServiceHandle&) = delete;
		SC_HANDLE Get() const { return Value; }
		void Reset(SC_HANDLE NewValue)
		{
			if (Value != nullptr) CloseServiceHandle(Value);
			Value = NewValue;
		}
	private:
		SC_HANDLE Value;
	};

	void Error(const wchar_t* Operation, DWORD Code = GetLastError())
	{
		std::fwprintf(stderr, L"ArgusSetup: %ls failed (Win32 %lu)\n", Operation, Code);
	}

	std::wstring Join(const std::wstring& Directory, const wchar_t* Name)
	{
		return Directory + L"\\" + Name;
	}

	bool KnownFolder(REFKNOWNFOLDERID Id, std::wstring& Result)
	{
		PWSTR Value = nullptr;
		const HRESULT Status = SHGetKnownFolderPath(Id, KF_FLAG_DEFAULT, nullptr, &Value);
		if (FAILED(Status))
		{
			Error(L"SHGetKnownFolderPath", HRESULT_CODE(Status));
			return false;
		}
		Result = Value;
		CoTaskMemFree(Value);
		return true;
	}

	bool ModuleDirectory(std::wstring& Result)
	{
		std::vector<wchar_t> Buffer(MAX_PATH);
		for (;;)
		{
			const DWORD Length = GetModuleFileNameW(nullptr, Buffer.data(), static_cast<DWORD>(Buffer.size()));
			if (Length == 0)
			{
				Error(L"GetModuleFileNameW");
				return false;
			}
			if (Length < Buffer.size())
			{
				Result.assign(Buffer.data(), Length);
				const size_t Slash = Result.find_last_of(L"\\/");
				if (Slash == std::wstring::npos) return false;
				Result.resize(Slash);
				return true;
			}
			Buffer.resize(Buffer.size() * 2);
		}
	}

	bool EnsureDirectory(const std::wstring& Path)
	{
		if (!CreateDirectoryW(Path.c_str(), nullptr) && GetLastError() != ERROR_ALREADY_EXISTS)
		{
			Error(L"CreateDirectoryW");
			return false;
		}
		const DWORD Attributes = GetFileAttributesW(Path.c_str());
		if (Attributes == INVALID_FILE_ATTRIBUTES ||
			(Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ||
			(Attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
		{
			Error(L"directory validation", ERROR_INVALID_REPARSE_DATA);
			return false;
		}
		return true;
	}

	bool Copy(const std::wstring& Source, const std::wstring& Destination)
	{
		for (int Attempt = 0; Attempt < 25; ++Attempt)
		{
			if (CopyFileW(Source.c_str(), Destination.c_str(), FALSE)) return true;
			if (GetLastError() != ERROR_SHARING_VIOLATION) break;
			Sleep(200);
		}
		Error(L"CopyFileW");
		std::fwprintf(stderr, L"  %ls -> %ls\n", Source.c_str(), Destination.c_str());
		return false;
	}

	bool FileExists(const std::wstring& Path)
	{
		const DWORD Attributes = GetFileAttributesW(Path.c_str());
		return Attributes != INVALID_FILE_ATTRIBUTES && (Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
	}

	bool LookupSidString(const wchar_t* AccountName, std::wstring& SidString)
	{
		DWORD SidBytes = 0;
		DWORD DomainChars = 0;
		SID_NAME_USE Use{};
		LookupAccountNameW(nullptr, AccountName, nullptr, &SidBytes,
			nullptr, &DomainChars, &Use);
		if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		{
			Error(L"LookupAccountNameW");
			return false;
		}
		std::vector<BYTE> Sid(SidBytes);
		std::vector<wchar_t> Domain(DomainChars);
		if (!LookupAccountNameW(nullptr, AccountName, Sid.data(), &SidBytes,
			Domain.data(), &DomainChars, &Use))
		{
			Error(L"LookupAccountNameW");
			return false;
		}
		LPWSTR Text = nullptr;
		if (!ConvertSidToStringSidW(Sid.data(), &Text))
		{
			Error(L"ConvertSidToStringSidW");
			return false;
		}
		SidString = Text;
		LocalFree(Text);
		return true;
	}

	bool ServiceSid(std::wstring& SidString)
	{
		if (!LookupSidString(Argus::Identity::AgentServiceAccount, SidString)) return false;
		if (SidString != Argus::Identity::AgentServiceSid)
		{
			Error(L"ArgusAgent service SID mismatch", ERROR_INVALID_SID);
			return false;
		}
		return true;
	}

	bool EnsureRuntimeGroup()
	{
		LPBYTE RawInfo = nullptr;
		NET_API_STATUS Status = NetLocalGroupGetInfo(nullptr,
			Argus::Identity::RuntimeGroupName, 1, &RawInfo);
		if (Status == NERR_GroupNotFound)
		{
			LOCALGROUP_INFO_1 Info{
				const_cast<LPWSTR>(Argus::Identity::RuntimeGroupName),
				const_cast<LPWSTR>(Argus::Identity::RuntimeGroupComment)
			};
			DWORD ParameterError = 0;
			Status = NetLocalGroupAdd(nullptr, 1, reinterpret_cast<LPBYTE>(&Info), &ParameterError);
			if (Status != NERR_Success && Status != NERR_GroupExists)
			{
				Error(L"NetLocalGroupAdd", Status);
				return false;
			}
			Status = NetLocalGroupGetInfo(nullptr, Argus::Identity::RuntimeGroupName, 1, &RawInfo);
		}
		if (Status != NERR_Success || RawInfo == nullptr)
		{
			Error(L"NetLocalGroupGetInfo", Status);
			return false;
		}
		const auto* Info = reinterpret_cast<const LOCALGROUP_INFO_1*>(RawInfo);
		const bool Owned = Info->lgrpi1_comment != nullptr &&
			std::wcscmp(Info->lgrpi1_comment, Argus::Identity::RuntimeGroupComment) == 0;
		NetApiBufferFree(RawInfo);
		if (!Owned)
		{
			Error(L"ArgusAgentRuntime group name collision", ERROR_ALREADY_EXISTS);
			return false;
		}

		std::wstring SidString;
		if (!ServiceSid(SidString)) return false;
		PSID Service = nullptr;
		if (!ConvertStringSidToSidW(SidString.c_str(), &Service))
		{
			Error(L"ConvertStringSidToSidW(ArgusAgent)");
			return false;
		}
		LOCALGROUP_MEMBERS_INFO_0 Member{ Service };
		Status = NetLocalGroupAddMembers(nullptr, Argus::Identity::RuntimeGroupName,
			0, reinterpret_cast<LPBYTE>(&Member), 1);
		LocalFree(Service);
		if (Status != NERR_Success && Status != ERROR_MEMBER_IN_ALIAS)
		{
			Error(L"NetLocalGroupAddMembers", Status);
			return false;
		}
		return true;
	}

	bool RemoveRuntimeGroup()
	{
		LPBYTE RawInfo = nullptr;
		NET_API_STATUS Status = NetLocalGroupGetInfo(nullptr,
			Argus::Identity::RuntimeGroupName, 1, &RawInfo);
		if (Status == NERR_GroupNotFound) return true;
		if (Status != NERR_Success || RawInfo == nullptr)
		{
			Error(L"NetLocalGroupGetInfo", Status);
			return false;
		}
		const auto* Info = reinterpret_cast<const LOCALGROUP_INFO_1*>(RawInfo);
		const bool Owned = Info->lgrpi1_comment != nullptr &&
			std::wcscmp(Info->lgrpi1_comment, Argus::Identity::RuntimeGroupComment) == 0;
		NetApiBufferFree(RawInfo);
		if (!Owned)
		{
			Error(L"ArgusAgentRuntime group name collision", ERROR_ALREADY_EXISTS);
			return false;
		}
		Status = NetLocalGroupDel(nullptr, Argus::Identity::RuntimeGroupName);
		if (Status != NERR_Success && Status != NERR_GroupNotFound)
		{
			Error(L"NetLocalGroupDel", Status);
			return false;
		}
		return true;
	}

	bool ApplyProtectedDacl(const std::wstring& Path, const std::wstring& Sddl)
	{
		PSECURITY_DESCRIPTOR Descriptor = nullptr;
		if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(Sddl.c_str(), SDDL_REVISION_1,
			&Descriptor, nullptr))
		{
			Error(L"ConvertStringSecurityDescriptorToSecurityDescriptorW");
			return false;
		}
		BOOL Present = FALSE;
		BOOL Defaulted = FALSE;
		PACL Dacl = nullptr;
		const bool Valid = GetSecurityDescriptorDacl(Descriptor, &Present, &Dacl, &Defaulted) && Present;
		DWORD Status = ERROR_INVALID_SECURITY_DESCR;
		if (Valid)
		{
			Status = SetNamedSecurityInfoW(const_cast<LPWSTR>(Path.c_str()), SE_FILE_OBJECT,
				DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
				nullptr, nullptr, Dacl, nullptr);
		}
		LocalFree(Descriptor);
		if (Status != ERROR_SUCCESS)
		{
			Error(L"SetNamedSecurityInfoW", Status);
			return false;
		}
		return true;
	}

	bool SetInstallDirectoryAcl(const std::wstring& Directory)
	{
		return ApplyProtectedDacl(Directory,
			L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;GRGX;;;BU)");
	}

	bool SetInstalledFileAcl(const std::wstring& Path)
	{
		return ApplyProtectedDacl(Path,
			L"D:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;GRGX;;;BU)");
	}

	bool SetLogDirectoryAcl(const std::wstring& Directory)
	{
		std::wstring Sid;
		if (!ServiceSid(Sid)) return false;
		const std::wstring DirectorySddl =
			L"D:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;GRGW;;;" + Sid + L")";
		if (!ApplyProtectedDacl(Directory, DirectorySddl)) return false;

		// An existing log may have explicit permissions that override directory inheritance.
		const std::wstring LogFile = Join(Directory, L"Agent.log");
		const DWORD Attributes = GetFileAttributesW(LogFile.c_str());
		if (Attributes == INVALID_FILE_ATTRIBUTES)
		{
			const DWORD Code = GetLastError();
			if (Code == ERROR_FILE_NOT_FOUND || Code == ERROR_PATH_NOT_FOUND) return true;
			Error(L"GetFileAttributesW(Agent.log)", Code);
			return false;
		}
		if ((Attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) != 0)
		{
			Error(L"Agent.log validation", ERROR_INVALID_REPARSE_DATA);
			return false;
		}
		const std::wstring FileSddl =
			L"D:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;GRGW;;;" + Sid + L")";
		return ApplyProtectedDacl(LogFile, FileSddl);
	}

	bool SetAgentServiceAcl(SC_HANDLE Service)
	{
		// Full control for system and administrators; authenticated users may only inspect it.
		constexpr wchar_t Sddl[] =
			L"D:P(A;;0xF01FF;;;SY)(A;;0xF01FF;;;BA)(A;;0x2008D;;;AU)";
		PSECURITY_DESCRIPTOR Descriptor = nullptr;
		if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(Sddl, SDDL_REVISION_1,
			&Descriptor, nullptr))
		{
			Error(L"ConvertStringSecurityDescriptorToSecurityDescriptorW(service)");
			return false;
		}
		const bool Success = SetServiceObjectSecurity(Service, DACL_SECURITY_INFORMATION, Descriptor) != FALSE;
		if (!Success) Error(L"SetServiceObjectSecurity(ArgusAgent)");
		LocalFree(Descriptor);
		return Success;
	}

	bool WaitForState(SC_HANDLE Service, DWORD Wanted)
	{
		for (int Attempt = 0; Attempt < 150; ++Attempt)
		{
			SERVICE_STATUS_PROCESS Status{};
			DWORD Bytes = 0;
			if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO,
				reinterpret_cast<LPBYTE>(&Status), sizeof(Status), &Bytes))
			{
				Error(L"QueryServiceStatusEx");
				return false;
			}
			if (Status.dwCurrentState == Wanted) return true;
			if (Status.dwCurrentState == SERVICE_STOPPED && Wanted != SERVICE_STOPPED)
			{
				Error(L"service startup", Status.dwWin32ExitCode);
				return false;
			}
			Sleep(200);
		}
		Error(L"service state timeout", ERROR_TIMEOUT);
		return false;
	}

	bool Stop(SC_HANDLE Service)
	{
		SERVICE_STATUS_PROCESS Status{};
		DWORD Bytes = 0;
		if (!QueryServiceStatusEx(Service, SC_STATUS_PROCESS_INFO,
			reinterpret_cast<LPBYTE>(&Status), sizeof(Status), &Bytes))
		{
			Error(L"QueryServiceStatusEx");
			return false;
		}
		if (Status.dwCurrentState == SERVICE_STOPPED) return true;
		if (Status.dwCurrentState != SERVICE_STOP_PENDING)
		{
			SERVICE_STATUS Current{};
			if (!ControlService(Service, SERVICE_CONTROL_STOP, &Current))
			{
				Error(L"ControlService(STOP)");
				return false;
			}
		}
		return WaitForState(Service, SERVICE_STOPPED);
	}

	bool Start(SC_HANDLE Service)
	{
		if (!StartServiceW(Service, 0, nullptr) && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
		{
			Error(L"StartServiceW");
			return false;
		}
		return WaitForState(Service, SERVICE_RUNNING);
	}

	bool RemoveDriverService(SC_HANDLE Manager)
	{
		FServiceHandle Driver(OpenServiceW(Manager, DriverServiceName,
			SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE));
		if (Driver.Get() == nullptr)
		{
			const DWORD Code = GetLastError();
			if (Code == ERROR_SERVICE_DOES_NOT_EXIST || Code == ERROR_SERVICE_MARKED_FOR_DELETE) return true;
			Error(L"OpenServiceW(ArgusKernel)", Code);
			return false;
		}
		if (!Stop(Driver.Get())) return false;
		if (!DeleteService(Driver.Get()) && GetLastError() != ERROR_SERVICE_MARKED_FOR_DELETE)
		{
			Error(L"DeleteService(ArgusKernel)");
			return false;
		}
		// SCM finishes deletion only after every handle has been closed.
		Driver.Reset(nullptr);
		return true;
	}

	bool Install()
	{
		std::wstring Source, Programs, Data;
		if (!ModuleDirectory(Source) || !KnownFolder(FOLDERID_ProgramFiles, Programs) ||
			!KnownFolder(FOLDERID_ProgramData, Data)) return false;
		const std::wstring InstallDir = Join(Programs, L"Argus");
		const std::wstring DriverDir = Join(InstallDir, L"Driver");
		const std::wstring LogDir = Join(Data, L"Argus");
		const std::wstring AgentPath = Join(InstallDir, AgentFileName);
		const std::wstring InfPath = Join(DriverDir, DriverFileNames[0]);
		if (!FileExists(Join(Source, AgentFileName)))
		{
			Error(L"Agent payload", ERROR_FILE_NOT_FOUND);
			return false;
		}
		for (const auto& Name : DriverFileNames)
		{
			if (!FileExists(Join(Join(Source, L"Driver"), Name)))
			{
				Error(L"driver payload", ERROR_FILE_NOT_FOUND);
				return false;
			}
		}

		FServiceHandle Manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT | SC_MANAGER_CREATE_SERVICE));
		if (Manager.Get() == nullptr)
		{
			Error(L"OpenSCManagerW");
			return false;
		}
		{
			FServiceHandle Existing(OpenServiceW(Manager.Get(), AgentServiceName,
				SERVICE_STOP | SERVICE_QUERY_STATUS | SERVICE_CHANGE_CONFIG | SERVICE_START));
			if (Existing.Get() != nullptr && !Stop(Existing.Get())) return false;
			if (Existing.Get() == nullptr && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
			{
				Error(L"OpenServiceW(ArgusAgent)");
				return false;
			}
		}
		{
			FServiceHandle Existing(OpenServiceW(Manager.Get(), DriverServiceName,
				SERVICE_STOP | SERVICE_QUERY_STATUS));
			if (Existing.Get() != nullptr && !Stop(Existing.Get())) return false;
			if (Existing.Get() == nullptr && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
			{
				Error(L"OpenServiceW(ArgusKernel)");
				return false;
			}
		}
		if (!EnsureDirectory(InstallDir) || !SetInstallDirectoryAcl(InstallDir) ||
			!EnsureDirectory(DriverDir) || !SetInstallDirectoryAcl(DriverDir) ||
			!Copy(Join(Source, AgentFileName), AgentPath) ||
			!SetInstalledFileAcl(AgentPath)) return false;
		for (const auto& Name : DriverFileNames)
		{
			const std::wstring InstalledPath = Join(DriverDir, Name);
			if (!Copy(Join(Join(Source, L"Driver"), Name), InstalledPath) ||
				!SetInstalledFileAcl(InstalledPath)) return false;
		}

		BOOL NeedReboot = FALSE;
		if (!DiInstallDriverW(nullptr, InfPath.c_str(), 0, &NeedReboot))
		{
			Error(L"DiInstallDriverW");
			return false;
		}
		const std::wstring Command = L"\"" + AgentPath + L"\" --service";
		const wchar_t Dependencies[] = L"ArgusKernel\0";
		FServiceHandle Agent(OpenServiceW(Manager.Get(), AgentServiceName,
			SERVICE_CHANGE_CONFIG | SERVICE_START | SERVICE_QUERY_STATUS | WRITE_DAC));
		if (Agent.Get() == nullptr && GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
		{
			Agent.Reset(CreateServiceW(Manager.Get(), AgentServiceName, L"Argus Agent",
				SERVICE_CHANGE_CONFIG | SERVICE_START | SERVICE_QUERY_STATUS | WRITE_DAC,
				SERVICE_WIN32_OWN_PROCESS, SERVICE_AUTO_START, SERVICE_ERROR_NORMAL,
				Command.c_str(), nullptr, nullptr, Dependencies, L"LocalSystem", nullptr));
		}
		else if (Agent.Get() != nullptr && !ChangeServiceConfigW(Agent.Get(), SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, Command.c_str(), nullptr, nullptr,
			Dependencies, L"LocalSystem", nullptr, L"Argus Agent"))
		{
			Error(L"ChangeServiceConfigW");
			return false;
		}
		if (Agent.Get() == nullptr)
		{
			Error(L"CreateServiceW");
			return false;
		}
		SERVICE_SID_INFO SidInfo{ SERVICE_SID_TYPE_UNRESTRICTED };
		if (!ChangeServiceConfig2W(Agent.Get(), SERVICE_CONFIG_SERVICE_SID_INFO, &SidInfo))
		{
			Error(L"ChangeServiceConfig2W(SID)");
			return false;
		}
		if (!SetAgentServiceAcl(Agent.Get()) || !EnsureRuntimeGroup()) return false;
		if (!EnsureDirectory(LogDir) || !SetLogDirectoryAcl(LogDir)) return false;
		if (NeedReboot)
		{
			std::wprintf(L"Argus installed. Restart Windows before starting the services.\n");
			return true;
		}
		FServiceHandle Driver(OpenServiceW(Manager.Get(), DriverServiceName,
			SERVICE_START | SERVICE_QUERY_STATUS));
		if (Driver.Get() == nullptr || !Start(Driver.Get()) || !Start(Agent.Get()))
		{
			if (Driver.Get() == nullptr) Error(L"OpenServiceW(ArgusKernel)");
			return false;
		}
		std::wprintf(L"Argus installed and running.\n");
		return true;
	}

	bool RemoveFileIfPresent(const std::wstring& Path)
	{
		if (DeleteFileW(Path.c_str()) || GetLastError() == ERROR_FILE_NOT_FOUND) return true;
		Error(L"DeleteFileW");
		std::fwprintf(stderr, L"  %ls\n", Path.c_str());
		return false;
	}

	bool RemoveEmptyDirectory(const std::wstring& Path)
	{
		if (RemoveDirectoryW(Path.c_str())) return true;
		if (GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_FILE_NOT_FOUND) return true;
		Error(L"RemoveDirectoryW");
		std::fwprintf(stderr, L"  %ls\n", Path.c_str());
		return false;
	}

	bool IsMsiOwnedFile(const std::wstring& RelativePath)
	{
		if (_wcsicmp(RelativePath.c_str(), AgentFileName) == 0 ||
			_wcsicmp(RelativePath.c_str(), L"ArgusSetup.exe") == 0) return true;
		for (const auto& Name : DriverFileNames)
		{
			if (_wcsicmp(RelativePath.c_str(), Join(L"Driver", Name).c_str()) == 0) return true;
		}
		return false;
	}

	bool RemoveInstallContents(const std::wstring& Directory,
		const std::wstring& RelativeDirectory, bool PreserveMsiFiles)
	{
		WIN32_FIND_DATAW Entry{};
		const std::wstring Pattern = Join(Directory, L"*");
		HANDLE Search = FindFirstFileW(Pattern.c_str(), &Entry);
		if (Search == INVALID_HANDLE_VALUE)
		{
			const DWORD Code = GetLastError();
			if (Code == ERROR_FILE_NOT_FOUND || Code == ERROR_PATH_NOT_FOUND) return true;
			Error(L"FindFirstFileW", Code);
			std::fwprintf(stderr, L"  %ls\n", Directory.c_str());
			return false;
		}

		bool Success = true;
		do
		{
			if (wcscmp(Entry.cFileName, L".") == 0 || wcscmp(Entry.cFileName, L"..") == 0) continue;
			const std::wstring Path = Join(Directory, Entry.cFileName);
			const std::wstring RelativePath = RelativeDirectory.empty()
				? std::wstring(Entry.cFileName) : Join(RelativeDirectory, Entry.cFileName);
			if (PreserveMsiFiles && IsMsiOwnedFile(RelativePath)) continue;

			if ((Entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
				(Entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0)
			{
				Success = RemoveInstallContents(Path, RelativePath, PreserveMsiFiles) && Success;
			}
			if ((Entry.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0 &&
				!SetFileAttributesW(Path.c_str(), Entry.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY))
			{
				Error(L"SetFileAttributesW");
				std::fwprintf(stderr, L"  %ls\n", Path.c_str());
				Success = false;
				continue;
			}
			if ((Entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				// The MSI removes its own Driver files, then removes this directory.
				if (!(PreserveMsiFiles && _wcsicmp(RelativePath.c_str(), L"Driver") == 0))
				{
					Success = RemoveEmptyDirectory(Path) && Success;
				}
			}
			else
			{
				Success = RemoveFileIfPresent(Path) && Success;
			}
		} while (FindNextFileW(Search, &Entry));
		const DWORD Code = GetLastError();
		FindClose(Search);
		if (Code != ERROR_NO_MORE_FILES)
		{
			Error(L"FindNextFileW", Code);
			Success = false;
		}
		return Success;
	}

	bool ValidateInstallDirectory(const std::wstring& InstallDir)
	{
		std::wstring Programs;
		if (!KnownFolder(FOLDERID_ProgramFiles, Programs)) return false;
		if (_wcsicmp(InstallDir.c_str(), Join(Programs, L"Argus").c_str()) != 0)
		{
			Error(L"Argus install path validation", ERROR_INVALID_NAME);
			return false;
		}
		const DWORD Attributes = GetFileAttributesW(InstallDir.c_str());
		if (Attributes == INVALID_FILE_ATTRIBUTES)
		{
			const DWORD Code = GetLastError();
			if (Code == ERROR_FILE_NOT_FOUND || Code == ERROR_PATH_NOT_FOUND) return true;
			Error(L"GetFileAttributesW(Argus install path)", Code);
			return false;
		}
		if ((Attributes & FILE_ATTRIBUTE_DIRECTORY) == 0 ||
			(Attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
		{
			Error(L"Argus install path validation", ERROR_INVALID_REPARSE_DATA);
			return false;
		}
		return true;
	}

	bool Uninstall()
	{
		std::wstring Programs, Data;
		if (!KnownFolder(FOLDERID_ProgramFiles, Programs) ||
			!KnownFolder(FOLDERID_ProgramData, Data)) return false;
		const std::wstring InstallDir = Join(Programs, L"Argus");
		const std::wstring DriverDir = Join(InstallDir, L"Driver");
		const std::wstring LogDir = Join(Data, L"Argus");
		const std::wstring InfPath = Join(DriverDir, DriverFileNames[0]);
		FServiceHandle Manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
		if (Manager.Get() == nullptr)
		{
			Error(L"OpenSCManagerW");
			return false;
		}
		{
			FServiceHandle Agent(OpenServiceW(Manager.Get(), AgentServiceName,
				SERVICE_STOP | SERVICE_QUERY_STATUS | DELETE));
			if (Agent.Get() != nullptr)
			{
				if (!Stop(Agent.Get()) || !DeleteService(Agent.Get()))
				{
					Error(L"stop/delete ArgusAgent");
					return false;
				}
			}
			else if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
			{
				Error(L"OpenServiceW(ArgusAgent)");
				return false;
			}
		}
		{
			FServiceHandle Driver(OpenServiceW(Manager.Get(), DriverServiceName,
				SERVICE_STOP | SERVICE_QUERY_STATUS));
			if (Driver.Get() != nullptr && !Stop(Driver.Get())) return false;
			if (Driver.Get() == nullptr && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
			{
				Error(L"OpenServiceW(ArgusKernel)");
				return false;
			}
		}
		BOOL NeedReboot = FALSE;
		if (FileExists(InfPath) && !DiUninstallDriverW(nullptr, InfPath.c_str(), 0, &NeedReboot))
		{
			Error(L"DiUninstallDriverW");
			return false;
		}
		if (!RemoveDriverService(Manager.Get()) || !RemoveRuntimeGroup()) return false;
		bool Success = ValidateInstallDirectory(InstallDir);
		if (Success)
		{
			Success = RemoveInstallContents(InstallDir, L"", false);
			Success = RemoveEmptyDirectory(InstallDir) && Success;
		}
		Success = RemoveFileIfPresent(Join(LogDir, L"Agent.log")) && Success;
		Success = RemoveEmptyDirectory(LogDir) && Success;
		if (Success)
		{
			std::wprintf(NeedReboot ? L"Argus removed; restart Windows to finish.\n" : L"Argus removed.\n");
		}
		return Success;
	}

	// Called by the MSI after it has installed the files and ArgusAgent service.
	// The MSI owns those resources; only the INF/driver and machine ACL are managed here.
	bool MsiDriverInstall()
	{
		std::wstring InstallDir, Data;
		if (!ModuleDirectory(InstallDir) || !KnownFolder(FOLDERID_ProgramData, Data)) return false;
		const std::wstring InfPath = Join(Join(InstallDir, L"Driver"), DriverFileNames[0]);
		for (const auto& Name : DriverFileNames)
		{
			if (!FileExists(Join(Join(InstallDir, L"Driver"), Name)))
			{
				Error(L"driver payload", ERROR_FILE_NOT_FOUND);
				return false;
			}
		}
		FServiceHandle Manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
		if (Manager.Get() == nullptr)
		{
			Error(L"OpenSCManagerW");
			return false;
		}
		FServiceHandle Agent(OpenServiceW(Manager.Get(), AgentServiceName, SERVICE_CHANGE_CONFIG));
		if (Agent.Get() == nullptr)
		{
			Error(L"OpenServiceW(ArgusAgent)");
			return false;
		}
		SERVICE_SID_INFO SidInfo{ SERVICE_SID_TYPE_UNRESTRICTED };
		if (!ChangeServiceConfig2W(Agent.Get(), SERVICE_CONFIG_SERVICE_SID_INFO, &SidInfo))
		{
			Error(L"ChangeServiceConfig2W(SID)");
			return false;
		}
		if (!EnsureRuntimeGroup()) return false;
		const std::wstring LogDir = Join(Data, L"Argus");
		if (!EnsureDirectory(LogDir) || !SetLogDirectoryAcl(LogDir)) return false;
		BOOL NeedReboot = FALSE;
		if (!DiInstallDriverW(nullptr, InfPath.c_str(), 0, &NeedReboot))
		{
			Error(L"DiInstallDriverW");
			return false;
		}
		if (NeedReboot)
		{
			std::fwprintf(stderr, L"Driver installation requires a restart.\n");
			return false; // Do not let MSI start an Agent whose driver is unavailable.
		}
		FServiceHandle Driver(OpenServiceW(Manager.Get(), DriverServiceName,
			SERVICE_START | SERVICE_QUERY_STATUS));
		if (Driver.Get() == nullptr)
		{
			Error(L"OpenServiceW(ArgusKernel)");
			return false;
		}
		return Start(Driver.Get());
	}

	// Called before MSI removes its files, after MSI stops the Agent service.
	bool MsiDriverUninstall()
	{
		std::wstring InstallDir;
		if (!ModuleDirectory(InstallDir)) return false;
		const std::wstring InfPath = Join(Join(InstallDir, L"Driver"), DriverFileNames[0]);
		FServiceHandle Manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
		if (Manager.Get() == nullptr)
		{
			Error(L"OpenSCManagerW");
			return false;
		}
		FServiceHandle Driver(OpenServiceW(Manager.Get(), DriverServiceName,
			SERVICE_STOP | SERVICE_QUERY_STATUS));
		if (Driver.Get() != nullptr && !Stop(Driver.Get())) return false;
		if (Driver.Get() == nullptr && GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST)
		{
			Error(L"OpenServiceW(ArgusKernel)");
			return false;
		}
		BOOL NeedReboot = FALSE;
		if (FileExists(InfPath) && !DiUninstallDriverW(nullptr, InfPath.c_str(), 0, &NeedReboot))
		{
			Error(L"DiUninstallDriverW");
			return false;
		}
		Driver.Reset(nullptr);
		if (!RemoveDriverService(Manager.Get()) || !RemoveRuntimeGroup()) return false;
		if (NeedReboot) std::wprintf(L"Driver removal requires a restart.\n");
		return true;
	}

	// Run only for an explicit MSI removal, before MSI removes its own files.
	bool MsiCleanInstallFolder()
	{
		std::wstring InstallDir;
		if (!ModuleDirectory(InstallDir) || !ValidateInstallDirectory(InstallDir)) return false;
		return RemoveInstallContents(InstallDir, L"", true);
	}

	bool TokenHasSid(HANDLE Token, const std::wstring& SidString, BOOL& Present)
	{
		PSID Sid = nullptr;
		if (!ConvertStringSidToSidW(SidString.c_str(), &Sid))
		{
			Error(L"ConvertStringSidToSidW");
			return false;
		}
		const bool Success = CheckTokenMembership(Token, Sid, &Present) != FALSE;
		if (!Success) Error(L"CheckTokenMembership");
		LocalFree(Sid);
		return Success;
	}

	bool ShowIdentity()
	{
		wchar_t GuidText[40]{};
		if (StringFromGUID2(Argus::Identity::ControlDeviceClassGuid,
			GuidText, static_cast<int>(sizeof(GuidText) / sizeof(GuidText[0]))) == 0)
		{
			Error(L"StringFromGUID2", ERROR_INSUFFICIENT_BUFFER);
			return false;
		}
		std::wstring Sid;
		if (!ServiceSid(Sid)) return false;
		std::wprintf(L"Control device class GUID: %ls\n", GuidText);
		std::wprintf(L"Service account: %ls\nService SID: %ls\n",
			Argus::Identity::AgentServiceAccount, Sid.c_str());

		std::wstring GroupSid;
		LPBYTE GroupInfo = nullptr;
		const NET_API_STATUS GroupStatus = NetLocalGroupGetInfo(nullptr,
			Argus::Identity::RuntimeGroupName, 0, &GroupInfo);
		if (GroupStatus == NERR_Success)
		{
			NetApiBufferFree(GroupInfo);
			const std::wstring GroupAccount = std::wstring(L".\\") + Argus::Identity::RuntimeGroupName;
			if (!LookupSidString(GroupAccount.c_str(), GroupSid)) return false;
			std::wprintf(L"Local group: %ls\nLocal group SID: %ls\n",
				Argus::Identity::RuntimeGroupName, GroupSid.c_str());
		}
		else if (GroupStatus == NERR_GroupNotFound)
		{
			std::wprintf(L"Local group: %ls (not installed)\n", Argus::Identity::RuntimeGroupName);
		}
		else
		{
			Error(L"NetLocalGroupGetInfo", GroupStatus);
			return false;
		}

		FServiceHandle Manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT));
		if (Manager.Get() == nullptr)
		{
			Error(L"OpenSCManagerW");
			return false;
		}
		FServiceHandle Agent(OpenServiceW(Manager.Get(), AgentServiceName, SERVICE_QUERY_STATUS));
		if (Agent.Get() == nullptr)
		{
			if (GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
			{
				std::wprintf(L"Agent service: not installed\n");
				return true;
			}
			Error(L"OpenServiceW(ArgusAgent)");
			return false;
		}
		SERVICE_STATUS_PROCESS Status{};
		DWORD Bytes = 0;
		if (!QueryServiceStatusEx(Agent.Get(), SC_STATUS_PROCESS_INFO,
			reinterpret_cast<LPBYTE>(&Status), sizeof(Status), &Bytes))
		{
			Error(L"QueryServiceStatusEx");
			return false;
		}
		if (Status.dwCurrentState != SERVICE_RUNNING || Status.dwProcessId == 0)
		{
			std::wprintf(L"Agent service: not running\n");
			return true;
		}
		HANDLE Process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Status.dwProcessId);
		if (Process == nullptr)
		{
			Error(L"OpenProcess(ArgusAgent)");
			return false;
		}
		HANDLE Token = nullptr;
		const bool OpenedToken = OpenProcessToken(Process, TOKEN_QUERY, &Token) != FALSE;
		CloseHandle(Process);
		if (!OpenedToken)
		{
			Error(L"OpenProcessToken(ArgusAgent)");
			return false;
		}
		BOOL ServicePresent = FALSE, GroupPresent = FALSE;
		const bool Success = TokenHasSid(Token, Sid, ServicePresent) &&
			(GroupSid.empty() || TokenHasSid(Token, GroupSid, GroupPresent));
		CloseHandle(Token);
		if (!Success) return false;
		std::wprintf(L"Service SID in Agent token: %ls\n", ServicePresent ? L"yes" : L"no");
		if (!GroupSid.empty())
			std::wprintf(L"Local group SID in Agent token: %ls\n", GroupPresent ? L"yes" : L"no");
		return true;
	}
}

int wmain(int Argc, wchar_t* Argv[])
{
	if (Argc != 2)
	{
		std::fwprintf(stderr, L"Usage: ArgusSetup.exe install|repair|uninstall|identity|msi-driver-install|msi-driver-uninstall|msi-clean-install-folder\n");
		return 2;
	}
	if (_wcsicmp(Argv[1], L"install") == 0 || _wcsicmp(Argv[1], L"repair") == 0)
	{
		return Install() ? 0 : 1;
	}
	if (_wcsicmp(Argv[1], L"uninstall") == 0)
	{
		return Uninstall() ? 0 : 1;
	}
	if (_wcsicmp(Argv[1], L"identity") == 0) return ShowIdentity() ? 0 : 1;
	if (_wcsicmp(Argv[1], L"msi-driver-install") == 0) return MsiDriverInstall() ? 0 : 1;
	if (_wcsicmp(Argv[1], L"msi-driver-uninstall") == 0) return MsiDriverUninstall() ? 0 : 1;
	if (_wcsicmp(Argv[1], L"msi-clean-install-folder") == 0) return MsiCleanInstallFolder() ? 0 : 1;
	std::fwprintf(stderr, L"Usage: ArgusSetup.exe install|repair|uninstall|identity|msi-driver-install|msi-driver-uninstall|msi-clean-install-folder\n");
	return 2;
}
