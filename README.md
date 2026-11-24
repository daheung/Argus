# Argus

Argus는 Windows x64 환경에서 동작하는 커널 드라이버와 사용자 모드 Agent로 구성된 안티치트 프로젝트입니다. 커널에서 프로세스, 스레드, 이미지 로드, 레지스트리, 객체 핸들 및 파일 시스템 이벤트를 처리하고, Agent가 보호 대상을 등록하고 알림을 조회해 기록합니다.

전체 흐름은 다음 세 단계로 구성됩니다.

1. **드라이버·서비스 시작** — `ArgusKernel`이 제어 장치와 커널 콜백을 초기화하고 Minifilter 시작을 시도합니다. 그다음 `ArgusAgent` 서비스가 시작됩니다.
2. **Agent 등록** — Agent가 `\\.\Argus` 장치를 열고 등록 IOCTL을 호출합니다. 커널은 요청 프로세스 이름과 서비스 SID를 확인합니다.
3. **이벤트 수집·정책 처리** — 커널 콜백이 프로세스·스레드·이미지·레지스트리·객체 핸들 이벤트를 처리하고, Minifilter가 CREATE 작업을 패시브 모드로 수집합니다. 보호·격리 역할에 따라 일부 핸들 권한을 조정합니다.
4. **알림 조회** — 커널이 이벤트를 종류별 큐에 보관하고, Agent가 IOCTL로 각 큐를 폴링합니다.
5. **보호 대상 등록** — Agent가 `maruexe.local.exe`의 생성 알림을 받으면 PID와 생성 시각을 커널에 등록합니다. 커널은 이후 해당 프로세스를 보호 대상으로 취급합니다.
6. **기록·정리** — Agent가 알림과 큐 유실 건수를 로그에 기록합니다. 커널은 프로세스 종료 시 등록된 참조를 정리하고, Agent는 서비스 중지 시 폴링을 멈추고 장치 핸들을 닫습니다.

현재 Argus의 프로젝트 버전은 **1.3.0**입니다.

---

## 1. System Requirements

### 1.1 Test environment

드라이버 패키지는 테스트 인증서와 테스트 altitude를 사용합니다. 테스트 VM에서 `ArgusTest.cer`를 신뢰하도록 설정해야 드라이버를 로드할 수 있습니다. MSI와 개발용 설치 도우미는 인증서를 자동 등록하지 않습니다. 정식 배포용 서명·설치 절차는 아직 준비되지 않았습니다.

---

## 2. Quick Start

1. `Argus/` 디렉터리에서 CMake 환경을 구성하고 Debug 빌드를 실행합니다.

   ```powershell
   cmake --preset windows-msvc-x64
   cmake --build --preset debug
   ```

   빌드 결과는 `.build/windows-msvc-x64` 아래에 생성됩니다.

2. Build a Release bundle

   ```powershell
   cmake --build --preset release --target ArgusBundle
   ```

   Agent, 설치 도우미, 드라이버 패키지와 테스트 인증서는 `.build/windows-msvc-x64/Bundle/Release`에 모입니다.

3. WiX 4.0.5를 설치한 뒤 CMake를 다시 구성하고 Release 빌드를 실행합니다.

   ```powershell
   dotnet tool install wix --version 4.0.5 --tool-path .build/wix4-tools
   cmake --preset windows-msvc-x64
   cmake --build --preset release
   ```

   WiX가 발견되면 Release 빌드에 `ArgusMsi`가 포함됩니다. 생성 경로는 `.build/windows-msvc-x64/Msi/Release/Argus-1.3.0-x64.msi`입니다.

---

## 3. Run the Kernel Driver

Argus 프로젝트는 `.sys` 파일을 직접 실행하지 않고, INF로 설치한 `ArgusKernel` 서비스를 시작해 로드합니다. 현재 [INF 설정 파일](Kernel/ArgusKernel.inf)은 **Windows 11 24H2 이상 x64**를 대상으로 하며 테스트용 altitude를 사용합니다. 다음 절차는 테스트 VM에서 수행합니다.

