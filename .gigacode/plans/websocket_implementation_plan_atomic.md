# Подробный план реализации wscpp — Атомарные задачи

## Обзор

Каждый этап разбит на атомарные задачи с четким результатом. Каждая задача должна быть завершена и протестирована перед переходом к следующей.

---

## Этап 0: Настройка проекта (8 задач)

### T0.1: Создать структуру директорий
**Результат:** Структура папок для проекта
```
wscpp/
├── include/wscpp/
├── src/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── regression/
├── examples/
├── docs/
├── cmake/
├── benchmarks/
└── .github/workflows/
```
**Критерий успеха:** Все папки созданы, `ls -la` показывает правильную структуру

### T0.2: Создать .gitignore
**Результат:** Файл `.gitignore` с правилами для C++ проектов
**Критерий успеха:** `git status` не показывает IDE файлы и build артефакты

### T0.3: Создать LICENSE
**Результат:** Файл `LICENSE` с MIT лицензией
**Критерий успеха:** Лицензия читается, содержит все необходимые пункты

### T0.4: Создать README.md
**Результат:** Базовый README.md с описанием проекта
**Критерий успеха:** README содержит: название, краткое описание, статус, ссылки на документацию

### T0.5: Создать CONTRIBUTING.md
**Результат:** Файл `CONTRIBUTING.md` с инструкциями по вкладу
**Критерий успеха:** CONTRIBUTING содержит: как сообщить об ошибке, как предложить фичу, как создать PR

### T0.6: Создать CHANGELOG.md
**Результат:** Файл `CHANGELOG.md` с шаблоном для изменений
**Критерий успеха:** CHANGELOG содержит: заголовок, формат версий, типы изменений

### T0.7: Создать CMakeLists.txt (базовый)
**Результат:** Базовый `CMakeLists.txt` с настройками сборки
**Критерий успеха:** `cmake . && make` не падает с ошибкой

### T0.8: Создать CI/CD workflow
**Результат:** `.github/workflows/ci.yml` с базовой сборкой
**Критерий успеха:** GitHub Actions показывает статус сборки

---

## Этап 1: Ядро протокола — Frame (12 задач)

### T1.1: Создать enum opcode
**Результат:** `include/wscpp/frame.hpp` с enum `opcode`
```cpp
enum class opcode : std::uint8_t {
    continuation = 0x0,
    text = 0x1,
    binary = 0x2,
    close = 0x8,
    ping = 0x9,
    pong = 0xA
};
```
**Критерий успеха:** Компилируется, 100% coverage

### T1.2: Создать структуру frame_header
**Результат:** `frame_header` структура с полями из RFC 5.2
```cpp
struct frame_header {
    bool fin;
    bool rsv1, rsv2, rsv3;
    opcode op;
    bool masked;
    std::uint64_t payload_length;
    std::array<std::uint8_t, 4> masking_key;
};
```
**Критерий успеха:** Компилируется, размер структуры ≤ 16 байт

### T1.3: Реализовать frame_parser::parse_header
**Результат:** Метод `parse_header()` чтения заголовка фрейма
**Критерий успеха:** Тесты для всех размеров заголовков (2, 4, 10 байт)

### T1.4: Реализовать frame_parser::parse_payload_length
**Результат:** Метод `parse_payload_length()` для 7, 16, 64 битных длин
**Критерий успеха:** Тесты для длин: 0-125, 126-65535, >65535

### T1.5: Реализовать frame_parser::parse_masking_key
**Результат:** Метод `parse_masking_key()` чтения 4 байт masking key
**Критерий успеха:** Тесты для masked и unmasked фреймов

### T1.6: Реализовать frame_parser::parse_payload
**Результат:** Метод `parse_payload()` чтения payload данных
**Критерий успеха:** Тесты для пустого и большого payload

### T1.7: Создать frame_builder
**Результат:** Класс `frame_builder` с методами сборки фреймов
**Критерий успеха:** Компилируется, методы `build_header()`, `build_masking_key()`

### T1.8: Реализовать frame_builder::build_header
**Результат:** Метод сборки 2-байтного заголовка
**Критерий успеха:** Тесты для всех комбинаций fin/rsv/op

### T1.9: Реализовать frame_builder::build_long_length
**Результат:** Метод сборки 16 и 64 битных длин
**Критерий успеха:** Тесты для длин >125

