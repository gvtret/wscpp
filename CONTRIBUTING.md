# Contributing to wscpp

Спасибо за ваш интерес к wscpp! Мы приветствуем вклад в развитие проекта.

**Статус проекта:** экспериментальная библиотека; исходный код создан с помощью ИИ (LLM-агенты). Перед использованием в продакшене нужна независимая ревизия и тестирование.

## Как внести вклад

1. Форкните репозиторий
2. Создайте ветку для вашей функции (`git checkout -b feature/amazing-feature`)
3. Закоммитьте изменения (`git commit -m 'Add some amazing feature'`)
4. Отправьте в ветку (`git push origin feature/amazing-feature`)
5. Откройте Pull Request

## Структура репозитория

- `src/` — исходный код библиотеки
- `include/` — заголовочные файлы
- `tests/` — unit, integration, regression tests
- `examples/` — ws:// and wss:// examples
- `docs/` — user guide, developer guide, HANDOFF log
- `benchmarks/` — performance harness (optional build)
- `AGENTS.md` — правила для ИИ-агентов, работающих с репозиторием

## Работа по плану разработки

При выполнении атомарных шагов плана:

1. `cmake --build build && ctest --timeout 30`
2. Append запись в `docs/HANDOFF.md` (дата, done, state, next)
3. Отдельный git commit с сообщением на английском

После изменений протокола (RFC gate) или новых compare-bench — обновите `ANALYSIS.md` и перезапустите `benchmarks/compare/` targets.

## Бенчмарки

```bash
cmake -B build -DWSCPP_BUILD_BENCHMARKS=ON
cmake --build build --target run_benchmarks
# or: benchmarks compare_benchmarks
ctest --timeout 30
```

См. `ANALYSIS.md` (методология) и `benchmarks/compare/README.md` (внешние библиотеки).

## TLS-тесты

Сертификаты **не хранятся в репозитории**. Перед прогоном:

```bash
bash scripts/gen-test-tls-certs.sh
cmake -B build -DWSCPP_BUILD_TESTS=ON && cmake --build build
cd build && ctest -R TlsIntegration --output-on-failure
```

Без fixtures тесты `TlsIntegration.*` пропускаются (`GTEST_SKIP`). Подробности: [docs/TLS.md](docs/TLS.md).

## Стандарты кода

- Используйте **`C++11`** или выше
- Следуйте стилю кода существующих файлов
- Добавляйте комментарии для сложных участков
- Пишите тесты для новых функций

## Связь

- Для вопросов открывайте Issues
- Для обсуждений используйте Pull Requests

## Лицензия

By contributing, you agree that your contributions will be licensed under the MIT License.
