# Argus

Windows Kernel-Based Anti-Cheat Solution

## 1. 프로젝트 소개

### 1.1 개요

Argus는 Windows x64 환경에서 동작하는 커널 드라이버와 사용자 모드 Agent로 구성된 안티치트 프로젝트입니다. 커널에서 프로세스, 스레드, 이미지 로드, 레지스트리, 객체 핸들 및 파일 시스템 이벤트를 다루고, Agent가 알림을 조회합니다.

### 1.2 개발 목표

- 보호 대상 프로세스에 대한 접근을 제어하고 관련 이벤트를 수집합니다.
- 서비스 SID와 DACL을 사용해 Agent, 설치 파일 및 로그에 대한 접근을 관리합니다.
- Minifilter, WFP, IOMMU, AI 기반 Zero Trust 기능을 단계적으로 확장합니다. 버전별 목표는 [개발 로드맵](#9-개발-로드맵)에 정리했습니다.

### 1.3 기술 스택

- Platform: Windows x64
- C++: 17
- Build: CMake 3.23 이상, Visual Studio 2022/MSVC, WDK
- Driver: WDM Kernel Driver 및 Filter Manager Minifilter
- Install: WiX Toolset v4 기반 MSI

## 2. 프로젝트 현황

### 2.1 현재 버전

`CMakeLists.txt`의 프로젝트 버전은 **1.0.3**입니다. Agent, 커널 드라이버, INF, MSI의 1.0.3 변경 내용은 [CHANGELOG.md](CHANGELOG.md)에 기록되어 있습니다.

### 2.2 기능별 구현 상태

아래 상태는 현재 소스 코드와 프로젝트 문서에서 확인한 범위입니다. 테스트 VM에서의 동작 검증 결과를 뜻하지 않습니다.

| 기능 | 코드상 상태 |
| --- | --- |
| 프로세스·스레드·이미지 로드 알림 | 커널 콜백과 알림 큐, Agent의 조회 경로 구현 |
| 레지스트리 알림 | 콜백, 알림 큐, 조회 IOCTL 및 Agent 기록 경로 구현. 정책 핸들러는 현재 통과 |
| 프로세스 보호·격리 | 보호/격리 대상 등록과 종료 시 정리, 객체 핸들 접근 권한 조정 및 일부 격리 플래그 처리 구현 |
| 파일 시스템 Minifilter | NTFS 볼륨 관리와 CREATE 요청·완료 정보 수집 구현. 현재 패시브 모드이며 파일 차단 정책은 적용하지 않음 |
| Agent 서비스·설치 | 서비스 실행 경로, MSI와 개발용 설치 도우미 구현 |
| WFP·IOMMU·AI 기반 Zero Trust | 로드맵에 포함된 향후 목표 |

### 2.3 알려진 제한 사항

- Minifilter는 현재 CREATE 이벤트를 패시브 모드로 수집합니다. READ, WRITE 등 등록된 다른 파일 작업은 통과하며, 수집된 CREATE 작업 큐의 Agent 전달 경로는 구현되지 않았습니다.
- 레지스트리 정책 핸들러는 현재 `STATUS_SUCCESS`를 반환합니다. 레지스트리 알림과 차단 정책을 같은 구현 상태로 취급하지 마세요.
- 커널 제어 장치의 SDDL은 현재 `SDDL_DEVOBJ_SYS_ALL`입니다. Agent 등록 시 서비스 SID를 확인하지만, 배포된 Agent 실행 파일의 신원·서명을 확인하는 검증은 아직 없습니다.
- Agent 서비스는 현재 `LocalSystem`으로 실행됩니다. 정식 배포용 파일·실행 파일 서명 검증과 서비스 권한 축소는 후속 작업입니다.
- MSI 자체는 아직 서명되지 않았습니다. 드라이버 설치에 재시작이 필요하면 현재 MSI 설치는 실패하고 롤백됩니다. 설치·제거·업그레이드의 테스트 VM 검증 결과는 이 문서에 기록하지 않았습니다.

## 3. 프로젝트 구조

### 3.1 디렉터리 구조

```text
Argus/
├─ Agent/       사용자 모드 서비스 및 콘솔 실행 파일
├─ Kernel/      커널 드라이버, 콜백, 알림 큐, Minifilter, INF
├─ Core/        공통 자료형·프로토콜과 사용자 모드 정적 라이브러리
├─ Setup/       설치 도우미와 WiX MSI 정의
├─ cmake/       WDK 설정 및 드라이버 패키징 스크립트
├─ Docs/        세부 설계 및 알려진 문제 문서
├─ CMakeLists.txt
└─ CHANGELOG.md
```

### 3.2 모듈별 역할

| 폴더 | CMake 대상 | 역할 |
| --- | --- | --- |
| `Agent` | `ArgusAgent` | 커널 알림 조회, 보호 대상 등록, 로그 기록, Windows 서비스 실행 |
| `Kernel` | `ArgusKernel` | 커널 콜백, 보호·격리 레지스트리, 장치 제어, Minifilter |
| `Core` | `ArgusCore` | Agent가 링크하는 `Core.lib` 및 Agent/Kernel 공통 인터페이스 |
| `Setup` | `ArgusSetup`, `ArgusBundle`, `ArgusMsi` | 설치 도우미, 파일 묶음, MSI 패키지 |

### 3.3 빌드 대상 및 의존성

`ArgusAgent`는 `ArgusCore`를 링크합니다. `ArgusKernel`은 사용자 모드 `Core.lib`를 링크하지 않고, 필요한 `Core/Private` 구현을 커널 도구 집합으로 다시 컴파일합니다. 공통 IOCTL 정의와 프로토콜은 `Core/Public/DeviceControl`에 있습니다. `ArgusKernelPackage`는 빌드된 드라이버와 INF/CAT을 패키징하고 테스트 서명합니다. `ArgusBundle`은 Agent, 설치 도우미, 드라이버 패키지를 모으며, WiX가 발견되면 `ArgusMsi`가 MSI를 만듭니다.

`ArgusCore`는 `Core/Public`과 `Core/Private`의 파일을 사용합니다. `ArgusCore`와 `ArgusKernel`의 include 기준 경로는 프로젝트 루트이므로 `#include "Core/Public/Misc/Defines.h"`처럼 실제 경로를 적습니다. 공유 프로토콜에는 고정 크기 필드와 버전·크기를 명시하고 레이아웃을 검증합니다. Win32·WDK 객체와 포인터는 공통 통신 구조체에 넣지 않습니다.

## 4. 프로젝트 아키텍처

### 4.1 전체 시스템 구성

```text
Windows 이벤트/파일 I/O
        │
        ▼
ArgusKernel.sys ── 콜백·정책·알림 큐·Minifilter
        │
        │ 제어 장치와 IOCTL
        ▼
ArgusAgent.exe ── 보호 대상 등록·알림 조회·로그 기록
        │
        ▼
%ProgramData%\Argus\Agent.log (서비스 실행 시)
```

### 4.2 Kernel / Agent 통신

Agent는 `\\.\Argus` 제어 장치를 열고 등록 IOCTL을 호출합니다. 커널은 등록 요청자의 프로세스 이름과 서비스 SID를 확인하고 등록된 프로세스 참조를 보관합니다. 알림 조회와 프로세스 등록 IOCTL은 등록된 Agent 프로세스를 확인합니다. 알림 조회는 Agent의 폴링 방식이며, 관련 요청·응답 구조체와 IOCTL 코드는 `Core/Public/DeviceControl`에 정의되어 있습니다.

### 4.3 이벤트 수집 및 전달

커널 콜백은 이벤트를 알림 큐에 기록하고 Agent는 프로세스, 스레드, 이미지, 레지스트리, 객체 핸들 알림을 각각 조회합니다. 프로세스·레지스트리·객체 알림에는 배치 헤더와 남은 기록 수를 다루는 경로가 있습니다. Agent는 이 세 알림 큐의 유실 건수를 로그에 반영합니다. Minifilter CREATE 작업은 별도 컨테이너에 저장되며 현재 Agent 조회 경로에는 연결되지 않았습니다.

### 4.4 프로세스 보호 구조

Agent는 프로세스 생성 알림에서 `maruexe.local.exe`를 찾으면 PID와 생성 시각을 사용해 보호 대상 등록 IOCTL을 호출합니다. 비교할 이름은 `Agent/Private/Main.cpp`의 `ProtectedExecutableName`에서 변경할 수 있습니다. 대상의 생성 알림을 받으려면 Agent 서비스를 대상보다 먼저 실행해야 합니다.

커널의 `FProtectedProcessRegistry`와 `FIsolationProcessRegistry`는 각각 보호 대상과 격리 대상 프로세스 참조를 관리하고 종료 알림에서 해제합니다. 객체 콜백은 Agent와 보호 대상의 프로세스 핸들 권한을 조정합니다. 격리 대상에는 설정된 플래그에 따라 자식 프로세스 생성 제한 또는 프로세스 핸들 권한 제한이 적용됩니다.

### 4.5 보안 및 접근 제어 설계

설치 경로·파일의 DACL은 `SYSTEM`과 `Administrators`에 전체 권한, `Users`에 읽기·실행 권한을 줍니다. Agent 서비스 객체는 `SYSTEM`과 `Administrators`에 전체 권한, 인증된 사용자에게 조회 권한만 줍니다. 로그 폴더·파일은 `SYSTEM`, `Administrators`, `NT SERVICE\ArgusAgent`에 필요한 권한을 부여합니다. MSI는 설치 경로와 서비스 DACL을 `MsiLockPermissionsEx`로, 설치 도우미는 로그 DACL을 설정합니다.

설치 도우미는 `ArgusAgentRuntime` 로컬 그룹을 만들고 `NT SERVICE\ArgusAgent`를 멤버로 추가합니다. 그룹의 숫자 SID는 PC가 생성 시 부여하므로 빌드에 고정하지 않습니다. Kernel 장치의 전용 GUID와 Agent 서비스 SID는 `Core/Public/Security/ArgusIdentity.h`에 정의되어 있습니다. 로컬 그룹은 현재 Kernel 인증에는 사용하지 않습니다.

| 식별자 | 값 |
| --- | --- |
| Kernel 제어 장치 전용 GUID | `{923B8B1D-F226-4FD6-A3B2-C6ED24DBF455}` |
| Agent 서비스 SID 이름 | `NT SERVICE\ArgusAgent` |
| Agent 서비스 SID | `S-1-5-80-1711714136-669378850-3648073523-2394855389-1004444190` |
| 로컬 그룹 이름 | `ArgusAgentRuntime` |

현재 장치 SDDL과 배포용 신원 검증의 제한은 [2.3 알려진 제한 사항](#23-알려진-제한-사항)을 참고하세요.

## 5. 주요 기능

### 5.1 프로세스 및 스레드 모니터링

커널은 프로세스 생성 및 종료와 스레드 생성 및 종료 알림을 수집합니다. 프로세스 생성 콜백은 정책 적용 결과와 생성 정보를 알림 큐로 전달하고, 종료 시 등록된 프로세스 상태를 정리합니다. Agent는 조회한 프로세스 이벤트를 기록하고 지정된 실행 파일을 보호 대상으로 등록합니다. 또한, 모니터링에서 수집되는 정보는 모두 paged pool에 할당합니다.

프로세스 모니터링에서 수집되는 정보는 다음과 같습니다:
```C++
struct FProcessNotifyInfo
{
  EProcessNotifyType NotifyType = EProcessNotifyType::None;

  LARGE_INTEGER CreatedAt{};
  LARGE_INTEGER ExitedAt{};

  uint32 ProcessId = 0;
  uint32 ParentProcessId = 0;
  uint32 CreatingThreadId = 0;
  uint32 CreatingProcessId = 0;

  FString<> CommandLine;
  FString<> ImageFileName;

  uint32 ExitCode = 0;
  uint32 CreationStatus = STATUS_SUCCESS;

  EPolicyRole PolicyRole = EPolicyRole::None;
  EPolicyStatus PolicyStatus = EPolicyStatus::AccessPass;
};
```
수집되는 모든 프로세스들은 `Role`을 부여받습니다. `Role`은 Agent가 IOCTL을 통해 등록할 수 있으며 `PolicyRole`에 저장합니다. 각각 `None(정책을 적용받지 않는 프로세스)`와 `AgentProcess(Agent 프로세스)`, `ProtectedProcess(Agent가 보호 대상으로 등록한 프로세스)`, `IsolatedProcess(Agent가 격리 대상으로 등록한 프로세스)`가 존재합니다. Callback System은 이를 참고하여 프로세스마다 `Role`을 부여하고 이후 모니터링 시스템에서 기록합니다. 

프로세스에 대한 이벤트가 들어오면 Callback System에서 먼저 프로세스의 `Role`을 확인합니다. 이후 정책을 적용하고 이를 `PolicyStatus`에 기록합니다. 이후 Notify System에 이를 전달하고 기록합니다.

스레드 모니터링에서 수집되는 정보는 다음과 같습니다:
```C++
struct FThreadNotifyInfo
{
  EThreadNotifyType NotifyType = EThreadNotifyType::Unknown;

  LARGE_INTEGER CreatedAt{};
  LARGE_INTEGER ExitedAt{};

  uint32 ProcessId = 0;
  uint32 ThreadId = 0;
  uint32 ExitCode = 0;
};
```

### 5.2 이미지 로드 모니터링

커널은 이미지 로드 콜백에서 이미지 경로와 프로세스 정보를 수집해 큐에 저장합니다. 수집된 정보는 모두 paged pool에 할당되며, Agent에는 이미지 알림 조회 IOCTL 경로가 있습니다.

이미지 로드 모니터링에서 수집되는 정보는 다음과 같습니다:
```C++
struct FImageNotifyInfo
{
  LARGE_INTEGER LoadedAt{};
  uint32 ProcessId = 0;
  uint64 ImageSize = 0;
  uint64 ImageBaseAddress = 0;
  uint32 ImageFileNameChars = 0;
  PWCHAR ImageFileName = nullptr;
};
```

### 5.3 레지스트리 모니터링

커널은 레지스트리 콜백을 등록하고 요청 전·후 정보를 알림 큐에 기록합니다. Agent는 레지스트리 알림을 조회해 로그에 남깁니다. 생성·설정·삭제 등에 대한 정책 확장 지점은 있으나 현재 정책 함수는 통과 상태입니다.

### 5.4 프로세스 보호 및 격리

객체 콜백은 프로세스·스레드 핸들 생성과 복제 요청을 관찰합니다. Agent와 보호 대상에 대한 특정 프로세스 핸들 권한을 제거하고, 격리 대상에 대해서는 설정된 플래그에 따라 권한을 제한합니다. 격리 대상의 자식 프로세스 생성을 거부하는 플래그도 처리합니다. 보호·격리 대상 등록은 Agent IOCTL로 수행합니다.

### 5.5 파일 시스템 Minifilter

`Minifilter`는 NTFS 볼륨에 연결하고 볼륨 GUID, 장치 경로, 파일 시스템 속성과 드라이브 루트를 관리합니다. Minifilter는 정책을 정의하는 `Mode`가 존재하며 크게 4가지 종류가 존재합니다. 각각 `Pass(I/O 정보를 기록하지 않고 통과시킴)`, `Passive(I/O 정보를 기록하고 통과시킴)`, `Active(I/O 정보를 기록하고 필요에 따라 수정 및 차단)`, `None(FltMgr에 등록되지 않음)`이 존재하며 기본값은 `Passive`입니다. 

`Mode`는 실시간으로 변경될 수 있으며, `Mode`를 `None`으로 변경하는 동작은 허용되지 않습니다. Minifilter는 I/O가 감지되면 현재 `Mode`를 확인하고 I/O를 처리합니다. Pre-operation에서 `Mode`를 확인하면 Post-operation에서도 동일한 기준을 따릅니다.

...
### 5.5.1 Volume 정보 저장

`FMinifilter`는 `IRP_MJ_SET_VOLUME_INFORMATION`이 발생했을 때 Post-operation에서 Work Item을 등록하고 드라이브 루트 정보를 갱신하며, Object Manager에서 DOS 장치 심볼릭 링크를 조회하며 찾지 못하면 `Unavailable`로 기록합니다. 

### 5.5.2 Operation 정보 저장
...

`FMinifilter`는 I/O 요청 종류에 따라 다른 Container를 사용하며, `FMinifilterContainer`는 요청 종류마다 Container를 소유합니다. Create에서 사용하는 Container에 접근하기 위해 `FMinifilterContainer::Get<Create>()` 함수를 사용합니다.
```C++
namespace Argus::Minifilter
{
  enum class EOperation : uint8
  {
    Create = IRP_MJ_CREATE,
    ...
  };
}

namespace Argus::Minifilter::Private
{
  template <EOperation Operation>
  struct TOperationContainerType;

  template <> struct TOperationContainerType<EOperation::Create> { using Type = FCreateOperationContainer; };
  ...
}

class FMinifilterContainer
{
public:
  template <Argus::Minifilter::EOperation Operation>
  NODISCARD auto& Get() noexcept;

private:
  ...
};
```

I/O 요청 종류마다 다른 Container를 사용하려면 개발자는 EOperation에 새로운 열거값을 생성하고 `Template meta-programming`을 통해 `FMinifilterContainer::Get()`함수에 대한 특수화를 지정하여야 합니다.

특수화를 지정하는 예시는 다음과 같습니다:
```C++
template <>
FORCEINLINE auto& FMinifilterContainer::Get<Argus::Minifilter::EOperation::Create>() noexcept
{
  return this->CreateOperationContainer;
}
```

이러한 설계는 불필요한 캐스팅을 줄이면서 Operation 종류가 늘어나도 함수 하나로 단순하게 호출하기 위함입니다. 또한, Container 선택 경로를 컴파일 시간에 결정하여 런타임을 감소시키는 최적화 과정입니다. 

### 5.5.1 Create
```C++
struct FCreateOperationDescriptor
{
  uint32 RequestorPid = 0;
  uint32 RequestorTid = 0;
  EProcessorMode ProcessorMode = EProcessorMode::Unknown;

  LARGE_INTEGER ObservedAt{};

  uint8 OperationFlags = 0;
  uint16 SharedAccess = 0;
  uint16 Attribute = 0;

  uint32 Disposition = 0;
  ACCESS_MASK DesiredAccess = 0;

  FCreatePreOperationDecision PreOperationDecision{};
  FCreatePostOperationDecision PostOperationDecision{};

  EOperationStringStatus VolumeNameStatus = EOperationStringStatus::NotApplicable;
  EOperationStringStatus FilePathNameStatus = EOperationStringStatus::NotApplicable;

  FString<> VolumeName{};
  FString<> FilePathName{};

  EOperationStringStatus CompletionVolumeNameStatus = EOperationStringStatus::NotApplicable;
  EOperationStringStatus CompletionFilePathNameStatus = EOperationStringStatus::NotApplicable;

  FString<> CompletionVolumeName{};
  FString<> CompletionFilePathName{};

  uint32 InternalAttemptedCount = 0;
  uint64 CompletionInformation = 0;
  NTSTATUS CompletionStatus = STATUS_UNSUCCESSFUL;

  ...
};
```

### 5.6 Agent 서비스 및 설치 관리

Agent는 서비스 모드에서 중지·시스템 종료 요청을 받으면 폴링을 멈추고 장치 핸들을 닫습니다. MSI는 Agent 서비스를 생성·자동 시작하고 드라이버 INF 설치 도우미를 실행합니다. 개발용 `ArgusSetup.exe`는 별도의 설치·복구·제거 명령을 제공합니다. 두 설치 방식은 상태를 공유하지 않으므로 같은 테스트 VM에서는 한 방식만 사용합니다.

## 6. 빌드

### 6.1 요구 사항

Visual Studio 2022, MSVC, Windows Driver Kit과 CMake 3.23 이상이 필요합니다. `ArgusKernel`은 `WindowsKernelModeDriver10.0` 도구 집합을, `ArgusCore`와 `ArgusAgent`는 Visual Studio 2022 도구 집합을 사용합니다. 현재 `CMakePresets.json`은 Visual Studio Community 경로를 지정합니다. 다른 설치를 사용하면 `CMAKE_GENERATOR_INSTANCE`를 수정하세요. CMake는 설치된 WDK 중 가장 높은 버전을 선택하며, 필요하면 `ARGUS_WDK_ROOT`와 `ARGUS_WDK_VERSION`을 지정할 수 있습니다.

MSI 빌드에는 WiX Toolset v4의 `wix.exe`가 필요합니다. `wix.exe`가 있으면 일반 Release 빌드에 `ArgusMsi`가 포함됩니다.

### 6.2 CMake 빌드

```powershell
cmake --preset windows-msvc-x64
cmake --build --preset debug
```

MSI를 만들려면 WiX 4.0.5를 설치한 뒤 CMake를 다시 구성합니다.

```powershell
dotnet tool install wix --version 4.0.5 --tool-path .build/wix4-tools
cmake --preset windows-msvc-x64
cmake --build --preset release
```

### 6.3 빌드 결과

빌드 결과는 `.build/windows-msvc-x64` 아래에 생성됩니다.

| 결과물 | 경로 |
| --- | --- |
| Debug `Core.lib` | `.build/windows-msvc-x64/Core/Debug/Core.lib` |
| Debug `ArgusKernel.sys` | `.build/windows-msvc-x64/Kernel/Debug/ArgusKernel.sys` |
| Release 번들 | `.build/windows-msvc-x64/Bundle/Release` |
| Release MSI (WiX 설치 시) | `.build/windows-msvc-x64/Msi/Release/Argus-1.0.3-x64.msi` |

## 7. 설치 및 실행

### 7.1 테스트 환경 준비

현재 드라이버 패키지는 테스트 인증서와 테스트 altitude를 사용합니다. 테스트 VM에서 `ArgusTest.cer`를 신뢰하도록 설정해야 드라이버를 로드할 수 있습니다. MSI와 개발용 설치 도우미는 인증서를 자동 등록하지 않습니다. 정식 배포용 서명·설치 절차는 아직 준비되지 않았습니다.

### 7.2 MSI 설치

테스트 VM 준비 후 `.build/windows-msvc-x64/Msi/Release/Argus-1.0.3-x64.msi`를 실행합니다. MSI는 `%ProgramFiles%\Argus`에 Agent, 설치 도우미, 드라이버 패키지를 설치하고 `ArgusAgent` 서비스를 생성·자동 시작합니다. `ArgusSetup.exe msi-driver-install`을 통해 드라이버 INF를 등록하고, 서비스 SID 유형과 설치·서비스·로그 DACL을 설정합니다.

개발용 설치 도우미를 사용하려면 다음처럼 번들을 빌드한 후 관리자 권한으로 `ArgusSetup.exe install` 또는 `repair`를 실행합니다.

```powershell
cmake --preset windows-msvc-x64
cmake --build --preset release --target ArgusBundle
```

번들 경로는 `.build/windows-msvc-x64/Bundle/Release`입니다. 설치 도우미만 독립적으로 빌드할 때는 `cmake -S Setup -B .build/setup-standalone -A x64 -G "Visual Studio 17 2022"` 다음 `cmake --build .build/setup-standalone --config Release`를 실행합니다. 독립 빌드 결과 옆에는 `ArgusAgent.exe`와 `Driver/ArgusKernel.inf`, `.sys`, `.cat`을 함께 배치해야 합니다.

### 7.3 Agent 실행

MSI 설치 시 Agent 서비스가 자동 시작됩니다. Agent에는 `ArgusAgent.exe` 콘솔 진입점과 `ArgusAgent.exe --service` 서비스 진입점이 있습니다. 커널의 Agent 등록은 서비스 SID가 포함된 토큰을 요구하므로 일반 콘솔 실행만으로 등록이 성공하지는 않습니다.

### 7.4 로그 확인

서비스 로그는 `%ProgramData%\Argus\Agent.log`에 저장됩니다. 실행 중인 로그는 관리자 PowerShell에서 확인할 수 있습니다.

```powershell
Get-Content "$env:ProgramData\Argus\Agent.log" -Tail 50 -Wait
```

### 7.5 제거

MSI는 Windows **설치된 앱 → Argus → 제거**에서 제거합니다. 이 경로는 Agent 서비스를 중지·삭제하고 `ArgusSetup.exe msi-driver-uninstall`로 드라이버 INF 및 서비스를 정리합니다. 설치 파일과 설치 폴더는 제거하지만 `%ProgramData%\Argus`의 로그 폴더와 로그 파일은 남깁니다.

개발용 설치 도우미로 설치했다면 관리자 권한으로 번들의 `ArgusSetup.exe uninstall`을 실행합니다. 개발용 제거 경로는 Agent를 중지하고 드라이버를 언로드한 뒤 설치 폴더와 로그 파일·폴더를 제거합니다. MSI와 개발용 설치 도우미의 제거 명령을 섞어 사용하지 마세요.

## 8. 검증

### 8.1 기능 검증

작성 예정.

### 8.2 보안 설정 검증

작성 예정.

### 8.3 안정성 및 스트레스 테스트

작성 예정.

## 9. 개발 로드맵

다음 표는 구현 완료 기록이 아닌 버전별 개발 목표입니다.

| 버전 구간 | 목표 |
| --- | --- |
| `0.0.0` ~ `1.0.0` | 보안 설정, 서비스 SID와 DACL 설정, 설치 및 제거 기능 완성 |
| `1.0.0` ~ `2.0.0` | Minifilter 기능 구현 및 적용 |
| `2.0.0` ~ `3.0.0` | WFP 기능 구현 및 적용 |
| `3.0.0` ~ `4.0.0` | IOMMU 기반 기능 구현 및 적용 |
| `4.0.0` ~ `5.0.0` | AI 기반 Zero Trust 기능 구현 및 적용 |

## 10. 코딩 규칙

[Epic C++ Coding Standard](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)를 따릅니다. 중괄호는 새 줄에 두고, 들여쓰기는 폭 4의 탭을 사용합니다. 타입에는 `F`/`E`/`T` 등 UE 접두사를 사용하고, 불리언 변수는 `b`로 시작합니다. 이 프로젝트는 Unreal Engine 자체에는 의존하지 않으므로 `U`/`A` 접두사와 UE 리플렉션 매크로는 해당 UE 타입을 도입할 때만 사용합니다.

## 11. 관련 문서 및 변경 이력

- [CHANGELOG.md](CHANGELOG.md): 버전별 변경 사항. 버전을 올릴 때 해당 버전의 항목을 추가합니다.
- [RegistryCallbackDraft.md](Docs/RegistryCallbackDraft.md): 레지스트리 콜백 설계 초안. 구현 현황과 차이가 있을 수 있습니다.
- [FKrnlString_KnownIssues.md](Docs/FKrnlString_KnownIssues.md): `FKrnlString` 관련 알려진 문제.
