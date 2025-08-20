#pragma once

#include <expected>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <cmath>
#include <limits>

#include "types.hpp"

namespace stdx::details {

using namespace std::literals;

// Функция-хелпер для конверсии строки в число.
template <typename T>
constexpr auto from_chars(const char* first, const char* last, T& value) {
    return std::from_chars(first, last, value);
}

// Перегрузка для double.
template <>
constexpr auto from_chars<double>(const char* first, const char* last, double& value) {
    return std::from_chars(first, last, value, std::chars_format::general);
}

// Поддержка целых чисел, чисел с плавающей точкой и натуральных чисел. Спецификаторы d,u,f.
template<typename T>
concept is_parsable = std::same_as<T, int> || std::same_as<T, double> || std::same_as<T, unsigned long long>;

template<is_parsable T>
constexpr std::expected<T, std::format_error> parse_value(std::string_view view) {
    T result {};
    auto [ptr, ec] = from_chars<T>(view.data(), view.data() + view.size(), result);

    if (ec != std::errc()) {
        if constexpr (std::same_as<T, int>) {
            return std::unexpected(std::format_error("Failed to convert into <int>."));
        } else if constexpr (std::same_as<T, double>) {
            return std::unexpected(std::format_error("Failed to convert into <double>."));
        } else if constexpr (std::same_as<T, unsigned long long>) {
            return std::unexpected(std::format_error("Failed to convert into <unsigned long long>."));
        }
    }
    return result;
}

// Поддержка спецификатора s: в исходной строке на месте плейсхолдера находится строка.
constexpr auto parse_string(std::string_view view) {
    return std::string {view};
}

template<typename T> constexpr std::expected<T, scan_error> process_empty_placeholder(std::string_view input) {
    if constexpr(std::is_constructible_v<T, std::string_view>) {
        // Поддержка std::string, std::string_view, const std::string и т.д.
        return T {input};
    }
    else if constexpr(is_integral<T>) {
        auto int_res = parse_value<int>(input);
        if(!int_res) {
            return std::unexpected(
                scan_error("Unexpected result. Failed to parse integer for {}: "s + int_res.error().what()));
        }
        return int_res.value();
    }
    else if constexpr(is_natural<T>) {
        auto natural_res = parse_value<unsigned long long>(input);
        if(!natural_res) {
            return std::unexpected(
                scan_error("Unexpected result. Failed to parse unsigned long long for {}: "s + natural_res.error().what()));
        }
        return natural_res.value();
    }
    else if constexpr(is_floating<T>) {
        auto floating_res = parse_value<double>(input);
        if(!floating_res) {
            return std::unexpected(
                scan_error("Unexpected result. Failed to parse float for {}: "s + floating_res.error().what()));
        }
        return floating_res.value();
    }
    else {
        return std::unexpected(scan_error("Unexpected result. Type not supported for {} placeholder."s));
    }
}

// Функция для парсинга значения с учетом спецификатора формата.
template<typename T>
constexpr std::expected<T, scan_error> parse_value_with_format(std::string_view input, std::string_view fmt) {
    // Если на внутри плейсхолдера пустая строка, то обработка данных в input на месте плейсхолдера как строки.
    if(fmt.empty()) {
        // Обработка данных в input на месте пустого {} placeholder.
        return process_empty_placeholder<T>(input);
    }
    // Невалидный префикс спецификатор формата, либо спецификатор формата больше одного символа.
    else if(fmt[0] == '%' && fmt.length() == 2) {
        // Обработка спецификаторов формата.
        switch(static_cast<unsigned char>(fmt[1])) {
            case 's': {
                if constexpr(is_c_string<T>) {
                    return reinterpret_cast<const char*>(input.data());
                }
                if constexpr(is_string<T> || is_string_view<T>) {
                    return parse_string(input);
                }
                else {
                    return std::unexpected(
                        scan_error("Unexpected result. Type mismatch: 's' specifier requires a string-line type."s));
                }
            }
            case 'd': {
                if constexpr(is_integral<T>) {
                    auto res = parse_value<int>(input);
                    if(!res) {
                        return std::unexpected(scan_error("Unexpected result."s + res.error().what()));
                    }
                    return res.value();
                }
                else {
                    return std::unexpected(
                        scan_error("Unexpected result. Type mismatch: 'd' specifier requires an integral type."s));
                }
            }
            case 'u': {
                if constexpr(is_natural<T>) {
                    auto res = parse_value<unsigned long long>(input);
                    if(!res) {
                        return std::unexpected(
                            scan_error("Unexpected result. Failed to parse natural for %u: "s + res.error().what()));
                    }
                    // Проверка на "обрезку" при конвертации.
                    auto parsed_ull = res.value();
                    if constexpr(sizeof(T) < sizeof(unsigned long long)) {
                        if(parsed_ull > static_cast<unsigned long long>(std::numeric_limits<T>::max())) {
                            return std::unexpected(
                                scan_error("Unexpected result. Unsigned integer out of range for target type %u."s));
                        }
                    }
                    // В этой точке тип T не будет "обрезан".
                    return static_cast<T>(parsed_ull);
                }
                else {
                    return std::unexpected(scan_error(
                        "Unexpected result. Type mismatch: 'u' specifier requires a natural (unsigned integer) type."s));
                }
            }
            case 'f': {
                if constexpr(is_floating<T>) {
                    auto res = parse_value<double>(input);
                    ;
                    if(!res) {
                        return std::unexpected(scan_error("Unexpected result. "s + res.error().what()));
                    }
                    return res.value();
                }
                else {
                    return std::unexpected(
                        scan_error("Unexpected result. Type mismatch: 'f' specifier requires a floating type."s));
                }
            }
            default:
                return std::unexpected(scan_error("Unexpected result. Unexpected format specifier."s));
        }
    }
    else {
        return std::unexpected(scan_error("Unexpected result. Wrong or too long format specifier."s));
    }
}

// Функция для проверки корректности входных данных и выделения из обеих строк интересующих данных для парсинга.
template<typename... Ts>
constexpr std::expected<std::pair<std::vector<std::string_view>, std::vector<std::string_view>>, scan_error>
parse_sources(std::string_view input, std::string_view format) {
    std::vector<std::string_view> format_parts;  // Части формата между {}
    std::vector<std::string_view> input_parts;
    size_t start = 0;
    while(true) {
        size_t open = format.find('{', start);
        if(open == std::string_view::npos) {
            break;
        }
        size_t close = format.find('}', open);
        if(close == std::string_view::npos) {
            break;
        }

        // Если между предыдущей } и текущей { есть текст,
        // проверяем его наличие во входной строке
        if(open > start) {
            std::string_view between = format.substr(start, open - start);
            auto pos                 = input.find(between);
            if(input.size() < between.size() || pos == std::string_view::npos) {
                return std::unexpected(
                    scan_error {"Unexpected result. Unformatted text in input and format string are different"});
            }
            if(start != 0) {
                input_parts.emplace_back(input.substr(0, pos));
            }

            input = input.substr(pos + between.size());
        }

        // Сохраняем спецификатор формата (то, что между {})
        format_parts.push_back(format.substr(open + 1, close - open - 1));
        start = close + 1;
    }

    // Проверяем оставшийся текст после последней }
    if(start < format.size()) {
        std::string_view remaining_format = format.substr(start);
        auto pos                          = input.find(remaining_format);
        if(input.size() < remaining_format.size() || pos == std::string_view::npos) {
            return std::unexpected(
                scan_error {"Unexpected result. Unformatted text in input and format string are different"});
        }
        input_parts.emplace_back(input.substr(0, pos));
        input = input.substr(pos + remaining_format.size());
    }
    else {
        input_parts.emplace_back(input);
    }
    return std::pair {format_parts, input_parts};
}
}  // namespace stdx::details
