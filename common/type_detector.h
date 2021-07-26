#ifndef SFINAE_TYPE_DETECTOR_H__
#define SFINAE_TYPE_DETECTOR_H__

#include <type_traits>

#define MEMBER_FUNCTION_DETECTOR(DetectorName, FunctionName, ReturnType, ...)                          \
    template <typename T>                                                                              \
    struct DetectorName                                                                                \
    {                                                                                                  \
        template <typename U, U u>                                                                     \
        class Checker;                                                                                 \
                                                                                                       \
        template <typename V>                                                                          \
        static std::true_type test(Checker<ReturnType (V::*)(__VA_ARGS__), &V::FunctionName> *);       \
                                                                                                       \
        template <typename V>                                                                          \
        static std::true_type test(Checker<ReturnType (V::*)(__VA_ARGS__) const, &V::FunctionName> *); \
                                                                                                       \
        template <typename V>                                                                          \
        static std::false_type test(...);                                                              \
                                                                                                       \
        typedef decltype(test<T>(nullptr)) type;                                                       \
        static const bool value = std::is_same<std::true_type, type>::value;                           \
    };

#endif