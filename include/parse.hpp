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

// Семейсвто функций parse_value.

// Поддержка спецификатора d: в исходной строке на месте плейсхолдера находится целое число.
constexpr std::expected<int, std::format_error> parse_integral(std::string_view view) {
    int result     = 0;
    auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), result);
    if(ec != std::errc()) {
        return std::unexpected(std::format_error("Failed to convert into <int>."s));
    }
    return result;
}

// Поддержка спецификатора f: — в исходной строке на месте плейсхолдера находится число с плавающей точкой.
constexpr std::expected<double, std::format_error> parse_floating(std::string_view view) {
    double result  = .0;
    auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), result, std::chars_format::general);
    if(ec != std::errc()) {
        return std::unexpected(std::format_error("Failed to convert into <double>."s));
    }
    return result;
}

// Поддержка спецификатора u: в исходной строке на месте плейсхолдера находится натуральное число.
constexpr std::expected<unsigned long long, std::format_error> parse_natural(std::string_view view) {
    unsigned long long result = 0;
    auto [ptr, ec]            = std::from_chars(view.data(), view.data() + view.size(), result);
    if(ec != std::errc()) {
        return std::unexpected(std::format_error("Failed to convert into <unsigned long long>."s));
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
    else if constexpr(isIntegral<T>) {
        auto int_res = parse_integral(input);
        if(!int_res) {
            return std::unexpected(
                scan_error("Unexpected result. Failed to parse integer for {}: "s + int_res.error().what()));
        }
        auto parsed_int = int_res.value();

        if constexpr(std::is_signed_v<std::remove_cv_t<T>>) {
            // Проверка на "обрезку" значений малоразмерных знаковых int.
            if constexpr(sizeof(T) < sizeof(int)) {
                // Значение должно укладываться в допустимый диапазон типа.
                if(parsed_int < static_cast<long long>(std::numeric_limits<T>::min()) ||
                   parsed_int > static_cast<long long>(std::numeric_limits<T>::max())) {
                    return std::unexpected(scan_error("Unexpected result. Integer out of range for target type {}."s));
                }
            }
        }
        else {  // Проверка беззнакового int.
            // Беззнаковый тип не может быть отрицатлеьным.
            if(parsed_int < 0) {
                return std::unexpected(scan_error("Unexpected result. Negative value parsed for unsigned type {}."s));
            }
            if constexpr(sizeof(T) < sizeof(int)) {  // Для uint8_t, uint16_t, uint32_t ...
                // Значение должно укладываться в допустимый диапазон типа.
                if(static_cast<unsigned long long>(parsed_int) >
                   static_cast<unsigned long long>(std::numeric_limits<T>::max())) {
                    return std::unexpected(
                        scan_error("Unexpected result. Unsigned integer out of range for target type {}."s));
                }
            }
        }
        // В этой точке тип T не будет "обрезан".
        return static_cast<T>(parsed_int);
    }
    else if constexpr(isFloating<T>) {
        auto float_res = parse_floating(input);
        if(!float_res) {
            return std::unexpected(
                scan_error("Unexpected result. Failed to parse float for {}: "s + float_res.error().what()));
        }
        double parsed_double = float_res.value();

        // Проверка на "обрезку", если double кастуется к float (T = float, value at {%f} = double).
        if constexpr(std::is_same_v<std::remove_cv_t<T>, float>) {
            constexpr auto float_max = static_cast<double>(std::numeric_limits<T>::max());
            constexpr auto float_min = static_cast<double>(std::numeric_limits<T>::lowest());
            if(parsed_double < float_min || parsed_double > float_max) {
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
                if constexpr(isCString<T>) {
                    return std::string {input};
                }
                if constexpr(isString<T> || isStringView<T>) {
                    return parse_string(input);
                }
                else {
                    return std::unexpected(
                        scan_error("Unexpected result. Type mismatch: 's' specifier requires a string-line type."s));
                }
            }
            case 'd': {
                if constexpr(isIntegral<T>) {
                    auto res = parse_integral(input);
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
                if constexpr(isNatural<T>) {
                    auto res = parse_natural(input);
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
                if constexpr(isFloating<T>) {
                    auto res = parse_floating(input);
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
