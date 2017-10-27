#pragma once

#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <type_traits>

#include "Memory/Memory.h"

/* we don't use this, leave as a reference
#define REPODArray std::vector

// base template, should not be used
template<typename T, int Alignment, bool POD = std::is_pod<T>::value>
struct REArrayType;

// POD
template<typename T, int Alignment>
struct REArrayType<T, Alignment, true> : 
	std::_Identity<REPODArray<T, REAllocator<T, Alignment>>> {};

// non-POD
template<typename T, int Alignment>
struct REArrayType<T, Alignment, false> : 
	std::_Identity<std::vector<T, REAllocator<T, Alignment>>> {};

// template alias
template<class T, int Alignment = 0>
using REArray = typename REArrayType<T, Alignment>::type;
*/

// template alias
template<class T, int Alignment = 0>
using REArray = std::vector<T, REAllocator<T, Alignment>>;

template<class TKey, class TValue, int Alignment = 0>
using REMap = std::unordered_map<TKey, TValue, std::hash<TKey>, std::equal_to<TKey>, REAllocator<std::pair<const TKey, TValue>, Alignment>>;

template<class TKey, class TValue, int Alignment = 0>
using RESortedMap = std::map<TKey, TValue, std::less<TKey>, REAllocator<std::pair<const TKey, TValue>, Alignment>>;

template<class T, int Alignment = 0>
using RESet = std::unordered_set<T, std::hash<T>, std::equal_to<T>, REAllocator<T, Alignment>>;

template<class T, int Alignment = 0>
using RESortedSet = std::set<T, std::less<T>, REAllocator<T, Alignment>>;