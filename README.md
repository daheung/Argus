# Argus

Windows x64 / MSVC / C++17 기반 프로젝트입니다.

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

Agent는 프로세스 생성 알림에 포함된 실행 파일 이름을 확인합니다. `hanjamaru.local.exe`와 일치하면 해당 PID와 생성 시각으로 보호 대상 등록 IOCTL을 호출합니다. 비교할 이름은 `Agent/Private/Main.cpp`의 `ProtectedExecutableName` 상수에서 바꿀 수 있습니다. 대상 프로세스의 생성 알림을 안정적으로 받으려면 Agent를 대상보다 먼저 실행하세요.

```powershell
ArgusAgent.exe
```

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
