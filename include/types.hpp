#pragma once

namespace stdx::details {

// Концепты для проверок соответствия типов. В том числе поддержка cv-квалификаторов типов.
template<typename... T>
concept isIntegral = (std::is_integral_v<std::remove_cv_t<T>> && ...);

template<typename... T>
concept isNatural = ((std::is_integral_v<std::remove_cv_t<T>> && std::is_unsigned_v<std::remove_cv_t<T>>) && ...);

template<typename... T>
concept isFloating = (std::is_floating_point_v<std::remove_cv_t<T>> && ...);

template<typename... T>
concept isCString = (std::same_as<const char*, T> && ...);

template<typename... T>
concept isString = (std::is_base_of_v<std::basic_string<typename T::value_type>, T> && ...);

template<typename... T>
concept isStringView = (std::is_base_of_v<std::basic_string_view<char>, T> && ...);

// Класс для хранения ошибки неуспешного сканирования.
struct scan_error {
    std::string message {};
};

// Шаблонный класс для хранения результатов успешного сканирования.
template<typename... Ts> struct scan_result {
    std::tuple<Ts...> result {};
    std::tuple<Ts...> values() const noexcept {
        return result;
    }
};

}  // namespace stdx::details
