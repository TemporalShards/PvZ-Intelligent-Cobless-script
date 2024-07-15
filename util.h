#pragma once
#ifndef __UTIL_H__
#define __UTIL_H__

#include <algorithm>
#include <iterator>
#include <unordered_set>
#include <vector>

// 判断两个区间是否有交集
inline bool HasIntersection(std::pair<float, float> interval1, std::pair<float, float> interval2)
{
    // 确保区间是有序的
    if (interval1.first > interval1.second) {
        std::swap(interval1.first, interval1.second);
    }
    if (interval2.first > interval2.second) {
        std::swap(interval2.first, interval2.second);
    }
    return std::max(interval1.first, interval2.first) <= std::min(interval1.second, interval2.second);
}

// 合并多个vector并返回一个新的vector
template <typename T, typename... Args>
std::vector<T> MergeVectors(Args&&... vectors)
{
    std::vector<T> result;
    size_t totalSize = 0;

    ((totalSize += vectors.size()), ...);

    result.reserve(totalSize);

    (..., std::copy(vectors.begin(), vectors.end(), std::back_inserter(result)));

    return result;
}

// 去除向量中的重复元素
template <typename T>
void RemoveDuplicates(std::vector<T> vector)
{
    std::unordered_set<T> seenElements;
    std::vector<T> result;

    for (const T& elem : vector) {
        if (seenElements.find(elem) == seenElements.end()) {
            result.push_back(elem);
            seenElements.insert(elem);
        }
    }

    vector.swap(result);
}

#endif //!__UTIL_H__