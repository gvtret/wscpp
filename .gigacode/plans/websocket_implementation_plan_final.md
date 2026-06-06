# Финальный план реализации легковесного WebSocket стека wscpp

## 1. Обзор проекта

**Название:** wscpp  
**Лицензия:** MIT  
**Целевая аудитория:** C++ разработчики, нуждающиеся в легковесном WebSocket клиенте/сервере  
**Платформы:** Windows, Linux, macOS  
**Компиляторы:** GCC 4.8+, Clang 3.3+, MSVC 2015+

---

## 2. Архитектура

### 2.1 Структура библиотеки

```
wscpp/
├── include/wscpp/
│   ├── websocket.hpp           # Main API (header-only)
│   ├── client.hpp              # WebSocket client
│   ├── server.hpp              # WebSocket server
│   ├── connection.hpp          # Connection interface
│   ├── frame.hpp               # Frame parsing/serialization
│   ├── handshake.hpp           # Handshake logic
│   ├── error.hpp               # Error codes
│   ├── buffer.hpp              # Buffer management
│   ├── async.hpp               # Async utilities
│   └── extensions/
│       ├── extension.hpp       # Extension interface
│       └── permessage_deflate.hpp
├── src/                        # Implementation files
│   ├── websocket.cpp
│   ├── client.cpp
│   ├── server.cpp
│   ├── connection.cpp
│   ├── frame.cpp
│   ├── handshake.cpp
│   ├── buffer.cpp
│   └── async.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── unit/
│   │   ├── test_frame.cpp
│   │   ├── test_masking.cpp
│   │   ├── test_handshake.cpp
│   │   └── test_error.cpp
│   ├── integration/
│   │   ├── test_real_server.cpp
│   │   └── test_websocket_echo.cpp
│   └── regression/
│       └── test_regression.cpp
├── examples/
│   ├── CMakeLists.txt
│   ├── simple_client/
│   │   └── main.cpp
│   ├── simple_server/
│   │   └── main.cpp
│   ├── echo_server/
│   │   └── main.cpp
│   ├── tls_client/
│   │   └── main.cpp
│   └── tls_server/
│       └── main.cpp
├── docs/
│   ├── user-guide/
│   │   ├── getting_started.md
│   │   ├── api_reference.md
│   │   ├── examples.md
│   │   └── faq.md
│   ├── developer-guide/
│   │   ├── architecture.md
│   │   ├── coding_style.md
│   │   └── testing.md
│   └── api-reference/          # Doxygen generated
├── cmake/
│   ├── wscpp-config.cmake.in
│   ├── FindOpenSSL.cmake
│   └── FindZLIB.cmake
├── benchmarks/
│   ├── CMakeLists.txt
│   ├── latency.cpp
│   └── throughput.cpp
├── .github/
│   ├── workflows/
│   │   ├── ci.yml
│   │   └── release.yml
│   └── ISSUE_TEMPLATE/
├── .gitignore
├── CMakeLists.txt
├── README.md
├── CONTRIBUTING.md
├── CHANGELOG.md
└── LICENSE
```

### 2.2 Технические решения

#### Сборка:
- **CMake minimum version:** 3.20
- **Build type:** Release/Debug/RelWithDebInfo
- **Standard:** C++11 (with options for C++14/17/20)
- **Installation:** CMake install target with config file

#### Зависимости:
| Тип | Библиотека | Мин. версия | Описание |
|-----|------------|-------------|----------|
| Обязательная | C++11 STL | - | Стандартная библиотека |
| Рекомендуемая | Boost.Asio | 1.55.0 | Async I/O |
| Опциональная | OpenSSL | 1.0.1 | TLS/SSL |
| Опциональная | zlib | 1.2.3 | Compression |

#### Подход к API:
- **Header-only для основного API** (websocket.hpp)
- **Static library для implementation** (опционально)
- **Два уровня API:**
  - Low-level (frame-based) — для экспертов
  - High-level (message-based) — для большинства

#### Стратегия async:
- **Callbacks** (Boost.Asio style) — основной паттерн
- **Synchronous blocking** — для простых случаев
- **Future/Promise** — через std::future (опционально)

---

## 3. Реализация по этапам

### Этап 0: Setup репозитория (дни 1-2)

**Задачи:**
- [x] Создать структуру директорий
- [x] Настроить CMakeLists.txt
- [x] Добавить .gitignore
- [x] Добавить LICENSE (MIT)
- [x] Добавить README.md
- [x] Добавить CONTRIBUTING.md
- [x] Настроить CI/CD (GitHub Actions)
  - Build на Ubuntu, macOS, Windows
  - GCC 4.8-11, Clang 3.3-14, MSVC 2017-2022
  - Unit tests
