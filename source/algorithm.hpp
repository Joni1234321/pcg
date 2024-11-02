#pragma once
#include "collections.hpp"

#include <span>
namespace pce {
template <typename InputIterator, typename OutputIterator, typename UnaryOperation>
constexpr OutputIterator transform_local(InputIterator first, InputIterator last, OutputIterator result, UnaryOperation unaryOperation) {
	for (; first != last; ++first, ++result)
		*result = unaryOperation(*first);
	return result;
}


template <typename InputIterator1, typename InputIterator2, typename OutputIterator, typename BinaryOperation>
constexpr OutputIterator transform_local(InputIterator1 first1, InputIterator1 last1, InputIterator2 first2, OutputIterator result, BinaryOperation binaryOperation) {
	for (; first1 != last1; ++first1, ++first2, ++result)
		*result = binaryOperation(*first1, *first2);
	return result;
}

template <typename Container1, typename Container2, typename OutputContainer, typename BinaryOperation>
constexpr OutputContainer::iterator transform(const Container1 &c1, const Container2 &c2, OutputContainer &out, BinaryOperation binaryOperation) {
	return transform_local(std::begin(c1), std::end(c1), std::begin(c2), std::begin(out), binaryOperation);
}

template <typename InputIterator, typename OutputIterator, typename UnaryOperation>
constexpr OutputIterator transform(InputIterator i1, OutputIterator out, UnaryOperation binaryOperation) {
	return transform_local(std::begin(i1), std::end(i1), std::begin(out), binaryOperation);
}

template <typename Container, typename BinaryOperation>
constexpr auto select(const Container &c1, const Container &c2, BinaryOperation binaryOperation) {
	Container out{};
	out.resize(c1.size());
	transform_local(std::begin(c1), std::end(c1), std::begin(c2), std::begin(out), binaryOperation);
	return out;
}

template <typename Container, typename UnaryOperation>
constexpr auto select(const Container &c, UnaryOperation unaryOperation) {
	//using ResultType = std::invoke_result_t<UnaryOperation, typename Container::value_type>;
	using ResultType = decltype(unaryOperation(*std::begin(c)));
	List<ResultType> out{};
	out.resize(c.size());
	transform_local(c.begin(), c.end(), out.begin(), unaryOperation);
	return out;
}


}