# 📱PocketMoneyApp

안녕하세요. 신입 개발자 구승민입니다.
PocketMoneyApp은 데이터를 **정확하고 빠르게 관리**하는 데 초점을 맞춘 **용돈 관리 앱**입니다.
**데이터의 신뢰성과 애플리케이션의 반응성**에 대해 고민하며, 어떤 기술적 설계와 고민을 거쳐 개발되었는지 설명하겠습니다.

## 개발 결과 이미지

<img width="3264" height="1536" alt="1" src="https://github.com/user-attachments/assets/2036432c-a74a-45f0-a46f-c00c5bd9b2f0" />

---

## 🔎 상황 & 기술적 배경: "데이터는 정확해야 하고, 앱은 빨라야 한다\!"

처음 과제를 받으며 **돈**과 관련된 문제는 대규모 데이터 처리 시 성능 저하나 데이터 불일치 등의 문제에 대해 생각했습니다.

- **트랜잭션 활용**
    - 가장 먼저 **정확성**을 생각했을 때, ACID 트랜잭션 특성 중 **Atomicity (원자성)**이 떠올랐습니다.
    - 금액과 관련된 부분은 **All or Nothing**으로 정확하게 처리될 수 있어야 한다고 생각했습니다.
- **데이터 무결성**
    - 지갑 잔액은 오차도 없이 정확해야 합니다.
    - 복잡한 트랜잭션 변경(추가/수정/삭제) 과정에서 문제가 생길 수 있는 시나리오를 떠올렸습니다.
- **빠른 처리**
    - 트랜잭션 수가 늘어나더라도, 지갑 조회나 잔액 업데이트가 지연 없이 이루어져야 한다고 생각했습니다.
- **C/C++ Native**
    - 하나의 프로젝트에서 **Foreign Function Interface (FFI)**를 통한 개발을 진행한 경험이 없어서 이에 대한 빠른 학습이 중요하다고 판단했습니다.

요구사항에 맞게 **C/C++ native 코드**를 선택하였지만, 대량의 데이터 처리 로직이나 복잡한 계산에서 발생할 수 있는 상황을 고민하며 개발에 임했습니다.

---

## 🛠️ 아키텍처 설계 & 구현: "기본기는 탄탄한가? 무엇을 놓치고 있는가?"

아키텍처 설계에 관한 내용을 찾아보던 중 다음 두 구조 중에 고민하였습니다.

<img width="736" height="300" alt="2" src="https://github.com/user-attachments/assets/93d4b55b-5a76-4377-b6b2-a9b56e04055a" />

3-tier와 4-tier를 고민하던 중, **5일이라는 짧은 시간** 내에 구현을 목표로 했기 때문에 **3-tier**로 개발하기로 결정했습니다.

<img width="1376" height="1664" alt="3" src="https://github.com/user-attachments/assets/363b0498-c8e9-4aab-9b2f-b5cba4a5e910" />

- **Presentation Tier (Kotlin)**
    - 사용자 인터페이스 및 상호작용을 담당합니다.
    - `MainActivity`, `TransactionListActivity` 등 UI 컴포넌트
- **Logic Tier (C++/JNI)**
    - 앱의 핵심 비즈니스 로직(예: 잔액 계산, 트랜잭션 CRUD)을 처리합니다.
    - `WalletRepository`, `TransactionRepository` 및 JNI 함수 등
- **Data Tier (SQLite)**
    - SQLite 데이터베이스를 통해 모든 데이터를 앱 내에 저장하고 관리합니다.
    - `DatabaseHelper`가 데이터베이스 접근을 추상화합니다.

이 아키텍처를 통해 개발 효율성과 유지보수성을 높이려고 했지만, 구현 과정에서 몇 가지 문제가 발생했습니다.

### 1. 구현 난관 A: 지갑 잔액 일관성 확보 (재계산 로직)

- **문제 상황**
    - 트랜잭션이 추가, 수정, 삭제될 때, 해당 지갑의 최종 잔액이 항상 정확히 일치하는 것을 보장하는 것이 핵심이었습니다.
    - 단순히 이전 값에서 증분/감분하는 방식은 트랜잭션 수정 시 발생하는 복잡한 경우의 수(예: 금액 변경, 유형 변경)를 처리하기 복잡했고, 데이터 불일치 위험이 높았습니다.
- **해결 전략**
    - `WalletRepository.cpp`에 `recalculateBalance` 함수를 도입하여, 잔액 변경이 필요한 시점마다 해당 지갑에 속한 모든 트랜잭션을 데이터베이스에서 **다시 조회하여 총 잔액을 계산하고 업데이트**하는 방식을 채택했습니다.

