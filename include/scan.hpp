#pragma once

#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "parse.hpp"
#include "types.hpp"

namespace stdx {

using namespace std::literals;

template<typename... Ts, std::size_t... Ids>
constexpr std::expected<std::tuple<Ts...>, details::scan_error>
populate_tuple_impl(const std::vector<std::string_view>& input_parts, const std::vector<std::string_view>& fmt_parts,
                    std::index_sequence<Ids...>) {
    std::tuple<Ts...> result;
    bool success = true;
    details::scan_error error;

    // Реализация лямбды, которая в compile-time пишет данные в кортеж result согласно индексу типа записываемых данных.
    auto process_element = [&](auto Id) {
        if(!success) {
            return;  // Досрочный выход из распаковки.
        }

        constexpr std::size_t i = Id();  // operator() позволяет получить значение индекса типа в compile-time, т.к. оно
                                         // нужно для вызова std::tuple_element_t и std::get в compile-time.

        if(i >= input_parts.size() || i >= fmt_parts.size()) {
            success = false;
            error   = details::scan_error("Unexpected result. Index out of bounds during tuple population."s);
            return;
        }

        using TypeAtIndex = std::tuple_element_t<i, std::tuple<Ts...>>;
        auto parse_result = details::parse_value_with_format<TypeAtIndex>(input_parts[i], fmt_parts[i]);

        if(parse_result) {
            std::get<i>(result) = std::move(parse_result.value());
        }
        else {
            success = false;
            error   = parse_result.error();
        }
    };

    // Вызов основной вариабельной шаблонной лямбды, которая упаковывает распарсенные данные пакета типов в кортеж.
    // Используем std::integral_constant, который позволяет распаковать индексы типов и корректно передать их в лямбду.
    (process_element(std::integral_constant<std::size_t, Ids> {}), ...);

    if(success) {
        return result;
    }
    else {
        return std::unexpected(std::move(error));
    }
}

template<typename... Ts>
constexpr std::expected<std::tuple<Ts...>, details::scan_error>
populate_tuple(const std::vector<std::string_view>& input, const std::vector<std::string_view>& format) {
    return populate_tuple_impl<Ts...>(input, format, std::index_sequence_for<Ts...> {});
}

// замените болванку функции scan на рабочую версию
template<typename... Ts>
constexpr std::expected<details::scan_result<Ts...>, details::scan_error> scan(std::string_view input,
                                                                               std::string_view format) {
    // Получаем результат разбиения строк форматов и исходных данных.
    auto parsed = details::parse_sources<Ts...>(input, format);
    if(!parsed) {
        return std::unexpected(details::scan_error(parsed.error().message));
    }

    auto [fmt, data] = std::move(parsed.value());

    // Число спецификаторов формата должно совпадать с числом шаблонных параметров ... Ts.
    if(fmt.size() != sizeof...(Ts)) {
        return std::unexpected(
            details::scan_error {"Unexpected result. Mismatched number of format specifiers and target types"s});
    }

    // Агрегируем результаты работы parse_value_with_format в объект типа scan_result.
    auto populateResult = populate_tuple<Ts...>(data, fmt);
    if(!populateResult) {
        return std::unexpected(populateResult.error());
    }
    details::scan_result<Ts...> scanResult;
    scanResult.result = std::move(populateResult.value());

    return std::move(scanResult);
}
}  // namespace stdx
