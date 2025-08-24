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

## scan<Ts...>: key capabilities

The header-only library exposes a compile-time templated function `scan<Ts...>(std::string_view input, std::string_view format)` that interprets runtime strings and extracts typed values into a tuple. The API is safe-by-default and returns `std::expected<details::scan_result<Ts...>, details::scan_error>`.

- Supported target types:
  - Integers: `signed char`, `unsigned char`, `short int`, `unsigned short int`, `int`, `unsigned int`, `long long int`, `unsigned long long int`
  - Floating point: `float`, `double`
  - Strings: `std::string`, `std::string_view`, `const char*`
- Supported placeholders in the `format` string:
  - `{}`: empty placeholder. The input slice is interpreted based on the target type `T`.
  - `{%s}`: string placeholder. Allowed targets: `std::string`, `std::string_view`, `const char*`.
  - `{%d}`: signed integer placeholder. Allowed targets: any signed integral type.
  - `{%u}`: unsigned integer placeholder. Allowed targets: any unsigned integral type.
  - `{%f}`: floating placeholder. Allowed targets: `float`, `double`.
- Literal text outside placeholders must match the input exactly. Placeholders are processed in order and mapped to `T...`.

Return type details:
- On success: `scan` returns `{ .result = std::tuple<Ts...> }` wrapped in `std::expected`.
- On failure: `scan` returns `std::unexpected(scan_error{message})` with a descriptive message.

## Basic examples

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

## Restrictions and error messages

The function performs strict validation and returns failures with `scan_error{message}` in these situations (non-exhaustive):

- Mismatched arity:
  - Message: `"Unexpected result. Mismatched number of format specifiers and target types"`
  - When the count of `{...}` in `format` differs from the number of template types `T...`.

- Literal text mismatch between `input` and `format`:
  - Message: `"Unexpected result. Unformatted text in input and format string are different"`
  - Any static text outside placeholders must match exactly.

- Wrong or unknown format specifier:
  - Message: `"Unexpected result. Wrong or too long format specifier."` or `"Unexpected result. Unexpected format specifier."`

- Type mismatch for specifiers:
  - For `%d` with a non-signed integral target: `"Unexpected result. Type mismatch: 'd' specifier requires an integral type."`
  - For `%u` with a non-unsigned integral target: `"Unexpected result. Type mismatch: 'u' specifier requires a natural (unsigned integer) type."`
  - For `%s` with a non-string-like target: `"Unexpected result. Type mismatch: 's' specifier requires a string-line type."`
  - For `%f` with a non-floating target: `"Unexpected result. Type mismatch: 'f' specifier requires a floating type."`

- Conversion failures (numeric parsing):
  - Messages include: `"Unexpected result.Failed to convert to <int>."`, `"Unexpected result.Failed to convert to <unsigned short int>."`, etc.
  - Triggered when the substring cannot be parsed as the requested numeric type.

- Range checks for empty `{}` numeric captures:
  - Signed integer out of range: `"Unexpected result. Integer out of range for target type {}."`
  - Unsigned integer negative value: `"Unexpected result. Negative value parsed for unsigned type {}."`
  - Unsigned integer out of range: `"Unexpected result. Unsigned integer out of range for target type {}."`

- Range checks for `%f` into `float`:
  - Double-to-float narrowing out of range: `"Unexpected result. Double value out of range for float target type {}."`

Notes on string outputs:
- `std::string_view` returns a view into the original input buffer. Ensure the source string outlives the view.
- `const char*` points into the original buffer and may not be NUL-terminated. Prefer `std::string`/`std::string_view` for safety unless you control downstream usage.