-  **관련 소스 코드 요약(`WalletRepository.cpp`)**
```cpp
// bool WalletRepository::recalculateBalance(int walletId)

// 잔액 결과 SQL문 생성
sqlite3_stmt *stmt;
const char* sql = "SELECT SUM(CASE WHEN Type = 0 THEN Amount ELSE -Amount END) FROM Transactions WHERE wallet_id = ?;";

// SQL문 값 할당
sqlite3_bind_int(stmt, 1, walletId);
// 쿼리 실행
sqlite3_step(stmt);
// 결과 저장
long long newBalance = 0;
newBalance = sqlite3_column_int64(stmt, 0);

// 리소스 해제
sqlite3_finalize(stmt);

// 업데이트 SQL문 생성
sqlite3_stmt *updateStmt;
const char* updateSql = "UPDATE Wallets SET BALANCE = ? WHERE ID = ?;";

// SQL문 값 할당
sqlite3_bind_int64(updateStmt, 1, newBalance);
sqlite3_bind_int(updateStmt, 2, walletId);
// 쿼리 실행
sqlite3_step(updateStmt);

// 리소스 해제
sqlite3_finalize(updateStmt);
```

- **호출 지점 (`TransactionRepository.cpp`):**
```cpp
// 트랜잭션 생성, 수정, 삭제 성공 후 해당 지갑의 잔액 재계산 호출
if (success && s_walletRepo != nullptr) {
  s_walletRepo->recalculateBalance(newTransaction.walletId);
}
```
- 결과 로그
<img width="1612" height="178" alt="4" src="https://github.com/user-attachments/assets/5e6abfe8-3614-4b4d-b028-9ff90824cc65" />

- **현재의 한계**
    - '전체 재계산' 방식은 정확하고 효율적일 수 있지만, **트랜잭션 수가 극단적으로 많아질 경우(예: 10만 건 이상)** 성능 저하가 생길 수 있다고 생각합니다.

### 2. 구현 난관 B: JNI 계층의 함수명, 참조 관리

- **문제 상황**
    - Kotlin/Java (Android)와 C++ (Native Core) 간의 인터페이스인 JNI 계층에서 예상치 못한 크래시와 `NoSuchMethodError`가 빈번했습니다.
    - 이는 앱의 전반적인 안정성을 및 개발속도를 늦추는 원인이었습니다.
- **근본 원인 분석**
    - **JNI 함수명 불일치**
        - Kotlin의 `external` 함수 선언 방식(`companion object` 유무)과 C++ JNI 함수명 (`Java_패키지명_클래스명_함수명`) 간의 정확한 매핑 규칙을 파악하고 적용하는 데 어려움이 있었습니다.
    - **JNI 참조 관리 누락**
        - C++ 코드에서 Java 객체(예: `jstring`, `jobject`)를 생성하거나 참조할 때, `Local Reference` 및 `Global Reference`를 올바르게 사용하고, 사용 후 명시적으로 해제(`DeleteLocalRef`, `DeleteGlobalRef`)하지 않아 에러가 많이 발생했습니다.
        - 특히 `jclass` 및 `jmethodID`는 앱의 생명주기 내내 유효하도록 `JNI_OnLoad`에서 `GlobalRef`로 저장해야 한다는 점이 중요했습니다.
- **해결 전략**
    - **JNI 함수명 컨벤션 적용**
        - Kotlin `external` 함수 선언에 맞춰 C++ JNI 함수명을 `Java_com_example_pocketmoneyapp_클래스명_함수명` 형태로 통일하여 적용했습니다.
    - **`JNI_OnLoad`/`JNI_OnUnload`를 통한 `GlobalRef` 관리**
        - 앱 시작 시 (`JNI_OnLoad`), 필요한 모든 DTO (`jclass`) 및 해당 생성자 (`jmethodID`)를 `env->NewGlobalRef()`를 사용하여 **전역 참조로 저장**
        - 앱 종료 시 (`JNI_OnUnload`) `env->DeleteGlobalRef()`로 **명시적으로 해제**하여 앱 생명주기 전반에 걸친 안정성을 확보했습니다.
    - **`LocalRef`의 해제**
        - 모든 JNI 함수 내에서 `NewStringUTF`, `NewObject` 등으로 생성된 **로컬 참조들**은 사용 직후 `env->DeleteLocalRef()`를 **호출하여 해제**함으로써 메모리 누수 및 JNI 스택 오버플로우 위험을 제거했습니다.
