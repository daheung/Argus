# Argus

Windows x64 / MSVC / C++17 기반 프로젝트입니다.

버전별 변경사항은 [변경 이력](CHANGELOG.md)에 기록합니다. 버전을 올릴 때 해당 버전의 항목을 추가합니다.

| 폴더 | CMake 대상 | 역할 |
| --- | --- | --- |
| `Agent` | `ArgusAgent` | 사용자 모드 실행 파일 |
| `Kernel` | `ArgusKernel` | WDM 커널 드라이버 (`.sys`) |
| `Core` | `ArgusCore` | Agent가 사용하는 사용자 모드 `Core.lib` |

## 빌드

Visual Studio 2022와 WDK가 필요합니다. `ArgusKernel`은 `WindowsKernelModeDriver10.0` 도구 집합을 사용하고, `ArgusCore`와 `ArgusAgent`는 `v143`을 사용합니다. 이 PC의 Build Tools 설치에는 WDK 도구 집합이 없어 CMake 프리셋은 Community 설치를 지정합니다. 다른 Visual Studio 에디션을 사용한다면 `CMakePresets.json`의 `CMAKE_GENERATOR_INSTANCE` 경로를 해당 설치 경로로 변경하세요. CMake는 설치된 WDK 중 가장 높은 버전을 선택합니다. 다른 WDK 설치 경로 또는 버전을 사용하려면 `ARGUS_WDK_ROOT`, `ARGUS_WDK_VERSION` 캐시 변수를 지정하세요.

```powershell
cmake --preset windows-msvc-x64
cmake --build --preset debug
```

빌드 결과는 `.build/windows-msvc-x64` 아래에 생성됩니다. 현재 드라이버는 프로세스·스레드·이미지 알림과 Agent 및 보호 대상 프로세스 등록 IOCTL을 처리합니다. `FAgentState`는 등록된 Agent의 프로세스 참조와 버전을 보관합니다. `FProtectedProcessRegistry`와 `FIsolationProcessRegistry`는 각각 보호 대상과 격리 대상의 프로세스 참조를 관리하고, 종료 알림에서 이를 해제합니다. `FObjectCallback`은 Agent와 보호 대상 프로세스의 종료 권한을 제거합니다. 격리 정책은 아직 콜백에 연결되지 않았습니다. Minifilter는 등록되어 있지만 파일 I/O를 통과시킵니다.

Agent는 프로세스 생성 알림에 포함된 실행 파일 이름을 확인합니다. `maruexe.local.exe`와 일치하면 해당 PID와 생성 시각으로 보호 대상 등록 IOCTL을 호출합니다. 비교할 이름은 `Agent/Private/Main.cpp`의 `ProtectedExecutableName` 상수에서 바꿀 수 있습니다. 대상 프로세스의 생성 알림을 안정적으로 받으려면 Agent를 대상보다 먼저 실행하세요.

```powershell
ArgusAgent.exe
```

개발용 콘솔 실행은 그대로 지원합니다. 서비스는 `--service`로 실행되며, `SERVICE_CONTROL_STOP` 또는 시스템 종료 요청을 받으면 폴링을 멈추고 장치 핸들을 닫습니다. 로그는 `%ProgramData%\Argus\Agent.log`에 기록합니다. 실행 중인 서비스 로그는 관리자 PowerShell에서 `Get-Content "$env:ProgramData\Argus\Agent.log" -Tail 50 -Wait`로 확인할 수 있습니다.

## MSI 설치 프로그램 (테스트용)

WiX Toolset v4의 `wix.exe`를 설치하고 CMake를 다시 구성하면 일반 빌드에 `ArgusMsi`가 포함됩니다. 빌드 시 `ArgusBundle`이 Agent, 설치 도우미, 서명된 드라이버 패키지를 모은 뒤 WiX로 단일 MSI를 만듭니다. 아래 명령은 WiX 4.0.5를 작업 공간에 설치합니다.

```powershell
dotnet tool install wix --version 4.0.5 --tool-path .build/wix4-tools
cmake --preset windows-msvc-x64
cmake --build --preset release
```

테스트 VM 준비가 끝나면 `.build/windows-msvc-x64/Msi/Release/Argus-1.0.3-x64.msi`를 더블클릭해 설치합니다. 제거는 Windows **설치된 앱 → Argus → 제거**에서 진행합니다. 설치·제거 확인 명령은 선택 사항이며, 정상 사용에 필요하지 않습니다.

MSI는 `%ProgramFiles%\Argus` 아래의 Agent/도우미/드라이버 패키지 파일을 설치하고, `ArgusAgent` 서비스를 생성·자동 시작하며, Windows의 **설치된 앱**에 Argus를 등록합니다. 제거하면 MSI가 Agent 서비스를 중지·삭제하고 설치 파일을 제거합니다. MSI가 호출하는 `ArgusSetup.exe msi-driver-install`과 `msi-driver-uninstall`은 드라이버 INF 등록·제거를 담당합니다. 설치 시 도우미는 `ArgusAgent`의 서비스 SID 유형을 `UNRESTRICTED`로 설정하고 `%ProgramData%\Argus` 로그 폴더와 기존 `Agent.log`에 DACL을 적용합니다. 제거 시 `ArgusKernel` 서비스를 별도로 삭제합니다. MSI 제거 도우미는 설치 경로의 `.vs`를 포함한 추가 파일과 하위 폴더를 먼저 정리하고, MSI가 관리하는 파일과 빈 설치 폴더는 MSI가 제거합니다. 로그 폴더와 로그 파일은 제거 후에도 남습니다.

