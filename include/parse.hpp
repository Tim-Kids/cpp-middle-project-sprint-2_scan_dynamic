#pragma once

#include <charconv>
#include <concepts>
#include <expected>
#include <format>
#include <type_traits>
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
template<typename T> constexpr auto from_chars(const char* first, const char* last, T& value) {
    return std::from_chars(first, last, value);
}

// Перегрузка для double.
template<> constexpr auto from_chars<double>(const char* first, const char* last, double& value) {
    return std::from_chars(first, last, value, std::chars_format::general);
}

// Поддержка целых чисел, чисел с плавающей точкой и натуральных чисел. Спецификаторы d,u,f.
template<typename T>
concept is_parsable = std::same_as<T, signed char> ||
                      std::same_as<T, unsigned char> ||
                      std::same_as<T, short int> ||
                      std::same_as<T, unsigned short int> ||
                      std::same_as<T, int> ||
                      std::same_as<T, unsigned int> ||
                      std::same_as<T, long long int> ||
                      std::same_as<T, unsigned long long int> ||
                      std::same_as<T, float> ||
                      std::same_as<T, double>;

template<is_parsable T> constexpr std::expected<T, std::format_error> parse_value(std::string_view view) {
    T result {};
    auto [ptr, ec] = from_chars<T>(view.data(), view.data() + view.size(), result);

    if(ec != std::errc()) {
        if constexpr(std::same_as<T, signed char>) {
            return std::unexpected(std::format_error("Failed to convert to <signed char>."));
        }
        else if constexpr(std::same_as<T, unsigned char>) {
            return std::unexpected(std::format_error("Failed to convert to <unsigned char>."));
        }
        if constexpr(std::same_as<T, signed short int>) {
            return std::unexpected(std::format_error("Failed to convert to <signed short int>."));
        }
        else if constexpr(std::same_as<T, unsigned short int>) {
            return std::unexpected(std::format_error("Failed to convert to <unsigned short int>."));
        }
        else if constexpr(std::same_as<T, int>) {
            return std::unexpected(std::format_error("Failed to convert to <int>."));
        }
        else if constexpr(std::same_as<T, unsigned int>) {
            return std::unexpected(std::format_error("Failed to convert to <unsigned int>."));
        }
        else if constexpr(std::same_as<T, long long int>) {
            return std::unexpected(std::format_error("Failed to convert to <long long int>."));
        }
        else if constexpr(std::same_as<T, unsigned long long int>) {
            return std::unexpected(std::format_error("Failed to convert to <unsigned long long int>."));
        }
        else if constexpr(std::same_as<T, float>) {
            return std::unexpected(std::format_error("Failed to convert to <float>."));
        }
        else if constexpr(std::same_as<T, double>) {
            return std::unexpected(std::format_error("Failed to convert to <double>."));
        }
        else {
            return std::unexpected(std::format_error("Unxpected type T failed to get converted."));
        }
    }
    return result;
}

template<typename T> constexpr std::expected<T, scan_error> process_empty_placeholder(std::string_view input) {
    if constexpr(std::is_constructible_v<T, std::string_view>) {
        // Поддержка std::string, std::string_view, const std::string и т.д.
        return T {input};
    }
    else if constexpr(is_integral<T>) {
        auto int_res = parse_value<long long int>(input);
        if(!int_res) {
            return std::unexpected(
                scan_error("Unexpected result. Failed to parse integer for {}: "s + int_res.error().what()));
        }
        auto parsed_int = int_res.value();

        if constexpr(std::is_signed_v<std::remove_cv_t<T>>) {
            // Проверка на "обрезку" значений малоразмерных знаковых int.
            if constexpr(sizeof(T) < sizeof(long long int)) {  // Для int8_t, int16_t, int32_t, int64_t.
                // Значение должно укладываться в допустимый диапазон типа.
                if(parsed_int < static_cast<long long int>(std::numeric_limits<T>::min()) ||
                   parsed_int > static_cast<long long int>(std::numeric_limits<T>::max())) {
                    return std::unexpected(scan_error("Unexpected result. Integer out of range for target type {}."s));
                }
            }
        }
        else {  // Проверка беззнакового int.
            // Беззнаковый тип не может быть отрицатлеьным.
            if(parsed_int < 0) {
                return std::unexpected(scan_error("Unexpected result. Negative value parsed for unsigned type {}."s));
            }
            if constexpr(sizeof(T) < sizeof(unsigned long long int)) {  // Для uint8_t, uint16_t, uint32_t, uint64_t.
                // Значение должно укладываться в допустимый диапазон типа.
                if(static_cast<unsigned long long int>(parsed_int) >
                   static_cast<unsigned long long int>(std::numeric_limits<T>::max())) {
                    return std::unexpected(
                        scan_error("Unexpected result. Unsigned integer out of range for target type {}."s));
                }
            }
        }
        // В этой точке тип T не будет "обрезан".
        return static_cast<T>(parsed_int);
    }
    else if constexpr(is_floating<T>) {
        auto floating_res = parse_value<double>(input);
        if(!floating_res) {
            return std::unexpected(
                scan_error("Unexpected result. Failed to parse float for {}: "s + floating_res.error().what()));
        }
        double parsed_double = floating_res.value();

        // Проверка на "обрезку", если double кастуется к float (T = float, value at {%f} = double).
        if constexpr(std::is_same_v<std::remove_cv_t<T>, float>) {
            constexpr auto float_lowest = std::numeric_limits<T>::lowest();
            constexpr auto float_max    = std::numeric_limits<T>::max();
            if(parsed_double < static_cast<double>(std::numeric_limits<T>::lowest()) ||
               parsed_double > static_cast<double>(std::numeric_limits<T>::max())) {
                return std::unexpected(
                    scan_error("Unexpected result. Double value out of range for float target type {}."s));
            }
        }
        // В этой точке тип T не будет "обрезан".
        return static_cast<T>(parsed_double);
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
                if constexpr(is_string_view<T>) {
                    return static_cast<T>(input);
                }
                if constexpr(is_string<T>) {
                    return T{input};
                }
                // Intentionally drop C-string support to avoid dangling/non-null-terminated slices
                else {
                    return std::unexpected(
                        scan_error("Unexpected result. Type mismatch: 's' specifier requires a string-line type."s));
                }
            }
            case 'd': {
                if constexpr(is_integral<T>) {
                    auto res = parse_value<T>(input);
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
                    auto res = parse_value<T>(input);
                    if(!res) {
                        return std::unexpected(
                            scan_error("Unexpected result."s + res.error().what()));
                    }
                    return res.value();
                }
                else {
                    return std::unexpected(scan_error(
                        "Unexpected result. Type mismatch: 'u' specifier requires a natural (unsigned integer) type."s));
                }
            }
            case 'f': {
                if constexpr(is_floating<T>) {
                    auto res = parse_value<double>(input);
                    if(!res) {
                        return std::unexpected(scan_error("Unexpected result. "s + res.error().what()));
                    }
                    double parsed_double = res.value();
                    if constexpr(std::is_same_v<std::remove_cv_t<T>, float>) {
                        if(parsed_double < static_cast<double>(std::numeric_limits<T>::lowest()) ||
                           parsed_double > static_cast<double>(std::numeric_limits<T>::max())) {
                            return std::unexpected(
                                scan_error("Unexpected result. Double value out of range for float target type {}."s));
                        }
                    }
                    return static_cast<T>(parsed_double);
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