- **관련 소스 코드 요약 (`native_core.cpp`)**
```cpp
// JNI_OnLoad에서 DTO 클래스와 생성자를 GlobalRef로 저장
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* reserved) {
  // ... JNIEnv 획득 ...
  jclass walletDtoLocalClass = env->FindClass("com/example/pocketmoneyapp/data/WalletDto");
  g_walletDtoClass = reinterpret_cast<jclass>(env->NewGlobalRef(walletDtoLocalClass));
  g_walletDtoConstructor = env->GetMethodID(g_walletDtoClass, "<init>", "(ILjava/lang/String;Ljava/lang/String;J)V");
  env->DeleteLocalRef(walletDtoLocalClass); // 로컬 참조 즉시 해제
  // 다른 DTO도 동일하게 처리
  return JNI_VERSION_1_6;
}

// JNI 함수 내에서 로컬 참조 사용 및 해제(getWalletByIdNative)
extern "C" JNIEXPORT jobject JNICALL Java_com_example_pocketmoneyapp_TransactionListActivity_getWalletByIdNative(JNIEnv* env, jobject /* this */, jint id) {
  // ... (Repository 호출 및 데이터 획득) ...
  jstring nameJStr = env->NewStringUTF(wallet.name.c_str());
  jstring descriptionJStr = env->NewStringUTF(wallet.description.c_str());
  jobject walletDtoObj = env->NewObject(g_walletDtoClass, g_walletDtoConstructor, /* ... 데이터 전달 ... */);
  env->DeleteLocalRef(nameJStr); // 사용 후 로컬 참조 해제
  env->DeleteLocalRef(descriptionJStr);
  return walletDtoObj; // 반환하는 객체는 JNI가 자동으로 해제 (또는 호출자가 해제)
}
```
- **현재의 한계**
    - JNI를 직접 사용하는 방식은 코드의 **복잡성**을 증가시키고, C++ 개발/디버깅 난이도가 높습니다.
    - Kotlin DTO 객체를 C++에서 수동으로 생성하고 필드를 채우는 과정에서 반복적이여서 잘못 호출할 가능성도 있습니다.

### 3. 아키텍처적 미흡함: 계층 간 결합도 (현재의 아키텍처 한계)

- **문제 상황**
    - 현재 3-Tier 아키텍처에서 **Presentation Tier (Kotlin Activity)가 Logic Tier (Native Core의 JNI 함수)를 직접 호출**하고 있습니다.
    - 이는 계층 간 의존성을 높여 장기적인 유지보수와 테스트 용이성에 부정적인 영향을 미칠 수 있습니다.
- **현재의 한계**
    - "Presentation Tier가 JNI를 직접 호출"하는 이 구조는 **Application/Service Layer의 부재**를 의미하며, 이는 3-Tier의 이상적인 모델에서 벗어나 Presentation Tier에 비즈니스 시나리오 조율 로직이 일부 혼재되어 있음을 나타냅니다.
    - 이는 엄밀히 말해 아키텍처적 결합도를 높이는 요인이지만, **기한 내 개발 목적 달성을 위해 감수**했습니다.

---

## 📝 개선하고 싶은점

1. **아키텍처 설계와 구현 과정 개선**
    - 아키텍처에 대한 학습을 지속하고 있지만, 많은 기술과 방법을 적용하는 것이 가장 큰 고민이었습니다.
    - 동일한 문제를 해결하는 솔루션도 적용하는 아키텍처에 따라 천차만별로 나뉠 수 있다고 생각합니다.
    - 지속적인 학습을 통해 여러 아키텍처의 구조를 이해하고, 비교하며, 선택할 수 있는 역량을 기르고 싶습니다.

2. **설계 시간과 개발 속도의 문제**
    - 핑계일 수 있지만, 총 5일의 시간을 온전히 쓰지 못한 것이 아쉽습니다.
    - 제가 분배한 프로젝트의 할당 시간은 다음과 같습니다.
        - **월요일** 10시~20시 | 10시간
            - `요구사항 분석, 아키텍처 탐색, C/C++ native 구현 방법 탐색`
        - **화요일** 17시~24시 | 7시간
            - `FFI, JNI 이해 및 구현 방식 학습 (관련 자료 탐색에 오랜 시간이 걸렸습니다)`
        - **수요일** 17시~03시 | 10시간
            - `프로젝트 생성 및 Cpp native 기능 연동`
            - `개발 플랫폼 변환(데스크톱, 노트북) 버전 문제 (JDK 및 Android Studio, AGP 버전 차이)`
            - `Git Repo 개설 및 Init`
        - **목요일** 17시~03시 | 10시간
            - `SQLite 헤더 추가 및 연동`
            - `DB in-memory 저장 방식 테스트`
            - `DB 저장 방식 변경(앱 내 저장)`
            - `WalletDto 생성 및 기초 UI 작업`
            - `지갑 생성 기능 구현 및 동작 성공`
        - **금요일** 10시~10시 | 12시간
            - `TransactionDto 생성 및 JNI 브릿지 함수 구현`
            - `지갑 잔액 유지용 recalculateBalance 함수 활용`
            - `UI/UX 수정 및 기타 버그 수정`
    - 많다면 많고, 적다면 적을 수 있는 시간이지만, 제 개인적인 생각으로는 더 많은 시간을 활용하여 개발하고 싶은 마음이 있습니다.

3. **더 많은 경험**
    - FFI 개념을 공부하면서 서로 다른 기술이라고 생각했던 **C와 자바(Java)를 통합하는 과정**이 정말 뜻깊었습니다.
    - 결국 기계어로 번역되어 동작하는 것은 마찬가지지만, 통합하는 과정 자체를 **배워가는 과정은 정말 즐거웠습니다**.
    - 테스트로 진행된 개발이긴하지만, 새로운 기술을 알고 **빠른 시간 내에 적용해보는 경험**을 할 수 있어서 좋았습니다.

> **오늘의 고민은 내일의 더 좋은 솔루션이 된다고 생각합니다. 좋은 기회 제공해주셔서 감사합니다.**