설치 경로와 파일의 SDDL은 `SYSTEM`·`Administrators`에 전체 권한, `Users`에 읽기·실행 권한을 줍니다. Agent 서비스 객체의 SDDL은 `SYSTEM`·`Administrators`에 전체 권한, 인증된 사용자에게 서비스 조회 권한만 줍니다. 로그 폴더와 파일은 `SYSTEM`·`Administrators`에 전체 권한, `NT SERVICE\ArgusAgent`에 읽기·쓰기 권한을 주며, 다른 사용자에게는 권한을 주지 않습니다. 설치 경로와 서비스 DACL은 MSI의 `MsiLockPermissionsEx`로 설정하고, 로그 DACL은 설치 도우미가 설정합니다.

설치 도우미는 로컬 그룹 `ArgusAgentRuntime`을 만들고 `NT SERVICE\ArgusAgent`를 멤버로 추가합니다. MSI 제거 또는 개발용 설치 도우미 제거 시 그룹도 삭제합니다. Kernel 장치용 전용 GUID와 서비스 SID는 `Core/Public/Security/ArgusIdentity.h`에 고정했습니다.

| 식별자 | 값 |
| --- | --- |
| Kernel 제어 장치 전용 GUID | `{923B8B1D-F226-4FD6-A3B2-C6ED24DBF455}` |
| Agent 서비스 계정 | `NT SERVICE\ArgusAgent` |
| Agent 서비스 SID | `S-1-5-80-1711714136-669378850-3648073523-2394855389-1004444190` |
| 로컬 그룹 이름 | `ArgusAgentRuntime` |

로컬 그룹의 **숫자 SID는 PC에서 그룹을 생성할 때 Windows가 부여**하므로 빌드에 고정하지 않습니다. 설치 후 관리자 PowerShell에서 `& "$env:ProgramFiles\Argus\ArgusSetup.exe" identity`를 실행하면 그룹 SID와 실행 중인 Agent 토큰에 서비스 SID·그룹 SID가 실제로 포함됐는지 표시합니다. Kernel은 전용 GUID를 `IoCreateDeviceSecure`의 장치 클래스 GUID로 사용하고, 장치 SDDL은 현재 `SDDL_DEVOBJ_SYS_ALL`로 설정합니다. Agent 등록 시에는 고정된 서비스 SID를 검사하며, 알림 조회와 프로세스 등록 IOCTL은 등록된 Agent 프로세스를 검사합니다. 로컬 그룹은 생성되지만 현재 Kernel 인증에는 사용하지 않습니다.

테스트 VM에서 설치 후 관리자 PowerShell로 권한을 확인할 수 있습니다.

```powershell
sc.exe qsidtype ArgusAgent
sc.exe sdshow ArgusAgent
icacls "$env:ProgramFiles\Argus"
icacls "$env:ProgramFiles\Argus\ArgusAgent.exe"
icacls "$env:ProgramData\Argus"
icacls "$env:ProgramData\Argus\Agent.log"
```

`qsidtype`은 `UNRESTRICTED`여야 합니다. 일반 사용자에게 Agent 서비스 중지·설정 변경과 설치 파일 쓰기 권한이 없어야 합니다. 서비스 시작 후 `Agent.log`가 만들어졌다면 로그 파일 ACL도 확인합니다.

MSI를 실행할 테스트 VM에는 `ArgusTest.cer` 신뢰 설정이 먼저 필요합니다. 이 MSI는 인증서를 설치하지 않으며 MSI 자체도 아직 서명하지 않았습니다. 드라이버 설치 과정에서 재시작이 필요하면 현재 MSI 설치는 실패하고 롤백됩니다. 실제 설치·제거와 업그레이드는 테스트 VM에서 검증해야 합니다.

### 기존 개발용 설치 도우미

이 명령은 MSI와 설치 상태를 공유하지 않습니다. 한 테스트 VM에서는 둘 중 한 방식만 사용하세요.

`ArgusSetup`은 Agent/Kernel과 별도 CMake 대상입니다. 전체 프로젝트에서는 아래처럼 서명된 테스트 드라이버, Agent, 설치 프로그램을 한 폴더로 묶습니다.

```powershell
cmake --preset windows-msvc-x64
cmake --build --preset release --target ArgusBundle
```

