# Airconditioner Auto Control system

## 동기
- 차에서 집에 갈 때 미리 휴대폰으로 에어컨을 켤 수는 없을까?
- 집에 도착하기 전에 온라인에서 집 에어컨을 컨트롤할 수 있는 시스템
- IR 레이저, esp32 WiFi 연동, 나스 웹 서버 등의 협업으로 가능할 것으로 보임

## 요구사항
- 집에 항상 가동되어 원격에서 에어컨을 껐다가 켤 수 있는 시스템
- IR 센서를 통해 에어컨 제어
- 보안 시스템(누구나 들어올 수 없도록 전용 휴대폰에서만 접근할 수 있는 시스템)

## 시스템 구조

### 전체 아키텍처
```
[모바일 앱] ←→ [웹 서버] ←→ [ESP32] ←→ [IR LED] ←→ [에어컨]
```

### 기술 스택
- **펌웨어**: ESP-IDF (ESP32)
- **웹 서버**: Node.js + Express + SQLite (크로스 플랫폼)
- **모바일 앱**: React Native 또는 PWA
- **보안**: JWT 토큰 + HTTPS + API 키
- **데이터베이스**: SQLite (간단한 사용자 관리)

### 구성
- 펌웨어 `./firmware` : esp-idf 기반 펌웨어 ✅ **완성**
- 하드웨어 `./hardware` : KiCAD 하드웨어 회로
- 웹서버 `./webserver` : Node.js 기반 웹 서버 ✅ **완성**
- 모바일앱 `./mobileapp` : React Native 앱

## 개발 명세서

### 1. 웹 서버 (Node.js + Express) ✅ **완성**

#### 1.1 기술 스택
- **런타임**: Node.js 18+ (크로스 플랫폼 지원)
- **웹 프레임워크**: Express.js 4.x
- **데이터베이스**: SQLite3
- **인증**: JWT (jsonwebtoken)
- **암호화**: bcrypt
- **CORS**: cors
- **환경변수**: dotenv

#### 1.2 프로젝트 구조
```
webserver/
├── src/
│   ├── controllers/
│   │   ├── authController.js
│   │   ├── deviceController.js
│   │   └── settingsController.js
│   ├── middleware/
│   │   ├── auth.js
│   │   └── validation.js
│   ├── models/
│   │   ├── User.js
│   │   ├── Device.js
│   │   └── Settings.js
│   ├── routes/
│   │   ├── auth.js
│   │   ├── device.js
│   │   └── settings.js
│   ├── utils/
│   │   ├── database.js ✅
│   │   └── logger.js ✅
│   └── app.js ✅
├── public/
│   ├── index.html
│   ├── settings.html ✅
│   ├── css/
│   └── js/
├── config/
│   └── database.sql
├── package.json ✅
└── env.example ✅
```

#### 1.3 API 엔드포인트

##### 인증 API
- `POST /api/auth/login` - 사용자 로그인
- `POST /api/auth/logout` - 로그아웃
- `POST /api/auth/refresh` - 토큰 갱신

##### 디바이스 제어 API
- `GET /api/device/status` - ESP32 상태 확인
- `POST /api/device/control` - 에어컨 제어 명령
- `GET /api/device/history` - 제어 히스토리

##### 설정 API
- `GET /api/settings` - 시스템 설정 조회
- `PUT /api/settings` - 시스템 설정 업데이트
- `POST /api/settings/test-connection` - ESP32 연결 테스트

#### 1.4 데이터베이스 스키마 ✅ **완성**

