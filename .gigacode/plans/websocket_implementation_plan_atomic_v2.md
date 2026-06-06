# Подробный план реализации wscpp — Атомарные задачи (с OpenSSL и standalone ASIO)

## Обзор

Каждый этап разбит на атомарные задачи с четким результатом. Каждая задача должна быть завершена и протестирована перед переходом к следующей.

**Технический стек:**
- C++11 minimum (C++14/17/20 optional)
- OpenSSL 1.0.1+ (для TLS/SSL)
- standalone ASIO 1.13.0+ (для async I/O)
- CMake 3.20+
- Google Test 1.10+ (для тестов)
- Doxygen 1.8+ (для документации)
- GitVerse (репозиторий)

---

## Этап 0: Настройка проекта (8 задач)

### T0.1: Создать структуру директорий
**Результат:** Структура папок для проекта
```
wscpp/
├── include/wscpp/
│   ├── frame.hpp
│   ├── error.hpp
│   ├── buffer.hpp
│   ├── async.hpp
│   ├── connection.hpp
│   ├── handshake.hpp
│   ├── client.hpp
│   ├── server.hpp
│   └── extensions/
├── src/
│   ├── frame.cpp
│   ├── buffer.cpp
│   ├── async.cpp
│   ├── connection.cpp
│   ├── handshake.cpp
│   ├── client.cpp
│   └── server.cpp
├── tests/
│   ├── unit/
│   ├── integration/
│   └── regression/
├── examples/
│   ├── simple_client/
│   ├── simple_server/
│   ├── echo_server/
│   ├── tls_client/
│   └── tls_server/
├── docs/
│   ├── user-guide/
│   └��─ developer-guide/
├── cmake/
│   ├── FindOpenSSL.cmake
│   ├── FindASIO.cmake
│   └── wscpp-config.cmake.in
├── benchmarks/
├── .gitverse/
│   ├── workflows/
│   └── ISSUE_TEMPLATE/
├── .gitignore
├── CMakeLists.txt
├── README.md
├── CONTRIBUTING.md
├── CHANGELOG.md
└── LICENSE
```
**Критерий успеха:** Все папки созданы, `ls -la` показывает правильную структуру

### T0.2: Создать .gitignore
**Результат:** Файл `.gitignore` с правилами для C++ проектов
**Критерий успеха:** `git status` не показывает IDE файлы и build артефакты
```
/build/
*.log
*.pid
*.gcov
*.info
*.orig
*.rej
*.swp
*.swo
*~
.vscode/
.idea/
*.sln
*.vcxproj
*.vcxproj.filters
*.suo
*.user
*.aps
*.ncb
*.opendb
*.db
```

### T0.3: Создать LICENSE
**Результат:** Файл `LICENSE` с MIT лицензией
**Критерий успеха:** Лицензия читается, содержит все необходимые пункты

### T0.4: Создать README.md
**Результат:** Базовый README.md с описанием проекта
**Критерий успеха:** README содержит: название, краткое описание, статус, ссылки на документацию, примеры

### T0.5: Создать CONTRIBUTING.md
**Результат:** Файл `CONTRIBUTING.md` с инструкциями по вкладу
**Критерий успеха:** CONTRIBUTING содержит: как сообщить об ошибке, как предложить фичу, как создать PR, process для GitVerse

### T0.6: Создать CHANGELOG.md
**Результат:** Файл `CHANGELOG.md` с шаблоном для изменений
**Критерий успеха:** CHANGELOG содержит: заголовок, формат версий, типы изменений (Added, Changed, Deprecated, Removed, Fixed, Security)

### T0.7: Создать CMakeLists.txt (базовый)
**Результат:** Базовый `CMakeLists.txt` с настройками сборки
**Критерий успеха:** `cmake . && make` не падает с ошибкой

```cmake
cmake_minimum_required(VERSION 3.20)
project(wscpp VERSION 0.1.0 LANGUAGES CXX)

# Опции сборки
option(WSCPP_BUILD_TESTS "Build tests" ON)
option(WSCPP_BUILD_EXAMPLES "Build examples" ON)
option(WSCPP_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(WSCPP_WITH_OPENSSL "Enable OpenSSL support" ON)
option(WSCPP_USE_STANDALONE_ASIO "Use standalone ASIO" ON)

# Проверка версии C++
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
endif()

# Поиск зависимостей
find_package(OpenSSL REQUIRED)
find_package(ASIO REQUIRED)

# Установка заголовков
install(DIRECTORY include/wscpp DESTINATION include)
```

