# План реализации легковесного WebSocket стека на C++11+

## Обзор проекта

**Название:** wscpp — легковесный WebSocket стек для C++11+

**Цель:** Создать полностью соответствующий RFC 6455 WebSocket протоколу библиотеку с минимальными зависимостями, высокой производительностью и простотой использования.

---

## 1. Архитектурные решения

### 1.1 Базовая архитектура

```
wscpp/
├── include/wscpp/          # API заголовки
│   ├── websocket.hpp       # Основной API
│   ├── client.hpp          # Клиентский API
│   ├── server.hpp          # Серверный API
│   ├── connection.hpp      # Connection interface
│   ├── frame.hpp           # Framing logic
│   ├── handshake.hpp       # Handshake logic
│   └── extensions/         # Extension support
│       ├── permessage_deflate.hpp
│       └── compression.hpp
├── src/                    # Implementation
│   ├── websocket.cpp
│   ├── client.cpp
│   ├── server.cpp
│   ├── connection.cpp
│   ├── frame.cpp
│   └── handshake.cpp
├── tests/                  # All tests
│   ├── unit/
│   ├── integration/
│   └── regression/
├── examples/               # Examples
│   ├── simple_client/
│   ├── simple_server/
│   ├── tls_client/
│   └── tls_server/
├── docs/                   # Documentation
│   ├── user-guide.md
│   ├── developer-guide.md
│   └── api-reference/
├── cmake/                  # CMake modules
│   ├── FindOpenSSL.cmake
│   └── wscpp-config.cmake.in
└── benchmarks/             # Performance benchmarks
```

### 1.2 Выбор зависимостей

**Минимальные зависимости (обязательные):**
- C++11 стандартная библиотека

**Опциональные зависимости:**
- **Boost.Asio** — для跨平台 async I/O (рекомендуется для production)
- **OpenSSL** — для TLS/SSL поддержки
- **zlib** — для permessage-deflate расширения

**Альтернативы:**
- Native sockets + select/poll/epoll/kqueue (без Boost)
- LibreSSL/BoringSSL вместо OpenSSL

### 1.3 Подход к API

**Два уровня API:**

1. **Low-level (frame-based):**
   - Прямой контроль над фреймами
   - Для экспертов и специфичных use cases

2. **High-level (message-based):**
   - Отправка/получение сообщений целиком
   - Автоматическая сборка фрагментированных сообщений
   - Для большинства use cases

### 1.4 Поддержка асинхронности

**Поддержка трех паттернов:**

1. **Callback-based async** (Boost.Asio style)
2. **Future/Promise** (C++11 std::future)
3. **Synchronous blocking** (для простых случаев)

---

## 2. Реализация по этапам

### Этап 0: Подготовка репозитория (1-2 дня)

**Задачи:**
- [ ] Создать репозиторий с GitHub/GitLab
- [ ] Настроить .gitignore (C++, CMake, IDE)
- [ ] Добавить LICENSE (MIT или Apache-2.0)
- [ ] Добавить README.md с кратким описанием
- [ ] Добавить CONTRIBUTING.md
- [ ] Настроить CI/CD (GitHub Actions)
- [ ] Настроить CMake build system
- [ ] Создать структуру директорий

**Структура CMake:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(wscpp VERSION 0.1.0 LANGUAGES CXX)