1. VMWare에서 실행 파일 준비하기

   먼저 Quick Start의 2번에서 만든 Release 번들을 테스트 VM으로 복사합니다. 번들에는 `ArgusSetup.exe`, `ArgusAgent.exe`, `ArgusTest.cer`와 `Driver\ArgusKernel.inf`·`.sys`·`.cat`이 포함됩니다.

   관리자 권한 PowerShell에서 [Windows 테스트 서명 모드](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option)를 켜고 VM을 재시작합니다.

   ```powershell
   bcdedit.exe /set TESTSIGNING ON
   ```

   Secure Boot 정책 때문에 명령이 거부되면 테스트 VM의 Secure Boot 설정을 확인해야 합니다. 테스트 서명 모드 변경은 **재시작 후** 적용됩니다.

   재시작 후 번들 디렉터리에서 테스트 인증서를 로컬 컴퓨터의 신뢰할 수 있는 루트 및 게시자 저장소에 등록합니다. [Microsoft의 테스트 인증서 설치 안내](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/installing-test-certificates)에 따라 인증서가 신뢰되어야 드라이버 패키지 서명을 검증할 수 있습니다.

   ```powershell
   Import-Certificate -FilePath .\ArgusTest.cer -CertStoreLocation Cert:\LocalMachine\Root
   Import-Certificate -FilePath .\ArgusTest.cer -CertStoreLocation Cert:\LocalMachine\TrustedPublisher
   ```

   `ArgusTest.cer`는 공개 인증서입니다. 서명용 개인 키와 `.pfx` 파일은 빌드 환경의 `.build/signing`에 두고 테스트 VM으로 복사하지 않습니다.

2. Install and start

   번들 디렉터리에서 관리자 권한으로 개발용 설치 도우미를 실행합니다.

   ```powershell
   .\ArgusSetup.exe install
   ```

   설치 도우미는 파일을 `%ProgramFiles%\Argus`에 복사하고, `ArgusKernel.inf`를 설치한 뒤 `ArgusKernel` 드라이버 서비스와 `ArgusAgent` 서비스를 순서대로 시작합니다. `ArgusAgent`는 `ArgusKernel`에 의존합니다.

   설치 도우미가 **`Restart Windows before starting the services`**를 출력하면 먼저 VM을 재시작합니다. 재시작 후 서비스를 조회하고 실행 중이 아니라면 관리자 권한으로 시작합니다.

   ```powershell
   sc.exe query ArgusKernel
   sc.exe query ArgusAgent
   ```

   실행 중이 아닌 서비스가 있다면 다음 명령으로 시작합니다.

   ```powershell
   sc.exe start ArgusKernel
   sc.exe start ArgusAgent
   ```

   `start` 명령은 해당 서비스가 실행 중이 아닐 때만 사용합니다. 드라이버가 이미 설치되어 있다면 `sc.exe start ArgusKernel` 또는 [`fltmc load ArgusKernel`](https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/loading-and-unloading)로 다시 로드할 수 있습니다.

3. Verify the driver

   관리자 권한 PowerShell에서 드라이버 서비스 상태와 Minifilter 등록 여부를 확인합니다.

   ```powershell
   sc.exe query ArgusKernel
   fltmc filters
   sc.exe query ArgusAgent
   Get-Content "$env:ProgramData\Argus\Agent.log" -Tail 50
   ```

   `ArgusKernel`의 서비스 상태가 `RUNNING`이고 `fltmc filters`에 이름이 표시되면 드라이버가 로드된 것입니다. Agent 로그는 이벤트 조회 경로를 추가로 확인하는 데 사용합니다. 드라이버 로드와 Minifilter 확인에 관한 명령은 [Microsoft의 Minifilter 테스트 도구 안내](https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/development-and-testing-tools)를 참고하십시오.

   MSI를 사용한다면 같은 VM에서 개발용 설치 도우미의 `install` 명령을 함께 실행하지 마십시오. MSI는 드라이버 설치와 Agent 서비스 시작을 자체적으로 수행합니다. 드라이버 설치에 재시작이 필요하면 현재 MSI 설치는 실패하고 롤백됩니다.