### T0.8: Создать CI/CD workflow для GitVerse
**Результат:** `.gitverse/workflows/ci.yml` с базовой сборкой
**Критерий успеха:** GitVerse Actions показывает статус сборки

---

## Этап 1: Базовые типы и буферы (4 задачи)

### T1.1: Создать include/wscpp/frame.hpp
**Результат:** `frame.hpp` с enum opcode и struct frame_header
```cpp
#ifndef WSCPP_FRAME_HPP
#define WSCPP_FRAME_HPP

#include <cstdint>
#include <array>
#include <vector>

namespace wscpp {

enum class opcode : std::uint8_t {
    continuation = 0x0,
    text = 0x1,
    binary = 0x2,
    close = 0x8,
    ping = 0x9,
    pong = 0xA
};

using mask_key = std::array<std::uint8_t, 4>;

struct frame_header {
    bool fin;
    bool rsv1, rsv2, rsv3;
    opcode op;
    bool masked;
    std::uint64_t payload_length;
    mask_key masking_key;
};

using buffer = std::vector<std::uint8_t>;

} // namespace wscpp

#endif // WSCPP_FRAME_HPP
```
**Критерий успеха:** Компилируется, 100% coverage

### T1.2: Создать include/wscpp/buffer.hpp
**Результат:** `buffer.hpp` с buffer pool и утилитами
```cpp
#ifndef WSCPP_BUFFER_HPP
#define WSCPP_BUFFER_HPP

#include <vector>
#include <cstdint>
#include <memory>

namespace wscpp {

using buffer = std::vector<std::uint8_t>;

class buffer_pool {
public:
    static buffer_pool& instance();
    buffer acquire(std::size_t size);
    void release(buffer&& buf);
private:
    buffer_pool();
};

} // namespace wscpp

#endif // WSCPP_BUFFER_HPP
```
**Критерий успеха:** Компилируется, тесты pool

### T1.3: Создать include/wscpp/error.hpp
**Результат:** `error.hpp` с error codes и exceptions
```cpp
#ifndef WSCPP_ERROR_HPP
#define WSCPP_ERROR_HPP

#include <system_error>
#include <stdexcept>
#include <string>

namespace wscpp {

enum class websocket_error_code {
    normal_closure = 1000,
    going_away = 1001,
    protocol_error = 1002,
    unsupported_data = 1003,
    no_status_received = 1005,
    abnormal_closure = 1006,
    invalid_payload_data = 1007,
    policy_violation = 1008,
    message_too_big = 1009,
    missing_extension = 1010,
    internal_error = 1011,
    tls_handshake_failure = 1015
};

const std::error_category& websocket_category();

inline std::error_code make_error_code(websocket_error_code e) {
    return std::error_code(static_cast<int>(e), websocket_category());
}

class websocket_exception : public std::runtime_error {
public:
    explicit websocket_exception(const std::string& msg);
    explicit websocket_exception(const std::error_code& ec);
    const std::error_code& code() const noexcept;
private:
    std::error_code m_ec;
};

} // namespace wscpp

#endif // WSCPP_ERROR_HPP
```
**Критерий успеха:** Компилируется, тесты error codes

### T1.4: Создать tests/unit/test_base_types.cpp
**Результат:** Unit tests для base types
**Критерий успеха:** 100% coverage, протокол RFC 5.2 test vectors проходят

---

## Этап 2: Frame parsing и serialization (6 задач)

### T2.1: Создать include/wscpp/frame_parser.hpp
**Результат:** `frame_parser.hpp` с классом frame_parser
```cpp
#ifndef WSCPP_FRAME_PARSER_HPP
#define WSCPP_FRAME_PARSER_HPP

#include "frame.hpp"
#include <cstdint>

namespace wscpp {

class frame_parser {
public:
    frame_parser();
    
    bool parse_header(const buffer& data, frame_header& header);
    bool parse_masking_key(const buffer& data, mask_key& key);
    bool parse_payload_length(const buffer& data, std::uint64_t& length);
    
    bool parse(const buffer& data, frame_header& header, buffer& payload);
    
    bool needs_more_data() const { return m_needs_more_data; }
    std::size_t bytes_needed() const { return m_bytes_needed; }
    
private:
    bool m_needs_more_data;
    std::size_t m_bytes_needed;
    std::size_t m_state;
};

} // namespace wscpp

#endif // WSCPP_FRAME_PARSER_HPP
```
**Критерий успеха:** Компилируется, тесты парсинга

