# B 언어 컴파일러 프로젝트

성균관대학교 프로그래밍언어 과제

## 개요

이 프로젝트는 ANTLR를 사용하여 "B"라는 언어를 C++로 변환하는 컴파일러의 일부를 구현한 것입니다. "B" 언어는 모든 타입이 `auto`로 정의된 간단한 언어로, 이 컴파일러는 심볼 테이블 생성, 타입 추론, 그리고 C 코드 출력을 수행합니다. 프로젝트는 다음과 같은 세 단계로 나뉘어 있습니다:

1. **심볼 테이블 생성**: 프로그램 내의 심볼(변수, 함수 등)을 수집하여 심볼 테이블에 저장합니다.
2. **타입 추론**: `auto`로 선언된 변수와 함수의 타입을 추론합니다.
3. **C 코드 출력**: 추론된 타입을 바탕으로 C 코드를 생성하고 출력합니다.

## 프로젝트 구조

```
B2CCompiler/
├── B2CMain.cpp          # 메인 컴파일러 코드
├── antlr4-cpp/          # ANTLR4 C++ 런타임 및 생성된 코드
│   ├── BBaseVisitor.h
│   ├── BLexer.h
│   ├── BParser.h
│   └── ...
├── B.g4                 # B 언어의 ANTLR 문법 파일
└── README.md            # 이 파일
```

- **`B2CMain.cpp`**: 컴파일러의 주요 로직이 포함된 파일로, ANTLR 파서와 방문자 클래스를 활용하여 입력 파일을 처리합니다.
- **`antlr4-cpp/`**: ANTLR4 C++ 런타임과 `B.g4` 문법 파일에서 생성된 코드가 포함됩니다.
- **`B.g4`**: "B" 언어의 문법을 정의한 ANTLR 문법 파일입니다.

## 주요 구성 요소

- **`SymbolTable`**: 심볼의 속성(타입, 범위 등)을 저장하는 클래스입니다. 변수 이름과 해당 속성을 매핑하며, 범위(scope)를 구분합니다(0: 글로벌, 1: 로컬).
- **`SymbolTableVisitor`**: ANTLR의 방문자 패턴을 활용하여 심볼 테이블을 구축합니다.
- **`TypeAnalysisVisitor`**: `auto`로 선언된 변수와 함수의 타입을 추론합니다.
- **`PrintTreeVisitor`**: 추론된 타입을 기반으로 C 코드를 출력합니다.

## 설치 및 실행 방법

### 1. ANTLR4 설치
- ANTLR4를 설치하고, ANTLR4 C++ 런타임을 프로젝트에 포함시킵니다.
- ANTLR4 jar 파일을 다운로드하고, `B.g4` 파일을 사용하여 파서와 렉서를 생성합니다:
  ```bash
  java -jar antlr-4.9.3-complete.jar -Dlanguage=Cpp B.g4
  ```
- 생성된 파일(`BLexer.h`, `BParser.h` 등)을 `antlr4-cpp/` 디렉토리에 배치합니다.

### 2. 프로젝트 빌드
- C++ 컴파일러(g++ 등)를 사용하여 `B2CMain.cpp`와 ANTLR4에서 생성된 파일들을 컴파일합니다:
  ```bash
  g++ -std=c++11 -I antlr4-cpp/ -I /path/to/antlr4/runtime/ B2CMain.cpp antlr4-cpp/*.cpp -l antlr4-runtime -o B2CCompiler
  ```
  - `/path/to/antlr4/runtime/`은 ANTLR4 런타임 라이브러리의 경로로 대체해야 합니다.

### 3. 실행
- 컴파일된 실행 파일을 사용하여 "B" 언어로 작성된 파일을 변환합니다:
  ```bash
  ./B2CCompiler <input-file>
  ```
  - `<input-file>`은 "B" 언어로 작성된 소스 파일의 경로입니다.

## 기능 설명

### 1. 심볼 테이블 생성
- **`SymbolTableVisitor`**는 파스 트리를 방문하여 변수와 함수를 심볼 테이블에 등록합니다.
- 글로벌 범위(`$_global_$`)와 함수별 로컬 범위를 구분하여 심볼을 관리합니다.
- 예: `auto x = 10;` → 심볼 테이블에 `{name: "x", type: tyINT, scope: 0}` 추가.

### 2. 타입 추론
- **`TypeAnalysisVisitor`**는 `auto`로 선언된 변수와 함수의 타입을 추론합니다.
- 현재 구현은 변수 초기화 값(`int`, `double`, `string` 등)을 기반으로 타입을 결정하며, 함수 반환 타입 추론은 제한적입니다.
- 예: `auto x = 10;` → `x`의 타입을 `tyINT`로 추론.

### 3. C 코드 출력
- **`PrintTreeVisitor`**는 파스 트리를 방문하여 C 형식의 코드를 출력합니다.
- 함수 정의, 변수 선언, 제어문 등을 변환하며, 현재는 모든 타입을 `auto`로 유지한 상태로 출력합니다.
- 예: `auto fn(auto x) { auto y = 10; }` → `auto fn(auto x) { auto y = 10; }`.

## B 언어 문법 요약
- **변수 선언**: `auto x, y = 10;`
- **함수 정의**: `auto fn(auto x, auto y) { ... }`
- **제어문**: `if`, `while`, `return` 등 지원.
- **연산자**: 산술(`+`, `-`, `*`, `/`), 비교(`>`, `<`, `==`), 논리(`&&`, `||`) 등.
- 자세한 문법은 `B.g4` 파일을 참조하세요.

## 의존성
- **ANTLR4**: 문법 파싱 및 트리 방문자를 위한 도구.
- **C++11 이상**: 프로젝트는 C++11 이상의 기능을 사용합니다.