### T1.10: Реализовать frame_builder::build_masking_key
**Результат:** Метод сборки masking key
**Критерий успеха:** Тесты генерации случайного ключа

### T1.11: Реализовать frame_builder::build
**Результат:** Метод сборки полного фрейма
**Критерий успеха:** Тесты компиляции и парсинга обратно

### T1.12: Создать unit tests для frame
**Результат:** `tests/unit/test_frame.cpp` с 50+ test cases
**Критерий успеха:** 100% coverage frame parsing/building

---

## Этап 2: Ядро протокола — Masking (4 задачи)

### T2.1: Создать функцию mask
**Результат:** `mask(data, key)` — маскирование данных
**Критерий успеха:** RFC 5.3 test vectors проходят

### T2.2: Создать функцию unmask
**Результат:** `unmask(data, key)` — размаскирование данных
**Критерий успеха:** Тесты `unmask(mask(data, key), key) == data`

### T2.3: Создать unit tests для masking
**Результат:** `tests/unit/test_masking.cpp` с 20+ test cases
**Критерий успеха:** Тесты RFC 5.3 test vectors

### T2.4: Создать benchmark для masking
**Результат:** `benchmarks/masking.cpp` с измерением скорости
**Критерий успеха:** Benchmark показывает время на 1MB данных

---

## Этап 3: Ядро протокола — Handshake (10 задач)

### T3.1: Создать uuid generation
**Результат:** `generate_uuid()` для Sec-WebSocket-Key
**Критерий успеха:** RFC 4.1 test vectors проходят

### T3.2: Создать base64 encoder/decoder
**Результат:** `base64_encode()` и `base64_decode()`
**Критерий успеха:** RFC 4.2 test vectors проходят

### T3.3: Создать sha1 hasher
**Результат:** `sha1_hash(data)` для Sec-WebSocket-Accept
**Критерий успеха:** RFC 4.2 test vectors проходят

### T3.4: Создать структуру handshake_request
**Результат:** `handshake_request` с полями заголовков
**Критерий успеха:** Компилируется, все необходимые поля

### T3.5: Создать структуру handshake_response
**Результат:** `handshake_response` с полями заголовков
**Критерий успеха:** Компилируется, все необходимые поля

### T3.6: Реализовать handshake_request::build
**Результат:** Метод сборки HTTP Upgrade запроса
**Критерий успеха:** Тесты генерации правильных заголовков

### T3.7: Реализовать handshake_response::parse
**Результат:** Метод парсинга HTTP Upgrade ответа
**Критерий успеха:** Тесты парсинга правильных ответов

### T3.8: Реализовать handshake_validator
**Результат:** Проверка правильности handshake
**Критерий успеха:** Тесты для валидных и невалидных ответов

### T3.9: Создать unit tests для handshake
**Результат:** `tests/unit/test_handshake.cpp` с 30+ test cases
**Критерий успеха:** RFC 4.2 test vectors проходят

### T3.10: Создать integration test для handshake
**Результат:** `tests/integration/test_handshake_server.cpp`
**Критерий успеха:** Успешный handshake с реальным сервером

---

## Этап 4: Ядро протокола — Error handling (3 задачи)

### T4.1: Создать error codes
**Результат:** `websocket_error` enum с кодами из RFC 7
**Критерий успеха:** Все коды 1000-1015, 3000-3999

### T4.2: Создать exception classes
**Результат:** `websocket_exception` и наследники
**Критерий успеха:** Компилируется, наследование работает

### T4.3: Создать unit tests для errors
**Результат:** `tests/unit/test_error.cpp` с 15+ test cases
**Критерий успеха:** 100% coverage error handling

---

## Этап 5: Connection API (10 задач)

### T5.1: Создать enum connection_state
**Результат:** `state` enum: CONNECTING, OPEN, CLOSING, CLOSED
**Критерий успеха:** Компилируется, 100% coverage

### T5.2: Создать connection_config
**Результат:** `connection_config` структура с настройками
**Критерий успеха:** Компилируется, все поля

### T5.3: Создать websocket_connection interface
**Результат:** `websocket_connection` абстрактный класс
**Критерий успеха:** Компилируется, все методы объявлены