### T2.2: Создать src/frame_parser.cpp
**Результат:** `frame_parser.cpp` с реализацией парсинга
**Критерий успеха:** RFC 5.2 test vectors проходят

### T2.3: Создать include/wscpp/frame_builder.hpp
**Результат:** `frame_builder.hpp` с классом frame_builder
```cpp
#ifndef WSCPP_FRAME_BUILDER_HPP
#define WSCPP_FRAME_BUILDER_HPP

#include "frame.hpp"

namespace wscpp {

class frame_builder {
public:
    frame_builder();
    
    buffer build_header(const frame_header& header);
    buffer build_masking_key(const mask_key& key);
    buffer build(const frame_header& header, const buffer& payload);
    
    void set_masking(bool enabled) { m_masking = enabled; }
    
private:
    bool m_masking;
};

} // namespace wscpp

#endif // WSCPP_FRAME_BUILDER_HPP
```
**Критерий успеха:** Компилируется, тесты сборки

### T2.4: Создать src/frame_builder.cpp
**Результат:** `frame_builder.cpp` с реализацией сборки
**Критерий успеха:** RFC 5.2 test vectors проходят

### T2.5: Создать tests/unit/test_frame_parser.cpp
**Результат:** Unit tests для frame_parser
**Критерий успеха:** 100% coverage, 100+ test cases

### T2.6: Создать tests/unit/test_frame_builder.cpp
**Результат:** Unit tests для frame_builder
**Критерий успеха:** 100% coverage, тесты компиляции и парсинга

---

## Этап 3: Masking (4 задачи)

### T3.1: Создать include/wscpp/masking.hpp
**Результат:** `masking.hpp` с функциями mask/unmask
```cpp
#ifndef WSCPP_MASKING_HPP
#define WSCPP_MASKING_HPP

#include "frame.hpp"

namespace wscpp {

void mask(buffer& data, const mask_key& key);
void unmask(buffer& data, const mask_key& key);

} // namespace wscpp

#endif // WSCPP_MASKING_HPP
```
**Критерий успеха:** RFC 5.3 test vectors проходят

### T3.2: Создать src/masking.cpp
**Результат:** `masking.cpp` с реализацией
**Критерий успеха:** RFC 5.3 test vectors проходят

### T3.3: Создать tests/unit/test_masking.cpp
**Результат:** Unit tests для masking
**Критерий успеха:** RFC 5.3 test vectors проходят, 100% coverage

### T3.4: Создать benchmarks/masking.cpp
**Результат:** Benchmark для masking
**Критерий успеха:** Показывает время на 1MB данных

---

## Этап 4: Handshake (8 задач)

### T4.1: Создать include/wscpp/base64.hpp
**Результат:** `base64.hpp` с encoder/decoder
**Критерий успеха:** RFC 4.2 test vectors проходят

### T4.2: Создать src/base64.cpp
**Результат:** `base64.cpp` с реализацией
**Критерий успеха:** RFC 4.2 test vectors проходят

### T4.3: Создать include/wscpp/sha1.hpp
**Результат:** `sha1.hpp` с hash функцией (через OpenSSL)
**Критерий успеха:** RFC 4.2 test vectors проходят

### T4.4: Создать src/sha1.cpp
**Результат:** `sha1.cpp` с реализацией
**Критерий успеха:** RFC 4.2 test vectors проходят

### T4.5: Создать include/wscpp/handshake.hpp
**Результат:** `handshake.hpp` с классами handshake_request и handshake_response
**Критерий успеха:** Компилируется, все необходимые поля

### T4.6: Создать src/handshake.cpp
**Результат:** `handshake.cpp` с реализацией
**Критерий успеха:** RFC 4.1, 4.2 test vectors проходят

### T4.7: Создать tests/unit/test_handshake.cpp
**Результат:** Unit tests для handshake
**Критерий успеха:** RFC 4.2 test vectors проходят, 100% coverage

### T4.8: Создать tests/integration/test_handshake_server.cpp
**Результат:** Integration test с реальным сервером
**Критерий успеха:** Успешный handshake с echo.websocket.org