결과는 `.build/windows-msvc-x64/Bundle/Release`에 있습니다. 관리자 권한으로 이 폴더의 `ArgusSetup.exe install`, `ArgusSetup.exe repair`, `ArgusSetup.exe uninstall`을 실행합니다. 설치 프로그램은 Agent와 드라이버 패키지를 `%ProgramFiles%\Argus`에 복사하고, 드라이버 패키지를 설치한 다음 `ArgusAgent` 서비스를 자동 시작으로 구성합니다. 서비스 SID를 활성화하고 설치 경로·파일, 서비스 객체, `%ProgramData%\Argus` 로그 폴더·파일에 위 DACL을 적용합니다. 제거 시 Agent를 먼저 중지하고 드라이버를 언로드한 뒤 `%ProgramFiles%\Argus` 폴더 전체를 제거합니다. 번들에 포함된 `ArgusTest.cer`는 테스트 VM에서만 신뢰하도록 등록해야 하며 설치 프로그램은 인증서를 자동 등록하지 않습니다.

설치 프로그램만 독립적으로 빌드할 수도 있습니다: `cmake -S Setup -B .build/setup-standalone -A x64 -G "Visual Studio 17 2022"` 다음 `cmake --build .build/setup-standalone --config Release`. 독립 빌드 결과를 실행하려면 같은 폴더에 `ArgusAgent.exe`와 `Driver/ArgusKernel.inf`, `.sys`, `.cat`을 함께 배치해야 합니다.

현재 패키지는 테스트 인증서와 테스트 altitude를 사용합니다. 테스트 VM에 인증서를 신뢰하도록 등록해야 드라이버를 로드할 수 있습니다. Kernel의 장치 DACL과 최초 Agent 등록자의 서비스 SID 검증은 아직 구현되지 않았으므로, 설치 프로그램은 현행 장치 접근과 호환되도록 Agent를 `LocalSystem`으로 실행합니다. 이 보안 변경을 완료한 뒤 서비스 계정을 `LocalService`로 낮추고 장치 DACL을 서비스 SID로 제한해야 합니다. 설치 파일과 실행 파일의 배포 서명 검증도 정식 배포 전에 추가해야 합니다.

Agent 등록은 이미 등록된 프로세스가 살아 있는 동안 다른 프로세스로 교체할 수 없습니다. 최초 등록자의 신원 검증은 아직 구현되지 않았습니다. 일반 사용자로 실행하는 Agent의 최종 설치 경로가 정해지면 보호된 실행 파일과 등록 호출자를 연결하는 검증을 추가해야 합니다.

| 라이브러리 | Debug 출력 경로 |
| --- | --- |
| `Core.lib` | `.build/windows-msvc-x64/Core/Debug/Core.lib` |
| `ArgusKernel.sys` | `.build/windows-msvc-x64/Kernel/Debug/ArgusKernel.sys` |

## Core 모듈 범위

`ArgusCore`는 `Core/Public`과 `Core/Private`의 파일만 등록합니다. `Public`은 사용자와 커널 대상에 공개되고 `Private`는 `ArgusCore` 내부에서만 포함됩니다. 현재 `Math`, `Memory`, `Misc`, `Templates`, `Logging` 헤더가 등록되어 있습니다. 통신을 구현할 때 명령/IOCTL 번호, 고정 크기 요청·응답 구조체, 공통 열거형과 상태 코드를 추가할 수 있습니다. 공유 구조체에는 버전과 크기를 명시하고 `static_assert`로 레이아웃을 검증합니다.

`ArgusCore`와 `ArgusKernel`의 include 기준 경로는 프로젝트 루트입니다. 따라서 `#include "Core/Public/Misc/Defines.h"` 또는 `#include "Kernel/Public/Misc/KernelDefines.h"`처럼 실제 폴더 경로를 사용합니다.

`ArgusKernel`은 커널에서 사용할 공통 Core 구현 파일을 `/kernel`로 다시 컴파일하고 `Kernel/Public`, `Kernel/Private`의 커널 전용 파일과 함께 `ArgusKernel.sys`에 넣습니다. 커널 드라이버는 사용자 모드로 빌드된 `Core.lib`를 링크하지 않습니다. 현재 커널용 공통 구현 목록은 `Core.cpp`, `Assertion.cpp`, `Stricmp.cpp`입니다. CRT 함수에 의존하는 `CString.cpp`와 `Math.cpp`는 커널용 구현을 준비하기 전까지 `Core.lib`에만 포함됩니다. Win32·WDK 객체와 포인터가 포함된 통신 구조체는 공통 인터페이스에 두지 않습니다.

## 코딩 규칙

[Epic C++ Coding Standard](https://dev.epicgames.com/documentation/unreal-engine/epic-cplusplus-coding-standard-for-unreal-engine)를 따릅니다. 중괄호는 새 줄에 두고, 들여쓰기는 폭 4의 탭을 사용합니다. 타입에는 `F`/`E`/`T` 등 UE 접두사를 사용하고, 불리언 변수는 `b`로 시작합니다. 이 프로젝트는 Unreal Engine 자체에는 의존하지 않으므로 `U`/`A` 접두사와 UE 리플렉션 매크로는 해당 UE 타입을 도입할 때만 사용합니다.
