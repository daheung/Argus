# FKrnlString 미해결 문제

기록일: 2026-11-02. 대상: `Kernel/Public/Containers/KrnlString.h` 및 `Kernel/Private/Containers/KrnlString.cpp`.

`ArgusKernel` 전체 빌드가 성공해도 사용되지 않은 템플릿과 클래스 템플릿 멤버는 인스턴스화되지 않을 수 있다. 각 항목을 수정할 때 실제 호출부를 컴파일하여 확인한다.

## 생성, 대입, 초기화

1. **기본 복사 생성자와 복사·이동 대입 연산자**: `FKrnlString`은 세 함수를 `= default`로 선언하지만, `TKrnlArray`에는 대응하는 복사 생성자와 대입 연산자가 없다. 사용 시 삭제된 함수 오류가 난다. 기본 이동 생성자는 배열과 할당자의 이동 생성자를 사용한다.
2. **`ExtraSlack` 생성자**: `FKrnlString(Other, ExtraSlack)` 두 오버로드는 현재 없는 `TKrnlArray(Other.Data, ExtraSlack)` 생성자를 호출한다. 이동 오버로드의 `MoveTemp`에도 별도 정적 검증 문제가 있다. 실패 가능한 예약을 생성자에서 처리할 방법까지 정한 뒤 다시 설계한다.
3. **`InitializeWithLength` 세 오버로드**: `InitializeWithLengthImpl`의 인자는 `(Source, InitialLength)`인데 호출부는 `(StrLength, String)` 순서로 전달한다. 실제 사용 시 컴파일되지 않는다.
4. **`InitializeWithLengthImpl`**: `Source`와 길이의 조합, 음수 길이, `ConvertedLength` 실패, `DestLen + 1`의 범위를 먼저 검사해야 한다. 현재는 `Data`를 늘린 뒤 변환하므로 변환 실패 시 이전 문자열이 유지되지 않는다.
5. **`InitializeWithSlackImpl`**: `TCString<TArrayCharType>::Strlen(Source)`는 입력 문자 타입이 ANSI/UTF-8일 때 맞지 않는다. `ConvertedLength<CharType>`도 출력 타입인 `WIDECHAR`가 아니라 입력 타입을 지정한다. 길이·여유 공간 합산과 실패 시 기존 상태 유지도 점검해야 한다.

## 수정 연산과 입력 범위

6. **`AssignRange`**: 짧은 범위로 교체하는 경로가 기존 원소를 먼저 덮어쓴 뒤 `RemoveAt`을 호출한다. 제거가 실패하면 함수는 실패를 반환해도 문자열은 이미 변경되어 있다. 입력이 현재 문자열과 겹치지 않는다는 호출 규약도 API에 명시하거나 검사해야 한다.
7. **범위 `Append`**: `Append(CharRangeType&&)`가 원소 타입 대신 `CharRangeType` 자체를 `AppendCharsImpl`의 템플릿 인자로 전달한다. 범위 입력을 사용하려면 데이터 포인터의 원소 타입으로 변환해야 한다.
8. **범위 타입 특성**: 현재 `TIsContiguousContainer`는 C 배열만 등록되어 있고, `FKrnlString`의 범위 제약은 C 배열을 제외한다. 따라서 비배열 범위용 `Initialize`·`Append`·`Concat` 오버로드는 대상 컨테이너 특성을 추가하기 전에는 사실상 선택되지 않는다. `Concat`에는 별도로 `(포인터, 길이)` 오버로드를 마련했다.
9. **`Append` 입력 타입**: 포인터 오버로드는 문자 타입을 제한하지 않아 `UTF32CHAR*`도 받을 수 있다. 현재 UTF-32에서 `WIDECHAR`로의 변환은 BMP 밖의 문자를 UTF-16 서로게이트 쌍으로 만들지 않고 `?`로 대체할 수 있다.

## 인코딩과 API 계약

10. **`AppendChar`의 의미**: `UTF8CHAR` 한 개는 비 ASCII 문자 전체가 아니다. 현재 변환기는 불완전한 UTF-8 입력을 `?`로 바꿀 수 있다. `WIDECHAR` 한 개는 단독 서로게이트일 수 있다. 단일 코드 유닛 삽입과 완전한 문자 삽입 중 어떤 계약인지 정하고 유효성을 검사한다.
11. **손실 변환 정책**: `FGenericString::Convert`의 일부 경로는 표현할 수 없거나 잘못된 입력을 `?`로 대체한다. 레지스트리·프로세스 알림 이름처럼 원본을 완전하게 보존하거나 아예 제공하지 않아야 하는 데이터에는 변환 성공 여부와 손실 여부를 별도로 판정해야 한다.
12. **반환 및 실패 계약**: `operator=`와 경로 연산자 등은 실패를 호출자에게 전달하지 못하거나 내부 실패를 무시하는 곳이 있다. `PathAppend` 및 관련 연산의 할당 실패 처리와 기존 문자열 유지 규칙을 다시 확인한다.

## 후속 순서

1. 삭제된 생성·대입 경로와 초기화 오버로드의 컴파일 문제를 해결한다.
2. 모든 쓰기 API의 실패 시 상태 유지와 길이 오버플로를 확인한다.
3. ANSI/UTF-8/UTF-16/UTF-32 입력 정책과 손실 변환 판정을 정의한다.
4. 실제 사용 조합을 인스턴스화하는 컴파일 검증과 빈 문자열·할당 실패·잘못된 인코딩 검증을 추가한다.

레지스트리 콜백 개발에서는 이 항목들이 해결되기 전까지 `FKrnlString`을 알림 이름의 신뢰 가능한 저장소로 가정하지 않는다.