---

## Этап 5: Connection API (10 задач)

### T5.1: Создать include/wscpp/connection.hpp
**Результат:** `connection.hpp` с абстрактным классом websocket_connection
**Критерий успеха:** Компилируется, все методы объявлены

### T5.2: Создать include/wscpp/client_connection.hpp
**Результат:** `client_connection.hpp` с реализацией client_connection
**Критерий успеха:** Компилируется, connect() и send() работают

### T5.3: Создать include/wscpp/server_connection.hpp
**Результат:** `server_connection.hpp` с реализацией server_connection
**Критерий успеха:** Компилируется, accept() и send() работают

### T5.4: Создать src/connection.cpp
**Результат:** `connection.cpp` с реализацией connection
**Критерий успеха:** Unit tests проходят

### T5.5: Создать src/client_connection.cpp
**Результат:** `client_connection.cpp` с реализацией client
**Критерий успеха:** Unit tests проходят

### T5.6: Создать src/server_connection.cpp
**Результат:** `server_connection.cpp` с реализацией server
**Критерий успеха:** Unit tests проходят

### T5.7: Создать tests/unit/test_connection.cpp
**Результат:** Unit tests для connection
**Критерий успеха:** 100% coverage, 100+ test cases

### T5.8: Создать tests/integration/test_connection.cpp
**Результат:** Integration test с echo.websocket.org
**Критерий успеха:** Успешный обмен сообщениями

### T5.9: Создать benchmarks/connection.cpp
**Результат:** Benchmark для connection
**Критерий успеха:** Показывает round-trip latency

### T5.10: Создать examples/simple_client/main.cpp
**Результат:** Простой клиент
**Критерий успеха:** Компилируется и работает

---

## Этап 6: Client API (5 задач)

### T6.1: Создать include/wscpp/client.hpp
**Результат:** `client.hpp` с websocket_client
**Критерий успеха:** Компилируется, connect() и send() работают

### T6.2: Создать src/client.cpp
**Результат:** `client.cpp` с реализацией
**Критерий успеха:** Unit tests проходят

### T6.3: Создать tests/unit/test_client.cpp
**Результат:** Unit tests для client
**Критерий успеха:** 100% coverage

### T6.4: Создать examples/simple_client/main.cpp (обновление)
**Результат:** Обновленный пример
**Критерий успеха:** Работает с echo.websocket.org

### T6.5: Создать examples/tls_client/main.cpp
**Результат:** TLS клиент
**Критерий успеха:** Работает с wss://echo.websocket.org

---

## Этап 7: Server API (5 задач)

### T7.1: Создать include/wscpp/server.hpp
**Результат:** `server.hpp` с websocket_server
**Критерий успеха:** Компилируется, listen() работает

### T7.2: Создать src/server.cpp
**Результат:** `server.cpp` с реализацией
**Критерий успеха:** Unit tests проходят

### T7.3: Создать tests/unit/test_server.cpp
**Результат:** Unit tests для server
**Критерий успеха:** 100% coverage

### T7.4: Создать examples/echo_server/main.cpp
**Результат:** Echo сервер
**Критерий успеха:** Компилируется и эхует сообщения

### T7.5: Создать examples/tls_server/main.cpp
**Результат:** TLS сервер
**Критерий успеха:** Работает с TLS

---

## Этап 8: Расширения (6 задач)

### T8.1: Создать include/wscpp/extensions/extension.hpp
**Результат:** `extension.hpp` с базовым классом extension
**Критерий успеха:** Компилируется

### T8.2: Создать include/wscpp/extensions/permessage_deflate.hpp
**Результат:** `permessage_deflate.hpp` с расширением
**Критерий успеха:** RFC 7692 test vectors проходят

### T8.3: Создать src/extensions/permessage_deflate.cpp
**Результат:** `permessage_deflate.cpp` с реализацией
**Критерий успеха:** RFC 7692 test vectors проходят

### T8.4: Создать tests/unit/test_compression.cpp
**Результат:** Unit tests для compression
**Критерий успеха:** 100% coverage

### T8.5: Создать examples/compression_client/main.cpp
**Результат:** Client с compression
**Критерий успеха:** Компилируется и использует compression

### T8.6: Создать examples/compression_server/main.cpp
**Результат:** Server с compression
**Критерий успеха:** Компилируется и использует compression