option(WSCPP_BUILD_TESTS "Build tests" ON)
option(WSCPP_BUILD_EXAMPLES "Build examples" ON)
option(WSCPP_WITH_TLS "Enable TLS support" ON)
option(WSCPP_WITH_ZLIB "Enable compression support" ON)
option(WSCPP_USE_BOOST_ASIO "Use Boost.Asio instead of native sockets" ON)
```

---

### Этап 1: Ядро протокола (3-5 дней)

**Задачи:**
- [ ] Реализовать frame parsing/serialization (RFC 5.2)
- [ ] Реализовать masking/unmasking (RFC 5.3)
- [ ] Реализовать opcodes (text, binary, close, ping, pong)
- [ ] Реализовать fragmentation (RFC 5.4)
- [ ] Реализовать handshake (RFC 4.1, 4.2)
- [ ] Реализовать error handling (RFC 7)

**Ключевые классы:**
- `frame_header` — структура заголовка фрейма
- `frame_parser` — парсинг входящих фреймов
- `frame_builder` — сборка исходящих фреймов
- `handshake_request` / `handshake_response`
- `websocket_error` — enum для ошибок

**Тесты:**
- [ ] Unit tests для frame parsing
- [ ] Unit tests для masking
- [ ] Integration tests с реальными WebSocket серверами

---

### Этап 2: Connection API (3-4 дня)

**Задачи:**
- [ ] Реализовать `websocket_connection` interface
- [ ] Реализовать `client_connection` (выходящие соединения)
- [ ] Реализовать `server_connection` (входящие соединения)
- [ ] Реализовать callback API
- [ ] Реализовать synchronous API
- [ ] Реализовать TLS support (опционально)

**Ключевые классы:**
- `class websocket_connection` — базовый класс
- `class client_connection : public websocket_connection`
- `class server_connection : public websocket_connection`
- `class connection_config` — конфигурация соединения

**Тесты:**
- [ ] Unit tests для connection lifecycle
- [ ] Integration tests с Echo сервером
- [ ] Tests для timeout и error cases

---

### Этап 3: Client и Server API (3-4 дня)

**Задачи:**
- [ ] Реализовать `websocket_client` — удобный клиент
- [ ] Реализовать `websocket_server` — удобный сервер
- [ ] Реализовать subprotocol negotiation
- [ ] Реализовать extension negotiation
- [ ] Реализовать graceful shutdown

**Ключевые классы:**
- `class websocket_client`
- `class websocket_server`
- `class server_config`
- `class client_config`

**Тесты:**
- [ ] Unit tests для client/server
- [ ] Integration tests с multi-client scenarios
- [ ] Tests для subprotocol negotiation

---

### Этап 4: Расширения (2-3 дня)

**Задачи:**
- [ ] Реализовать permessage-deflate extension (RFC 7692)
- [ ] Реализовать compression/decompression
- [ ] Добавить extension negotiation

**Ключевые классы:**
- `class permessage_deflate_extension`
- `class compression_context`

**Тесты:**
- [ ] Unit tests для compression
- [ ] Integration tests с сжатыми сообщениями

---

### Этап 5: Примеры (2-3 дня)

**Задачи:**
- [ ] Пример простого клиента (send/receive text)
- [ ] Пример простого сервера (echo server)
- [ ] Пример TLS клиента
- [ ] Пример TLS сервера
- [ ] Пример с subprotocols
- [ ] Пример с compression
- [ ] Пример с callbacks
- [ ] Пример с futures

**Ключевые примеры:**
- `examples/simple_client/main.cpp`
- `examples/simple_server/main.cpp`
- `examples/tls_client/main.cpp`
- `examples/tls_server/main.cpp`
- `examples/echo_server/main.cpp`
- `examples/chat_server/main.cpp`

---

### Этап 6: Тесты (4-6 дней)

**Юнит-тесты:**
- [ ] Frame parsing/serialization (100+ test cases)
- [ ] Masking/unmasking
- [ ] Handshake (valid/invalid cases)
- [ ] Error handling
- [ ] Extension parsing

**Интеграционные тесты:**
- [ ] Подключение к wss://echo.websocket.org
- [ ] Тесты с различными серверами ( autobahn testsuite)
- [ ] Multi-client scenarios
- [ ] Stress tests (large messages, high frequency)

**Регрессионные тесты:**
- [ ] Тесты для известных багов
- [ ] Тесты backward compatibility
- [ ] Performance regression tests

**Фреймворк тестов:**
- Google Test (gtest)
- CMake integration
- CI/CD integration

---

### Этап 7: Документация (5-7 дней)

**Пользовательская документация:**
- [ ] Установка (CMake, vcpkg, conan)
- [ ] Быстрый старт (5 минут tutorial)
- [ ] Руководство по API
- [ ] Примеры использования
- [ ] FAQ
- [ ] Лучшие практики

**Разработческая документация:**
- [ ] Архитектура проекта
- [ ] Руководство по коду
- [ ] How to contribute
- [ ] Release process
- [ ] Security considerations

**API reference:**
- [ ] Doxygen comments
- [ ] Сгенерированная документация
- [ ] Примеры кода для каждого класса

---

### Этап 8: Сравнительный анализ (2-3 дня)

**Библиотеки для сравнения:**
1. **Boost.Beast** — полная функциональность, большой overhead
2. **uWebSockets** — высокая производительность, сложный API
3. **WebSocket++** — header-only, хорошая документация
4. **Poco::Net** — фреймворк целиком, много зависимостей

**Метрики сравнения:**
- Размер бинарника
- Memory usage
- Latency
- Throughput
- Ease of use
- Documentation quality
- Feature completeness

**Таблица сравнения:**
| Feature | wscpp | Boost.Beast | uWebSockets | WebSocket++ | Poco::Net |
|---------|-------|-------------|-------------|-------------|-----------|
| C++11+ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Header-only | ✓ | ✗ | ✓ | ✓ | ✗ |
| TLS | ✓ | ✓ | ✓ | ✓ | ✓ |
| Compression | ✓ | ✓ | ✓ | ✗ | ✗ |
| Async API | ✓ | ✓ | ✓ | ✗ | ✗ |
| Sync API | ✓ | ✗ | ✗ | ✓ | ✓ |
| Dependencies | minimal | Boost | none | Boost | Poco |

---

### Этап 9: Выпуск и поддержка (ongoing)

**Первый релиз (v0.1.0):**
- [ ] Final code review
- [ ] Documentation review
- [ ] Security audit
- [ ] Performance benchmarks
- [ ] Release notes
- [ ] GitHub Releases

**Поддержка версий:**
- Semver: MAJOR.MINOR.PATCH
- Backward compatibility policy
- Deprecation policy

---

## 3. Приоритеты и зависимости

### Критичный путь (для MVP):
1. Подготовка репозитория (0)
2. Ядро протокола (1) → frame parsing, handshake
3. Connection API (2) → basic send/receive
4. Client/Server API (3) → easy to use
5. Простые примеры (4) → basic client/server
6. Базовая документация (6) → quick start
7. Выпуск v0.1.0 (9)

### Второстепенный (после MVP):
- TLS support
- Compression
- Extended examples
- Comprehensive tests
- Full documentation

---

## 4. Примерный график

| Эtap | Длительность | Приоритет |
|------|--------------|-----------|
| 0. Подготовка | 2 дня | Critical |
| 1. Ядро | 5 дней | Critical |
| 2. Connection | 4 дня | Critical |
| 3. Client/Server | 4 дня | Critical |
| 4. Расширения | 3 дня | High |
| 5. Примеры | 3 дня | Medium |
| 6. Тесты | 6 дней | Critical |
| 7. Документация | 7 дней | High |
| 8. Сравнение | 3 дня | Low |
| **Итого** | **37 дней** | |

**С учетом параллелизма и накладных расходов:** ~6-8 недель для v0.1.0

---

## 5. Ключевые решения архитектуры

### 5.1 Header-only vs Static Library
**Решение:** Header-only для простоты использования, с опциональной static library для production.

### 5.2 Async I/O
**Решение:** Boost.Asio как основа (максимальная переносимость), с возможностью использовать native sockets.

### 5.3 Memory Management
**Решение:** Минимальные аллокации через buffer pool и reusable buffers.

### 5.4 Error Handling
**Решение:** std::error_code для async API, exceptions для sync API.

### 5.5 Testing Strategy
**Решение:** 80%+ test coverage, integration tests с real WebSocket servers.

---

## 6. Следующие шаги

1. ✅ Создать репозиторий
2. ✅ Настроить CMake build
3. ✅ Начать реализацию этапа 1 (ядро протокола)
4. ✅ Добавить unit tests для frame parsing
5. ✅ Создать first working example

---

**Статус:** План утвержден, реализация готова к началу.

**Last updated:** 2026-06-06