##### Users 테이블
```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

##### Devices 테이블
```sql
CREATE TABLE devices (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name VARCHAR(100) NOT NULL,
    ip_address VARCHAR(15) NOT NULL,
    port INTEGER DEFAULT 80,
    api_key VARCHAR(255) NOT NULL,
    status VARCHAR(20) DEFAULT 'offline',
    last_seen DATETIME,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

##### Settings 테이블
```sql
CREATE TABLE settings (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    key VARCHAR(100) UNIQUE NOT NULL,
    value TEXT NOT NULL,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
);
```

##### Control_History 테이블
```sql
CREATE TABLE control_history (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id INTEGER,
    command VARCHAR(50) NOT NULL,
    parameters TEXT,
    user_id INTEGER,
    executed_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (device_id) REFERENCES devices(id),
    FOREIGN KEY (user_id) REFERENCES users(id)
);
```

#### 1.5 웹 설정 인터페이스 ✅ **완성**
- **URL**: `http://localhost:3000/settings`
- **기능**:
  - ESP32 IP 주소 설정
  - 포트 번호 설정
  - API 키 설정
  - 연결 테스트
  - 시스템 재시작

### 2. ESP32 펌웨어 (ESP-IDF) ✅ **완성**

#### 2.1 프로젝트 구조
```
firmware/
├── main/
│   ├── main.c ✅
│   ├── wifi_manager.c ✅
│   ├── ir_controller.c ✅
│   ├── web_server.c ✅
│   ├── api_handler.c
│   └── CMakeLists.txt ✅
├── components/
│   ├── ir_led/
│   └── wifi_config/
├── sdkconfig
└── CMakeLists.txt ✅
```

#### 2.2 주요 기능

##### WiFi 연결 관리 ✅
- WiFi 설정 저장 (NVS)
- 자동 재연결
- AP 모드 (설정용)

##### 웹 서버 ✅
- HTTP 서버 (포트 80)
- RESTful API 제공
- CORS 지원

##### IR 제어 ✅
- NEC 프로토콜 지원
- 에어컨 제어 명령 구현
- 신호 검증

#### 2.3 API 엔드포인트 ✅

##### 상태 확인
- `GET /api/status` - 디바이스 상태 반환
- `GET /api/wifi` - WiFi 연결 상태

##### 에어컨 제어
- `POST /api/aircon/power` - 전원 on/off
- `POST /api/aircon/temp` - 온도 설정
- `POST /api/aircon/mode` - 모드 설정 (냉방/난방/송풍)

##### 설정
- `POST /api/config/wifi` - WiFi 설정
- `GET /api/config` - 현재 설정 조회

#### 2.4 보안 ✅
- API 키 인증
- 요청 검증
- 로그 기록

### 3. 하드웨어 설계

#### 3.1 주요 구성 요소
- **ESP32-WROOM-32**: 메인 MCU
- **IR LED**: 에어컨 제어용
- **IR 수신기**: 에어컨 신호 학습용
- **LED 표시등**: 상태 표시
- **버튼**: 수동 제어용
- **전원 공급**: 5V → 3.3V 변환

#### 3.2 PCB 설계
- 2층 PCB
- 크기: 50mm x 30mm
- 커넥터: USB-C (전원/프로그래밍)
- GPIO 핀 배치 최적화

### 4. 모바일 앱 (PWA)

#### 4.1 기술 스택
- **프레임워크**: React + Vite
- **UI 라이브러리**: Material-UI
- **상태 관리**: Zustand
- **HTTP 클라이언트**: Axios

#### 4.2 주요 화면
- **로그인 화면**: 사용자 인증
- **메인 대시보드**: 에어컨 상태 및 제어
- **설정 화면**: 시스템 설정
- **히스토리 화면**: 제어 기록

#### 4.3 기능
- 푸시 알림
- 오프라인 지원
- 반응형 디자인

### 5. 배포 및 운영

#### 5.1 웹 서버 배포
- **Docker 지원**: 크로스 플랫폼 배포
- **환경별 설정**: 개발/운영 환경 분리
- **로그 관리**: 파일 기반 로그
- **백업**: SQLite DB 자동 백업

#### 5.2 보안 고려사항
- HTTPS 강제 적용
- API 요청 제한 (Rate Limiting)
- 입력값 검증
- SQL Injection 방지

### 6. 개발 순서

1. **ESP32 펌웨어 개발** (기본 WiFi + 웹서버) ✅ **완성**
2. **웹 서버 개발** (기본 API + 설정 페이지) ✅ **완성**
3. **하드웨어 제작** (PCB 설계 및 제작)
4. **IR 제어 기능 구현** (펌웨어 확장)
5. **모바일 앱 개발** (PWA)
6. **통합 테스트 및 최적화**

### 7. 테스트 계획

#### 7.1 단위 테스트
- 각 모듈별 기능 테스트
- API 엔드포인트 테스트
- 데이터베이스 쿼리 테스트

#### 7.2 통합 테스트
- 전체 시스템 연동 테스트
- 보안 테스트
- 성능 테스트

#### 7.3 사용자 테스트
- 실제 에어컨 제어 테스트
- 다양한 환경에서의 동작 확인

## 현재 진행 상황

### ✅ 완성된 부분
1. **ESP32 펌웨어**: WiFi 연결, 웹 서버, IR 제어 기능 구현
2. **웹 서버**: Node.js 기반 서버, 데이터베이스, API 구현
3. **설정 페이지**: 웹 기반 설정 인터페이스 구현
4. **데이터베이스**: SQLite 스키마 및 기본 데이터 구현

### 🔄 진행 중인 부분
- 하드웨어 설계 (사용자가 직접 진행)

### ⏳ 남은 작업
1. **하드웨어 제작**: PCB 설계 및 제작
2. **모바일 앱**: PWA 개발
3. **통합 테스트**: 전체 시스템 테스트
4. **배포**: 운영 환경 구축

## 설치 및 실행 방법

### 웹 서버 실행
```bash
cd webserver
npm install
cp env.example .env
# .env 파일에서 설정 수정
npm start
```

### ESP32 펌웨어 빌드
```bash
cd firmware
# ESP-IDF 환경 설정 필요
idf.py build
idf.py flash
```

## 다음 단계

1. **하드웨어 제작**: ESP32 기반 PCB 설계 및 제작
2. **모바일 앱 개발**: PWA 기반 모바일 인터페이스
3. **통합 테스트**: 전체 시스템 연동 테스트
4. **운영 배포**: 실제 환경에서의 안정성 확보