- [x] Настроить семантическую версионность

**CMake configuration:**
```cmake
cmake_minimum_required(VERSION 3.20)
project(wscpp VERSION 0.1.0 LANGUAGES CXX)

option(WSCPP_BUILD_TESTS "Build tests" ON)
option(WSCPP_BUILD_EXAMPLES "Build examples" ON)
option(WSCPP_BUILD_BENCHMARKS "Build benchmarks" OFF)
option(WSCPP_WITH_TLS "Enable TLS support" ON)
option(WSCPP_WITH_ZLIB "Enable compression support" ON)
option(WSCPP_USE_BOOST_ASIO "Use Boost.Asio" ON)
option(WSCPP_HEADER_ONLY "Build header-only version" ON)
```

---

### Этап 1: Ядро протокола (дни 3-8)

**Задачи:**

#### 1.1 Frame parsing/serialization (RFC 5.2)
- [ ] `frame_header` — структура заголовка
- [ ] `frame_parser` — парсинг входящих фреймов
- [ ] `frame_builder` — сборка исходящих фреймов
- [ ] Opcodes: TEXT (0x1), BINARY (0x2), CLOSE (0x8), PING (0x9), PONG (0xA)
- [ ] FIN bit handling
- [ ] Payload length encoding (7, 16, 64 bits)

#### 1.2 Masking (RFC 5.3)
- [ ] `mask_key` — структура ключа маскирования
- [ ] `mask()` — функция маскирования
- [ ] `unmask()` — функция размаскирования
- [ ] Тесты для всех размеров данных

#### 1.3 Fragmentation (RFC 5.4)
- [ ] `fragmented_message` — сборка фрагментированных сообщений
- [ ] Fragment buffer management
- [ ] Message reassembly

#### 1.4 Handshake (RFC 4.1, 4.2)
- [ ] `handshake_request` — клиентский запрос
- [ ] `handshake_response` — серверный ответ
- [ ] Base64 encoding/decoding для Sec-WebSocket-Key
- [ ] SHA-1 hashing для Sec-WebSocket-Accept
- [ ] Extension negotiation
- [ ] Subprotocol negotiation

#### 1.5 Error handling (RFC 7)
- [ ] `websocket_error` — enum для ошибок
- [ ] `websocket_exception` — исключения
- [ ] Close code handling (1000-1015)

**Ключевые классы:**
```cpp
namespace wscpp {

// Frame header structure
struct frame_header {
    bool fin;
    bool rsv1, rsv2, rsv3;
    opcode op;
    bool masked;
    std::uint64_t payload_length;
    mask_key masking_key;
};

// Frame parser
class frame_parser {
public:
    frame_parser();
    bool parse(const buffer& data, frame_header& header, buffer& payload);
    bool needs_more_data() const;
};

// Frame builder
class frame_builder {
public:
    frame_builder();
    buffer build(const frame_header& header, const buffer& payload);
};

// Handshake
class handshake {
public:
    static std::string generate_key();
    static std::string compute_accept(const std::string& key);
    static bool validate_response(const std::string& accept);
};

} // namespace wscpp
```

**Тесты:**
- Unit tests для frame parsing (100+ test cases)
- Unit tests для masking (все размеры: 0, 124, 125, 65535, 2^64-1)
- Integration tests с реальными WebSocket серверами

---

### Этап 2: Connection API (дни 9-13)

**Задачи:**

#### 2.1 Connection interface
- [ ] `websocket_connection` — базовый класс
- [ ] `connection_config` — конфигурация
- [ ] `connection_state` — enum состояний (CONNECTING, OPEN, CLOSING, CLOSED)

#### 2.2 Client connection
- [ ] `client_connection` — клиентское соединение
- [ ] `client_config` — конфигурация клиента
- [ ] Async connect
- [ ] Synchronous connect

#### 2.3 Server connection
- [ ] `server_connection` — серверное соединение
- [ ] `server_config` — конфигурация сервера
- [ ] Accept connection
- [ ] Reject connection

#### 2.4 Callback API
- [ ] `on_open()` — при открытии соединения
- [ ] `on_message()` — при получении сообщения
- [ ] `on_close()` — при закрытии соединения
- [ ] `on_error()` — при ошибке
- [ ] `on_ping()` / `on_pong()` — для control frames

