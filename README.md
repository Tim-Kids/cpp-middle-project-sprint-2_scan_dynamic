# cpp-middle-project-sprint-2-dynamic <!-- omit in toc -->

- [Getting Started](#getting-started)
- [Build & Test](#build--test)
- [API](#api)
- [Examples](#examples)
- [Constraints](#constraints)

Template repository for the task “Dynamic `scan`: interpret data at runtime”.

## Getting Started

1. Click `Use this template` → `Create a new repository`.
2. Clone your repo: `git clone your-repository-name`.
3. Create a branch: `git switch -c development`.
4. Open in VS Code and run `Dev Containers: Reopen in Container`.

## Build & Test

```bash
mkdir build && cd build
cmake ..
make -j4
```

```bash
cd build
ctest --verbose
```

## API

```cpp
std::expected<details::scan_result<Ts...>, details::scan_error>
scan<Ts...>(std::string_view input, std::string_view format);
```

- Placeholders: `{}`, `{%s}`, `{%d}`, `{%u}`, `{%f}`.
- Targets:
  - integral: `signed/unsigned char`, `short`, `unsigned short`, `int`, `unsigned int`, `long long`, `unsigned long long`
  - floating: `float`, `double`
  - string-like: `std::string`, `std::string_view`, `const char*`
- Literals in `format` must match `input` exactly; placeholders are mapped left-to-right to `T...`.

Returns `std::tuple<Ts...>` on success, or `scan_error` with a message on failure.

## Examples

```cpp
auto a = stdx::scan<std::string>("hello world", "{}");
auto b = stdx::scan<int, std::string, double>(
    "ID: 123 Name: Smith Score: 2.5", "ID: {%d} Name: {%s} Score: {%f}");
auto c = stdx::scan<std::string_view>("test_string_view", "{%s}");
```

## Constraints

- Arity: number of placeholders must equal `sizeof...(Ts)`.
- Literals: any non-placeholder text must match `input`.
- Specifiers: type must match placeholder kind (`%d` integral, `%u` unsigned integral, `%f` floating, `%s` string-like).
- Conversion: numeric parsing may fail, e.g. “Failed to convert to <int>”.
- Ranges:
  - `{}` into integral types: range-checked (signed/unsigned, width-aware).
  - `%f` into `float`: `double -> float` out-of-range is rejected.
- String notes:
  - `std::string_view` references `input`; ensure lifetime.
  - `const char*` points into `input` and may be non NUL-terminated; prefer `std::string`/`std::string_view` unless you control consumers.
