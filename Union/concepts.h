#pragma once
#include "framework.h"
#include "pch.h"

namespace HYDRA15::Union::framework
{
    // 信息输出相关概念
    template<typename T>
    concept can_be_transfer_to_string = requires(std::remove_cvref_t<T> a) {
        { std::to_string(a) } -> std::convertible_to<std::string>;
    } || requires(std::remove_cvref_t<T> a) {
        { to_string(a) } -> std::convertible_to<std::string>;
    };

    template<class T>
    concept has_info_interface = requires(std::remove_cvref_t<T> a) {
        { a.info() } -> std::convertible_to<std::string>;
    };

    // 键约束
    template<typename K>
    concept compare_key = requires(K k1, K k2) {
        { k1 < k2 }->std::same_as<bool>;
    };

    template<typename K>
    concept hash_key = requires(K k) {
        { std::hash<K>()(k) } -> std::same_as<size_t>;
    }&& requires(K a, K b) {
        { a == b } -> std::same_as<bool>;
    };

    template<typename K>
    concept index_key = compare_key<K> && hash_key<K>;

    // 锁约束
    template<typename T>
    concept lockable = requires(T t) {
        { t.lock() } -> std::same_as<void>;
        { t.unlock() } -> std::same_as<void>;
    };

    template<typename T>
    concept shared_lockable = lockable<T> && requires(T t) {
        { t.lock_shared() } -> std::same_as<void>;
        { t.unlock_shared() } -> std::same_as<void>;
    };

    // 去除修饰符后真实类型
    template<typename T, typename RT>
    concept is_really_same_v = std::is_same_v<std::remove_cvref_t<T>, std::remove_cvref_t<RT>>;

    template<typename T, typename RT>
    concept is_really_derived_from = std::derived_from<std::remove_cvref_t<T>, std::remove_cvref_t<RT>>;
}