#### 2.5 Synchronous API
- [ ] `send_sync()` — синхронная отправка
- [ ] `receive_sync()` — синхронное получение
- [ ] `connect_sync()` — синхронное подключение

**Ключевые классы:**
```cpp
namespace wscpp {

enum class state {
    CONNECTING,
    OPEN,
    CLOSING,
    CLOSED
};

class websocket_connection : public std::enable_shared_from_this<websocket_connection> {
public:
    virtual ~websocket_connection();
    
    virtual void connect() = 0;
    virtual void disconnect() = 0;
    
    virtual void send(const std::string& message) = 0;
    virtual void send(const buffer& data) = 0;
    
    virtual void on_open(std::function<void()> callback) = 0;
    virtual void on_message(std::function<void(const std::string&)> callback) = 0;
    virtual void on_close(std::function<void(close_code)> callback) = 0;
    virtual void on_error(std::function<void(error_code)> callback) = 0;
    
    state get_state() const;
    
protected:
    websocket_connection(connection_config config);
    
private:
    state m_state;
    connection_config m_config;
    std::function<void()> m_on_open;
    std::function<void(const std::string&)> m_on_message;
    std::function<void(close_code)> m_on_close;
    std::function<void(error_code)> m_on_error;
};

class client_connection : public websocket_connection {
public:
    client_connection(const client_config& config);
    
    void connect() override;
    void disconnect() override;
    
    void send(const std::string& message) override;
    void send(const buffer& data) override;
    
private:
    client_config m_config;
    // Async handlers
};

class server_connection : public websocket_connection {
public:
    server_connection(const server_config& config);
    
    void accept();
    void reject();
    
private:
    server_config m_config;
    // Server-specific state
};

} // namespace wscpp
```

**Тесты:**
- Unit tests для connection lifecycle
- Integration tests с echo.websocket.org
- Tests для timeout и error cases

---

### Этап 3: Client и Server API (дни 14-17)

**Задачи:**

#### 3.1 WebSocket client
- [ ] `websocket_client` — удобный клиент
- [ ] `client_config` — конфигурация клиента
- [ ] Поддержка ws:// и wss://
- [ ] Subprotocol negotiation

#### 3.2 WebSocket server
- [ ] `websocket_server` — удобный сервер
- [ ] `server_config` — конфигурация сервера
- [ ] Port binding
- [ ] Multiple clients support

#### 3.3 Subprotocol negotiation
- [ ] Sec-WebSocket-Protocol header parsing
- [ ] Subprotocol selection
- [ ] `negotiate_subprotocol()`

#### 3.4 Extension negotiation
- [ ] Sec-WebSocket-Extensions header
- [ ] Extension negotiation
- [ ] Permessage-deflate support

**Ключевые классы:**
```cpp
namespace wscpp {

class websocket_client {
public:
    websocket_client();
    explicit websocket_client(const client_config& config);
    
    void connect(const std::string& url);
    void connect_async(const std::string& url, std::function<void(error_code)> callback);
    
    void disconnect();
    
    void send(const std::string& message);
    void send(const buffer& data);
    
    void on_open(std::function<void()> callback);
    void on_message(std::function<void(const std::string&)> callback);
    void on_close(std::function<void(close_code)> callback);
    void on_error(std::function<void(error_code)> callback);
    
    client_config get_config() const;

private:
    client_config m_config;
    std::shared_ptr<client_connection> m_connection;
};

class websocket_server {
public:
    websocket_server();
    explicit websocket_server(const server_config& config);
    
    void listen(int port);
    void listen_async(int port, std::function<void(error_code)> callback);
    
    void stop();
    
    void on_connection(std::function<void(std::shared_ptr<server_connection>)> callback);
    
    server_config get_config() const;

private:
    server_config m_config;
    // Server state
};

} // namespace wscpp
```

**Тесты:**
- Unit tests для client/server
- Integration tests с multi-client scenarios
- Tests для subprotocol negotiation

---

### Этап 4: Расширения (дни 18-21)

**Задачи:**

#### 4.1 Permessage-deflate (RFC 7692)
- [ ] `permessage_deflate_extension` — расширение сжатия
- [ ] `compression_context` — контекст сжатия
- [ ] Window bits negotiation
- [ ] Memory level negotiation

#### 4.2 Extension框架
- [ ] `extension` — базовый класс
- [ ] Extension negotiation в handshake
- [ ] Extension data handling