### T5.4: Создать client_connection
**Результат:** `client_connection` реализация websocket_connection
**Критерий успеха:** Компилируется, connect() и send() работают

### T5.5: Создать server_connection
**Результат:** `server_connection` реализация websocket_connection
**Критерий успеха:** Компилируется, accept() и send() работают

### T5.6: Реализовать callback API
**Результат:** Методы `on_open`, `on_message`, `on_close`, `on_error`
**Критерий успеха:** Тесты вызовов callbacks

### T5.7: Реализовать synchronous API
**Результат:** `send_sync()`, `receive_sync()`, `connect_sync()`
**Критерий успеха:** Тесты блокирующего поведения

### T5.8: Создать unit tests для connection
**Результат:** `tests/unit/test_connection.cpp` с 40+ test cases
**Критерий успеха:** 100% coverage connection lifecycle

### T5.9: Создать integration test для connection
**Результат:** `tests/integration/test_connection.cpp`
**Критерий успеха:** Успешный обмен сообщениями

### T5.10: Создать benchmark для connection
**Результат:** `benchmarks/connection.cpp` с измерением latency
**Критерий успеха:** Benchmark показывает round-trip latency

---

## Этап 6: Client API (5 задач)

### T6.1: Создать websocket_client
**Результат:** `websocket_client` класс с удобным API
**Критерий успеха:** Компилируется, connect() и send() работают

### T6.2: Реализовать поддержку ws://
**Результат:** Парсинг ws:// URL и подключение
**Критерий успеха:** Тесты парсинга ws:// URLs

### T6.3: Реализовать поддержку wss://
**Результат:** Подключение через TLS (опционально)
**Критерий успеха:** Тесты подключения к wss://echo.websocket.org

### T6.4: Создать unit tests для client
**Результат:** `tests/unit/test_client.cpp` с 20+ test cases
**Критерий успеха:** 100% coverage client API

### T6.5: Создать example simple_client
**Результат:** `examples/simple_client/main.cpp`
**Критерий успеха:** Пример компилируется и работает

---

## Этап 7: Server API (5 задач)

### T7.1: Создать websocket_server
**Результат:** `websocket_server` класс с удобным API
**Критерий успеха:** Компилируется, listen() работает

### T7.2: Реализовать port binding
**Результат:** `listen(int port)` метод
**Критерий успеха:** Тесты на портах 8080, 8081, 0 (auto)

### T7.3: Реализовать on_connection callback
**Результат:** `on_connection()` для новых соединений
**Критерий успеха:** Тесты вызова callback при подключении

### T7.4: Создать unit tests для server
**Результат:** `tests/unit/test_server.cpp` с 20+ test cases
**Критерий успеха:** 100% coverage server API

### T7.5: Создать example echo_server
**Результат:** `examples/echo_server/main.cpp`
**Критерий успеха:** Пример компилируется и эхует сообщения

---

## Этап 8: Расширения (6 задач)

### T8.1: Создать extension interface
**Результат:** `extension` абстрактный класс
**Критерий успеха:** Компилируется, методы negotiate/compress/decompress

### T8.2: Реализовать permessage_deflate extension
**Результат:** `permessage_deflate_extension` класс
**Критерий успеха:** RFC 7692 test vectors проходят

### T8.3: Создать unit tests для compression
**Результат:** `tests/unit/test_compression.cpp` с 30+ test cases
**Критерий успеха:** 100% coverage compression

### T8.4: Реализовать extension negotiation
**Результат:** Метод negotiation в handshake
**Критерий успеха:** Тесты сжатого и несжатого соединения

### T8.5: Создать example compression_client
**Результат:** `examples/compression_client/main.cpp`
**Критерий успеха:** Пример компилируется и использует compression

### T8.6: Создать example compression_server
**Результат:** `examples/compression_server/main.cpp`
**Критерий успеха:** Пример компилируется и использует compression

---

## Этап 9: Документация (10 задач)

### T9.1: Создать user-guide/getting_started.md
**Результат:** Руководство по установке
**Критерий успеха:** Документация содержит: CMake, vcpkg, conan

### T9.2: Создать user-guide/api_reference.md
**Результат:** API reference для всех классов
**Критерий успеха:** Каждый класс имеет описание методов

### T9.3: Создать user-guide/examples.md
**Результат:** Сборник примеров с комментариями
**Критерий успеха:** Каждый пример компилируется

