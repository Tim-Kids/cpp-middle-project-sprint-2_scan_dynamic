#pragma once

namespace stdx::details {

// Концепты для проверок соответствия типов. В том числе поддержка cv-квалификаторов типов.
template<typename... T>
concept is_integral = ((std::integral<std::remove_cv_t<T>>) && ...);

template<typename... T>
concept is_natural = ((std::integral<std::remove_cv_t<T>> && std::unsigned_integral<std::remove_cv_t<T>>) && ...);

template<typename... T>
concept is_floating = ((std::floating_point<std::remove_cv_t<T>>) && ...);

template<typename... T>
concept is_c_string = (std::same_as<const char*, T> && ...);

template<typename... T>
concept is_string = (std::same_as<std::string, T> && ...);

template<typename... T>
concept is_string_view = (std::same_as<std::string_view, T> && ...);

// Класс для хранения ошибки неуспешного сканирования.
struct scan_error {
    std::string message {};
};

// Шаблонный класс для хранения результатов успешного сканирования.
template<typename... Ts> struct scan_result {
    std::tuple<Ts...> result {};
};

}  // namespace stdx::details
