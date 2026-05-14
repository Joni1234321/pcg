#pragma once
#include "u_collections.hpp"

namespace pce {
template <typename Iterator, typename OutputIterator, typename UnaryOperation> constexpr OutputIterator TransformLocal(Iterator first, Iterator last, OutputIterator result, UnaryOperation unary_operation) {
    while (first != last) {
        *result = unary_operation(*first);
        ++first;
        ++result;
    }
    return result;
}

template <typename Iterator1, typename Iterator2, typename OutputIterator, typename BinaryOperation>
constexpr OutputIterator TransformLocal(Iterator1 first1, Iterator1 last1, Iterator2 first2, OutputIterator result, BinaryOperation binary_operation) {
    while (first1 != last1) {
        *result = binary_operation(*first1, *first2);
        ++first1;
        ++first2;
        ++result;
    }
    return result;
}

template <typename Container1, typename Container2, typename OutputContainer, typename BinaryOperation>
constexpr typename OutputContainer::iterator Transform(const Container1& container1, const Container2& container2, OutputContainer& out, BinaryOperation binary_operation) {
    return TransformLocal(std::begin(container1), std::end(container1), std::begin(container2), std::begin(out), binary_operation);
}

template <typename Container, typename OutputContainer, typename UnaryOperation> constexpr OutputContainer Transform(Container iterator, OutputContainer out, UnaryOperation binary_operation) {
    return TransformLocal(std::begin(iterator), std::end(iterator), std::begin(out), binary_operation);
}

template <typename Container, typename BinaryOperation> constexpr auto Select(const Container& container1, const Container& container2, BinaryOperation binary_operation) {
    using ResultType = decltype(binary_operation(*container1.begin(), *container2.begin()));
    List<ResultType> out { };
    TransformLocal(std::begin(container1), std::end(container1), std::begin(container2), std::back_inserter(out), binary_operation);
    return out;
}

template <typename Container, typename UnaryOperation> constexpr auto Select(const Container& container, UnaryOperation unary_operation) {
    using ResultType = decltype(unary_operation(*std::begin(container)));
    List<ResultType> out { };
    TransformLocal(container.begin(), container.end(), std::back_inserter(out), unary_operation);
    return out;
}

class _Find_Index_Of_fn {
public:
    template <std::ranges::input_range R, class T, class P> [[nodiscard]] constexpr std::optional<u32> operator()(R&& range, const T& value, P projection) const {
        const auto& iterator = std::ranges::find(std::forward<R>(range), value, projection);
        if (iterator == range.end()) { return std::nullopt; }
        return static_cast<u32>(std::distance(range.begin(), iterator));
    }
};

inline constexpr _Find_Index_Of_fn find_index_of;
} // namespace pce