**Ключевые классы:**
```cpp
namespace wscpp {

class extension {
public:
    virtual ~extension();
    virtual std::string negotiate(const std::string& offered) = 0;
    virtual buffer compress(const buffer& data) = 0;
    virtual buffer decompress(const buffer& data) = 0;
};

class permessage_deflate_extension : public extension {
public:
    permessage_deflate_extension();
    
    std::string negotiate(const std::string& offered) override;
    buffer compress(const buffer& data) override;
    buffer decompress(const buffer& data) override;
    
    void set_window_bits(int bits);
    void set_memory_level(int level);

private:
    int m_window_bits;
    int m_memory_level;
    z_stream m_compress_stream;
    z_stream m_decompress_stream;
};

} // namespace wscpp
```

**Тесты:**
- Unit tests для compression/decompression
- Integration tests с сжатыми сообщениями

---

### Этап 5: Примеры (дни 22-25)

**Задачи:**

#### Простой клиент
```cpp
int main() {
    wscpp::websocket_client client;
    
    client.on_open([]() {
        std::cout << "Connected!" << std::endl;
        client.send("Hello, WebSocket!");
    });
    
    client.on_message([](const std::string& msg) {
        std::cout << "Received: " << msg << std::endl;
    });
    
    client.on_close([](wscpp::close_code code) {
        std::cout << "Closed with code: " << code << std::endl;
    });
    
    client.on_error([](wscpp::error_code ec) {
        std::cerr << "Error: " << ec.message() << std::endl;
    });
    
    client.connect("ws://echo.websocket.org");
    
    // Event loop
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

#### Простой сервер (Echo)
```cpp
int main() {
    wscpp::websocket_server server;
    
    server.on_connection([](std::shared_ptr<wscpp::server_connection> conn) {
        std::cout << "New connection!" << std::endl;
        
        conn->on_message([conn](const std::string& msg) {
            conn->send(msg); // Echo
        });
        
        conn->on_close([conn](wscpp::close_code code) {
            std::cout << "Connection closed" << std::endl;
        });
    });
    
    server.listen(8080);
    
    std::cout << "Server listening on port 8080..." << std::endl;
    
    // Keep running
    std::cin.get();
    
    return 0;
}
```

#### TLS клиент
```cpp
int main() {
    wscpp::client_config config;
    config.enable_tls();
    config.set_tls_verify(true);
    
    wscpp::websocket_client client(config);
    
    client.on_open([]() {
        std::cout << "Secure connection!" << std::endl;
    });
    
    client.connect("wss://echo.websocket.org");
    
    return 0;
}
```

---

### Этап 6: Тесты (дни 26-30)

**Юнит-тесты (Google Test):**
- [ ] `test_frame.cpp` — frame parsing/serialization
- [ ] `test_masking.cpp` — masking/unmasking
- [ ] `test_handshake.cpp` — handshake logic
- [ ] `test_error.cpp` — error handling
- [ ] `test_compression.cpp` — permessage-deflate

**Интеграционные тесты:**
- [ ] `test_real_server.cpp` — подключение к реальным серверам
- [ ] `test_websocket_echo.cpp` — тест с echo.websocket.org
- [ ] `test_multiclient.cpp` — multi-client scenarios

**Регрессионные тесты:**
- [ ] `test_regression.cpp` — известные б��ги

**CI/CD:**
- [ ] Ubuntu (GCC 4.8, 4.9, 5, 6, 7, 8, 9, 10, 11)
- [ ] macOS (Clang 12, 13, 14)
- [ ] Windows (MSVC 2017, 2019, 2022)
- [ ] Coverage reporting (Codecov)

---

### Этап 7: Документация (дни 31-35)

**Пользовательская документация:**

1. **Getting Started**
   - Установка (CMake, vcpkg, conan)
   - Быстрый старт (5 минут tutorial)
   - Примеры

2. **API Reference**
   - Классы и методы
   - Примеры использования
   - Лучшие практики

3. **FAQ**
   - Как использовать с CMake
   - Как включить TLS
   - Как включить compression

**Разработческая документация:**

1. **Architecture**
   - Общая архитектура
   - Потоки данных
   - Паттерны проектирования

2. **Coding Style**
   - Именование
   - Комментарии
   - Документация

3. **Testing**
   - Как писать тесты
   - CI/CD流程

**API Reference (Doxygen):**
- [ ] Все классы и методы
- [ ] Примеры кода
- [ ] Генерированная документация

---

### Этап 8: Сравнительный анализ (дни 36-38)

**Библиотеки для сравнения:**

| Feature | wscpp | Boost.Beast | uWebSockets | WebSocket++ | Poco::Net |
|---------|-------|-------------|-------------|-------------|-----------|
| C++11+ | ✓ | ✓ | ✓ | ✓ | ✓ |
| Header-only | ✓ | ✗ | ✓ | ✓ | ✗ |
| TLS | ✓ | ✓ | ✓ | ✓ | ✓ |
| Compression | ✓ | ✓ | ✓ | ✗ | ✗ |
| Async API | ✓ | ✓ | ✓ | ✗ | ✗ |
| Sync API | ✓ | ✗ | ✗ | ✓ | ✓ |
| Dependencies | minimal | Boost | none | Boost | Poco |
| License | MIT | Boost | Apache-2.0 | MIT | GPL/LGPL |
| Size (MB) | ~0.5 | ~10 | ~0.1 | ~1 | ~5 |
| Performance | High | High | Highest | Medium | Medium |

**Ключевые преимущества wscpp:**
1. **Minimal dependencies** — только C++11 STL и опционально Boost.Asio
2. **Header-only** — простота интеграции
3. **Two API levels** — low-level и high-level
4. **Full RFC 6455 compliance** — полное соответствие спецификации
5. **Modern C++** — C++11/14/17/20 support
6. **Cross-platform** — Windows, Linux, macOS
7. **Well documented** — comprehensive documentation
8. **Well tested** — unit, integration, regression tests

---

### Этап 9: Выпуск и поддержка (дни 39-40)

**Первый релиз (v0.1.0):**
- [ ] Final code review
- [ ] Documentation review
- [ ] Security audit
- [ ] Performance benchmarks
- [ ] Release notes
- [ ] GitHub Releases
- [ ] vcpkg portfile
- [ ] conan recipe

**Поддержка версий:**
- **Semver:** MAJOR.MINOR.PATCH
- **Backward compatibility:** Full for MINOR versions
- **Deprecation policy:** 2 versions deprecation period
- **Bug fixes:** Only for latest MINOR version

**Release流程:**
1. Update version in CMakeLists.txt
2. Update CHANGELOG.md
3. Create GitHub release
4. Update vcpkg
5. Update conan
6. Announce on relevant channels

---

## 4. График и приоритеты

### Критичный путь (для MVP):
1. Setup репозитория (Этап 0)
2. Ядро протокола (Этап 1) → frame parsing, handshake
3. Connection API (Этап 2) → basic send/receive
4. Client/Server API (Этап 3) → easy to use
5. Простые примеры (Этап 5) → basic client/server
6. Базовая документация (Этап 7) → quick start
7. Выпуск v0.1.0 (Этап 9)

### Второстепенный (после MVP):
- TLS support (уже в Этапе 1, но опционально)
- Compression (Этап 4)
- Extended examples (Этап 5)
- Comprehensive tests (Этап 6)
- Full documentation (Этап 7)

### Примерный график

| Этап | Длительность | Приоритет |
|------|--------------|-----------|
| 0. Setup | 2 дня | Critical |
| 1. Core Protocol | 6 дней | Critical |
| 2. Connection API | 5 дней | Critical |
| 3. Client/Server API | 4 дня | High |
| 4. Extensions | 4 дня | Medium |
| 5. Examples | 4 дня | Medium |
| 6. Tests | 5 дней | Critical |
| 7. Documentation | 5 дней | High |
| 8. Analysis | 3 дня | Low |
| 9. Release | 2 дня | Critical |
| **Итого** | **40 дней** | |

**С параллелизмом (2 разработчика):** ~3-4 недели для v0.1.0

---

## 5. Ключевые API примеры

### Клиент:
```cpp
wscpp::client client;
client.connect("ws://echo.websocket.org");
client.send("Hello, WebSocket!");
client.on_message([](const std::string& msg) {
    std::cout << "Received: " << msg << std::endl;
});
```

### Сервер:
```cpp
wscpp::server server;
server.on_connection([](wscpp::server::connection_ptr conn) {
    conn->on_message([](const std::string& msg) {
        conn->send(msg); // Echo
    });
});
server.listen(8080);
```

### С TLS:
```cpp
wscpp::client_config config;
config.enable_tls();
wscpp::client client(config);
client.connect("wss://echo.websocket.org");
```

---

## 6. Следующие шаги

1. ✅ Создать структуру репозитория
2. ✅ Настроить CMake build
3. ✅ Реализовать frame parsing (первый working test)
4. ✅ Добавить unit tests для frame parsing
5. ✅ Создать простой пример

---

**Статус:** Финальный план готов к реализации

**Last updated:** 2026-06-06
