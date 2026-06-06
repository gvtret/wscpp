# Обновленный план реализации WebSocket стека

## Уточненные требования

### Обязательные функции:
- RFC 6455 compliance (handshake, framing, masking)
- C++11 minimum
- Header-only библиотека (для простоты использования)
- CMake build system (v3.20+)
- Semver поддержка

### Опциональные функции:
- TLS/SSL (через OpenSSL)
- Compression (permessage-deflate)
- Subprotocols

### API style:
- Callback-based async (Boost.Asio style)
- Synchronous blocking (для простых случаев)
- Future/Promise (опционально)

### Тесты:
- Google Test
- Unit + Integration + Regression
- Integration с real WebSocket servers

### Документация:
- User guide (quick start, examples)
- Developer guide (architecture, contribution)
- API reference (Doxygen)

## Структура проекта

```
wscpp/
├── include/wscpp/
│   ├── websocket.hpp         # Main API
│   ├── client.hpp            # Client API
│   ├── server.hpp            # Server API
│   ├── connection.hpp
│   ├── frame.hpp
│   ├── handshake.hpp
│   ├── error.hpp
│   └── extensions/
│       └── permessage_deflate.hpp
├── src/
│   ├── websocket.cpp
│   ├── client.cpp
│   ├── server.cpp
│   ├── connection.cpp
│   ├── frame.cpp
│   └── handshake.cpp
├── tests/
│   ├── unit/
│   ├── integration/
│   └── regression/
├── examples/
│   ├── simple_client/
│   ├── simple_server/
│   └── echo_server/
├── docs/
│   ├── user-guide.md
│   ├── developer-guide.md
│   └── api-reference/
├── cmake/
│   └── wscpp-config.cmake.in
└── benchmarks/
```

## Реализация по этапам

### Этап 0: Setup (1-2 дня)
- [ ] Структура репозитория
- [ ] CMake build system
- [ ] CI/CD (GitHub Actions)
- [ ] .gitignore, LICENSE, README

### Этап 1: Core Protocol (3-5 дней)
- [ ] Frame parsing/serialization (RFC 5.2)
- [ ] Masking/unmasking (RFC 5.3)
- [ ] Opcodes (text, binary, close, ping, pong)
- [ ] Fragmentation (RFC 5.4)
- [ ] Handshake (RFC 4.1, 4.2)
- [ ] Error handling (RFC 7)

### Этап 2: Connection API (3-4 дня)
- [ ] websocket_connection interface
- [ ] client_connection
- [ ] server_connection
- [ ] Callback API
- [ ] Synchronous API

### Этап 3: Client/Server API (2-3 дня)
- [ ] websocket_client
- [ ] websocket_server
- [ ] Subprotocol negotiation
- [ ] Examples (simple client/server)

### Этап 4: Tests (3-4 дня)
- [ ] Unit tests (frame, handshake)
- [ ] Integration tests (real servers)
- [ ] Google Test integration

### Этап 5: Documentation (2-3 дня)
- [ ] User guide (quick start)
- [ ] API examples
- [ ] Developer guide

## Приоритеты для MVP

1. **Frame parsing/serialization** (базовая функциональность)
2. **Handshake** (установление соединения)
3. **Connection API** (send/receive)
4. **Simple client/server examples**
5. **Unit tests**
6. **Basic documentation**

## Технические решения

### Базовая архитектура:
- Header-only (для простоты)
- Boost.Asio для async I/O
- std::error_code для error handling
- std::function для callbacks

### Минимальные зависимости:
- C++11 STL
- Boost.Asio (опционально, но рекомендуется)

### Максимальная совместимость:
- C++11, C++14, C++17, C++20
- Windows, Linux, macOS
- GCC, Clang, MSVC

## Следующие шаги

1. Создать структуру репозитория
2. Настроить CMake build
3. Реализовать frame parsing (первый working test)
4. Добавить unit tests
5. Создать простой пример

## Примерный график

| Этап | Длительность | Приоритет |
|------|--------------|-----------|
| Setup | 2 дня | Critical |
| Core Protocol | 5 дней | Critical |
| Connection API | 4 дня | Critical |
| Client/Server | 3 дня | High |
| Tests | 4 дня | Critical |
| Docs | 3 дня | High |
| **Итого** | **21 день** | |

**С параллелизмом:** ~4-5 недель для v0.1.0

## Ключевые API примеры

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

## Сравнение с существующими библиотеками

| Feature | wscpp | Boost.Beast | uWebSockets | WebSocket++ |
|---------|-------|-------------|-------------|-------------|
| Header-only | ✓ | ✗ | ✓ | ✓ |
| C++11+ | ✓ | ✓ | ✓ | ✓ |
| TLS | ✓ | ✓ | ✓ | ✓ |
| Async API | ✓ | ✓ | ✓ | ✗ |
| Sync API | ✓ | ✗ | ✗ | ✓ |
| Dependencies | minimal | Boost | none | Boost |

---

**Статус:** Обновленный план готов к реализации

**Last updated:** 2026-06-06
