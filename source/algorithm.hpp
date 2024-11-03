#pragma once
#include "collections.hpp"

namespace pce {
template <typename Iterator, typename OutputIterator, typename UnaryOperation> constexpr OutputIterator TransformLocal(Iterator first, Iterator last, OutputIterator result, UnaryOperation unary_operation) {
    while (first != last) {
        *result = unary_operation(*first);
        ++first;
        ++result;
    }
    return result;
}

template <typename Iterator1, typename Iterator2, typename OutputIterator, typename BinaryOperation> constexpr OutputIterator TransformLocal(
Iterator1 first1, Iterator1 last1, Iterator2 first2, OutputIterator result, BinaryOperation binary_operation) {
    while (first1 != last1) {
        *result = binary_operation(*first1, *first2);
        ++first1;
        ++first2;
        ++result;
    }
    return result;
}

template <typename Container1, typename Container2, typename OutputContainer, typename BinaryOperation> constexpr typename OutputContainer::iterator Transform(
    const Container1& container1, const Container2& container2, OutputContainer& out, BinaryOperation binary_operation) {
    return TransformLocal(std::begin(container1), std::end(container1), std::begin(container2), std::begin(out), binary_operation);
}

template <typename Container, typename OutputContainer, typename UnaryOperation> constexpr OutputContainer Transform(Container iterator, OutputContainer out, UnaryOperation binary_operation) {
    return TransformLocal(std::begin(iterator), std::end(iterator), std::begin(out), binary_operation);
}

template <typename Container, typename BinaryOperation> constexpr auto Select(const Container& container1, const Container& container2, BinaryOperation binary_operation) {
    Container out { };
    out.Resize(container1.size());
    TransformLocal(std::begin(container1), std::end(container1), std::begin(container2), std::begin(out), binary_operation);
    return out;
}

template <typename Container, typename UnaryOperation> constexpr auto Select(const Container& container, UnaryOperation unary_operation) {
    using ResultType = decltype(unary_operation(*std::begin(container)));
    List<ResultType> out { };
    out.Resize(container.size());
    TransformLocal(container.begin(), container.end(), out.begin(), unary_operation);
    return out;
}
} // namespace pce