### T9.4: Создать developer-guide/architecture.md
**Результат:** Описание архитектуры
**Критерий успеха:** Содержит: диаграммы, потоки данных, паттерны

### T9.5: Создать developer-guide/coding_style.md
**Результат:** Руководство по стилю кода
**Критерий успеха:** Содержит: именование, комментарии, форматирование

### T9.6: Создать developer-guide/testing.md
**Результат:** Руководство по тестированию
**Критерий успеха:** Содержит: как писать тесты, CI/CD流程

### T9.7: Создать CONTRIBUTING.md обновление
**Результат:** Обновленный CONTRIBUTING с деталями
**Критерий успеха:** Содержит:流程 для PR, code review

### T9.8: Создать README.md обновление
**Результат:** Обновленный README с полным описанием
**Критерий успеха:** Содержит: все примеры, ссылки на документацию

### T9.9: Создать docs/api-reference/
**Результат:** Сгенерированная Doxygen документация
**Критерий успеха:** `doxygen Doxyfile` не падает с ошибкой

### T9.10: Создать docs/faq.md
**Результат:** FAQ с часто задаваемыми вопросами
**Критерий успеха:** 10+ вопросов с ответами

---

## Этап 10: Интеграционные тесты (5 задач)

### T10.1: Создать test с echo.websocket.org
**Результат:** `tests/integration/test_echo_server.cpp`
**Критерий успеха:** Протокол работает с реальным сервером

### T10.2: Создать test с multi-client
**Результат:** `tests/integration/test_multiclient.cpp`
**Критерий успеха:** 10+ клиентов одновременно

### T10.3: Создать test с large messages
**Результат:** `tests/integration/test_large_messages.cpp`
**Критерий успеха:** Сообщения 1MB+ работают

### T10.4: Создать test с fragmentation
**Результат:** `tests/integration/test_fragmentation.cpp`
**Критерий успеха:** Фрагментированные сообщения работают

### T10.5: Создать test с subprotocols
**Результат:** `tests/integration/test_subprotocols.cpp`
**Критерий успеха:** Subprotocol negotiation работает

---

## Этап 11: Бенчмарки (3 задачи)

### T11.1: Создать latency benchmark
**Результат:** `benchmarks/latency.cpp`
**Критерий успеха:** Измеряет round-trip latency

### T11.2: Создать throughput benchmark
**Результат:** `benchmarks/throughput.cpp`
**Критерий успеха:** Измеряет throughput в MB/s

### T11.3: Создать comparison benchmark
**Результат:** `benchmarks/comparison.cpp` с другими библиотеками
**Критерий успеха:** Сравнивает с Boost.Beast, WebSocket++

---

## Этап 12: Выпуск (5 задач)

### T12.1: Создать vcpkg portfile
**Результат:** `ports/wscpp/portfile.cmake`
**Критерий успеха:** `vcpkg install wscpp` работает

### T12.2: Создать conan recipe
**Результат:** `conanfile.py`
**Критерий успеха:** `conan install wscpp` работает

### T12.3: Создать GitHub release notes
**Результат:** `v0.1.0` release notes на GitHub
**Критерий успеха:** Содержит: changelog, ссылки на документацию

### T12.4: Создать Dockerfile для CI
**Результат:** `Dockerfile` для воспроизводимой сборки
**Критерий успеха:** `docker build .` работает

### T12.5: Создать announcement
**Результат:** Объявление о релизе
**Критерий успеха:** Announce на Hacker News, Reddit, etc.

---

## Итоговая статистика

- **Этап 0:** 8 задач
- **Этап 1:** 12 задач
- **Этап 2:** 4 задачи
- **Этап 3:** 10 задач
- **Этап 4:** 3 задачи
- **Этап 5:** 10 задач
- **Этап 6:** 5 задач
- **Этап 7:** 5 задач
- **Этап 8:** 6 задач
- **Этап 9:** 10 задач
- **Этап 10:** 5 задач
- **Этап 11:** 3 задачи
- **Этап 12:** 5 задач

**Всего:** 86 атомарных задач

**Ожидаемое время:** 86 дней (1 разработчик) → ~4-5 недель с параллелизмом

---

**Статус:** План готов к реализации

**Last updated:** 2026-06-06
