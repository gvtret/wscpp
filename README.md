# wscpp

Легковесный WebSocket-стек на C++11 и выше с поддержкой TLS.

## Возможности

- C++11 и выше
- standalone ASIO для сетевых операций
- OpenSSL для TLS/SSL
- CMake сборка
- Полная документация
- Примеры клиента и сервера

## Установка

### Требования

- C++11 совместимый компилятор (GCC 4.8+, Clang 3.3+, MSVC 2015+)
- CMake 3.10+
- OpenSSL 1.0.1+
- standalone ASIO 1.18+

### Сборка из исходников

```bash
git clone https://gitverse.ru/project/wscpp.git
cd wscpp
mkdir build && cd build
cmake ..
make
make install
```

## Примеры

Смотрите `examples/` для примеров использования.

## Лицензия

MIT License - см. файл LICENSE для подробной информации.

## Документация

- [Пользовательская документация](docs/README.md)
- [Документация разработчика](docs/DEVELOPER.md)

## Сравнение с другими библиотеками

Смотрите `ANALYSIS.md` для сравнительного анализа с другими библиотеками.