---

## Этап 9: Документация (10 задач)

### T9.1: Создать docs/user-guide/getting_started.md
**Результат:** Руководство по установке
**Критерий успеха:** CMake, vcpkg, conan описаны

### T9.2: Создать docs/user-guide/api_reference.md
**Результат:** API reference
**Критерий успеха:** Каждый класс описан

### T9.3: Создать docs/user-guide/examples.md
**Результат:** Сборник примеров
**Критерий успеха:** Каждый пример компилируется

### T9.4: Создать docs/developer-guide/architecture.md
**Результат:** Архитектура
**Критерий успеха:** Содержит диаграммы

### T9.5: Создать docs/developer-guide/coding_style.md
**Результат:** Стиль кода
**Критерий успеха:** Содержит правила

### T9.6: Создать docs/developer-guide/testing.md
**Результат:** Тестирование
**Критерий успеха:** CI/CD流程 описан

### T9.7: Обновить CONTRIBUTING.md
**Результат:** Обновленный CONTRIBUTING
**Критерий успеха:** Содержит GitVerse流程

### T9.8: Обновить README.md
**Результат:** Обновленный README
**Критерий успеха:** Содержит все ссылки

### T9.9: Создать docs/api-reference/
**Результат:** Doxygen документация
**Критерий успеха:** `doxygen Doxyfile` не падает

### T9.10: Создать docs/faq.md
**Результат:** FAQ
**Критерий успеха:** 10+ вопросов

---

## Этап 10: Интеграционные тесты (5 задач)

### T10.1: Создать tests/integration/test_echo_server.cpp
**Результат:** Test с echo.websocket.org
**Критерий успеха:** Работает с реальным сервером

### T10.2: Создать tests/integration/test_multiclient.cpp
**Результат:** Multi-client test
**Критерий успеха:** 10+ клиентов

### T10.3: Создать tests/integration/test_large_messages.cpp
**Результат:** Large messages test
**Критерий успеха:** 1MB+ сообщения

### T10.4: Создать tests/integration/test_fragmentation.cpp
**Результат:** Fragmentation test
**Критерий успеха:** Фрагментированные сообщения

### T10.5: Создать tests/integration/test_subprotocols.cpp
**Результат:** Subprotocol test
**Критерий успеха:** Subprotocol negotiation

---

## Этап 11: Бенчмарки (3 задачи)

### T11.1: Создать benchmarks/latency.cpp
**Результат:** Latency benchmark
**Критерий успеха:** Измеряет round-trip

### T11.2: Создать benchmarks/throughput.cpp
**Результат:** Throughput benchmark
**Критерий успеха:** Измеряет MB/s

### T11.3: Создать benchmarks/comparison.cpp
**Результат:** Comparison benchmark
**Критерий успеха:** Сравнивает с Boost.Beast

---

## Этап 12: Выпуск (5 задач)

### T12.1: Создать vcpkg portfile
**Результат:** `ports/wscpp/portfile.cmake`
**Критерий успеха:** `vcpkg install wscpp` работает

### T12.2: Создать conan recipe
**Результат:** `conanfile.py`
**Критерий успеха:** `conan install wscpp` работает

### T12.3: Создать GitVerse release notes
**Результат:** `v0.1.0` release notes
**Критерий успеха:** Содержит changelog

### T12.4: Создать Dockerfile
**Результат:** `Dockerfile` для CI
**Критерий успеха:** `docker build .` работает

### T12.5: Создать announcement
**Результат:** Объявление о релизе
**Критерий успеха:** Announce на Hacker News

---

## Итоговая статистика

- **Этап 0:** 8 задач
- **Этап 1:** 4 задачи
- **Этап 2:** 6 задач
- **Этап 3:** 4 задачи
- **Этап 4:** 8 задач
- **Этап 5:** 10 задач
- **Этап 6:** 5 задач
- **Этап 7:** 5 задач
- **Этап 8:** 6 задач
- **Этап 9:** 10 задач
- **Этап 10:** 5 задач
- **Этап 11:** 3 задачи
- **Этап 12:** 5 задач

**Всего:** 79 атомарных задач

**Ожидаемое время:** 79 дней (1 разработчик) → ~4-5 недель с параллелизмом

---

**Статус:** План готов к реализации

**Last updated:** 2026-06-06
