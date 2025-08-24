# cpp-middle-project-sprint-2-dynamic <!-- omit in toc -->

- [Начало работы](#начало-работы)
- [Сборка проекта и запуск тестов](#сборка-проекта-и-запуск-тестов)
  - [Команды для сборки проекта](#команды-для-сборки-проекта)
  - [Команда для запуска тестов](#команда-для-запуска-тестов)


Шаблон репозитория для практического задания «Динамическая версия `scan`: интерпретация данных в runtime» 2-го спринта «Мидл разработчик С++»

## Начало работы

1. Нажмите зелёную кнопку `Use this template`, затем `Create a new repository`.
2. Назовите свой репозиторий.
3. Склонируйте созданный репозиторий командой `git clone your-repository-name`.
4. Создайте новую ветку командой `git switch -c development`.
5. Откройте проект в `Visual Studio Code`.
6. Нажмите `F1` и откройте проект в dev-контейнере командой `Dev Containers: Reopen in Container`.

## Сборка проекта и запуск тестов

Данный репозиторий использует **cmake** — генератор систем сборки для C и C++. Позволяет создавать проекты, которые могут компилироваться на различных платформах и с различными компиляторами. Подробнее о cmake:
  - https://dzen.ru/a/ZzZGUm-4o0u-IQlb
  - https://neerc.ifmo.ru/wiki/index.php?title=CMake_Tutorial
  - https://cmake.org/cmake/help/book/mastering-cmake/cmake/Help/guide/tutorial/index.html

### Команды для сборки проекта

```bash
mkdir build ; cd build

# Вызывается один раз перед сборкой проекта
cmake ..

# Вызывается каждый раз, когда необходимо собрать проект
make -j4
```

### Команда для запуска тестов

```bash
cd build
ctest --verbose
```

## scan<Ts...>: ключевые возможности

Библиотека header-only предоставляет compile-time template-функцию `scan<Ts...>(std::string_view input, std::string_view format)`, которая интерпретирует runtime-строку и извлекает типизированные значения в `std::tuple`. API по умолчанию безопасен и возвращает `std::expected<details::scan_result<Ts...>, details::scan_error>`.

- Поддерживаемые целевые типы:
  - Целые: `signed char`, `unsigned char`, `short int`, `unsigned short int`, `int`, `unsigned int`, `long long int`, `unsigned long long int`
  - С плавающей точкой: `float`, `double`
  - Строковые: `std::string`, `std::string_view`, `const char*`
- Поддерживаемые placeholders в строке `format`:
  - `{}`: пустой placeholder. Фрагмент входной строки интерпретируется в зависимости от целевого типа `T`.
  - `{%s}`: строковый placeholder. Допустимые цели: `std::string`, `std::string_view`, `const char*`.
  - `{%d}`: placeholder знакового целого. Допустимые цели: любой знаковый integral-тип.
  - `{%u}`: placeholder беззнакового целого. Допустимые цели: любой беззнаковый integral-тип.
  - `{%f}`: placeholder числа с плавающей точкой. Допустимые цели: `float`, `double`.
- Литералы вне placeholders должны в точности совпадать с входной строкой. Placeholders обрабатываются по порядку и сопоставляются с `T...`.

Детали возвращаемого значения:
- Успех: `scan` возвращает `{ .result = std::tuple<Ts...> }` в обертке `std::expected`.
- Ошибка: `scan` возвращает `std::unexpected(scan_error{message})` с описательным сообщением.

## Базовые примеры

```cpp
#include "scan.hpp"

// Single value, empty placeholder -> string
auto r1 = stdx::scan<std::string>("hello world", "{}");
// r1->result == std::tuple{"hello world"}

// Multiple values, mixed specifiers
auto r2 = stdx::scan<int, std::string, double>(
    "ID: 123 Name: Smith Score: 2.5",
    "ID: {%d} Name: {%s} Score: {%f}");
// r2->result == std::tuple{123, std::string{"Smith"}, 2.5}

// string_view capture
auto r3 = stdx::scan<std::string_view>("test_string_view", "{%s}");
// r3->result == std::tuple{std::string_view{"test_string_view"}}

// Unsigned integer
auto r4 = stdx::scan<unsigned int>("456", "{%u}");
// r4->result == std::tuple{456u}

// Float with empty placeholder
auto r5 = stdx::scan<float>("3.14159", "{}");
// r5->result == std::tuple{3.14159f}
```

## Ограничения и сообщения об ошибках

Функция выполняет строгую валидацию и возвращает ошибки через `scan_error{message}` в следующих случаях (не исчерпывающий список):

- Несоответствие количества аргументов:
  - Сообщение: `"Unexpected result. Mismatched number of format specifiers and target types"`
  - Когда число `{...}` в `format` отличается от числа template-типов `T...`.

- Несовпадение литерального текста между `input` и `format`:
  - Сообщение: `"Unexpected result. Unformatted text in input and format string are different"`
  - Любой статический текст вне placeholders должен совпадать точно.

- Неверный или неизвестный format specifier:
  - Сообщение: `"Unexpected result. Wrong or too long format specifier."` или `"Unexpected result. Unexpected format specifier."`

- Несоответствие типов для specifiers:
  - Для `%d` с не-знаковым integral-типом: `"Unexpected result. Type mismatch: 'd' specifier requires an integral type."`
  - Для `%u` с не-беззнаковым integral-типом: `"Unexpected result. Type mismatch: 'u' specifier requires a natural (unsigned integer) type."`
  - Для `%s` с не-строковым типом: `"Unexpected result. Type mismatch: 's' specifier requires a string-line type."`
  - Для `%f` с не-floating типом: `"Unexpected result. Type mismatch: 'f' specifier requires a floating type."`

- Ошибки конверсии (numeric parsing):
  - Примеры сообщений: `"Unexpected result.Failed to convert to <int>."`, `"Unexpected result.Failed to convert to <unsigned short int>."` и др.
  - Возникают, если подстрока не может быть распознана как требуемый числовой тип.

- Проверки диапазонов для пустого `{}` при числах:
  - Выход за диапазон для знакового целого: `"Unexpected result. Integer out of range for target type {}."`
  - Отрицательное значение для беззнакового типа: `"Unexpected result. Negative value parsed for unsigned type {}."`
  - Выход за диапазон для беззнакового целого: `"Unexpected result. Unsigned integer out of range for target type {}."`

- Проверки диапазонов для `%f` в `float`:
  - Сужение `double -> float` вне диапазона: `"Unexpected result. Double value out of range for float target type {}."`

Примечания по строковым типам:
- `std::string_view` возвращает view во входной буфер. Убедитесь, что исходная строка живет дольше view.
- `const char*` указывает на исходный буфер и может не иметь NUL-терминатора. Для безопасности предпочтительнее `std::string`/`std::string_view`, если вы не контролируете дальнейшее использование.
