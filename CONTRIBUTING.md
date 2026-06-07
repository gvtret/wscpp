# Contributing to wscpp

Спасибо за ваш интерес к wscpp! Мы приветствуем вклад в развитие проекта.

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

## Работа по плану разработки

При выполнении атомарных шагов плана:

1. `cmake --build build && ctest`
2. Append запись в `docs/HANDOFF.md`
3. Отдельный git commit с сообщением на английском

## Стандарты кода

- Используйте C++11 или выше
- Следуйте стилю кода существующих файлов
- Добавляйте комментарии для сложных участков
- Пишите тесты для новых функций

## Связь

- Для вопросов открывайте Issues
- Для обсуждений используйте Pull Requests

## Лицензия

By contributing, you agree that your contributions will be licensed under the MIT License.
