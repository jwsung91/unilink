/*
 * Copyright 2025 Jinwoo Sung
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <cstdint>
#include <stdexcept>
#include <type_traits>

namespace unilink {
namespace common {

/**
 * @brief A C++17 compatible span-like class for safe array access
 *
 * This provides a lightweight, non-owning view over a contiguous sequence of objects.
 * Similar to std::span but compatible with C++17.
 */
template <typename T>
class SafeSpan {
 public:
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using iterator = T*;
  using const_iterator = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  // Constructors
  constexpr SafeSpan() noexcept : data_(nullptr), size_(0) {}

  constexpr SafeSpan(pointer data, size_type size) noexcept : data_(data), size_(size) {}

  template <typename Container>
  constexpr SafeSpan(Container& container) noexcept : data_(container.data()), size_(container.size()) {}

  template <typename Container>
  constexpr SafeSpan(const Container& container) noexcept
      : data_(const_cast<pointer>(container.data())), size_(container.size()) {}

  // Access methods
  constexpr reference operator[](size_type index) const { return data_[index]; }

  constexpr reference at(size_type index) const {
    if (index >= size_) {
      throw std::out_of_range("SafeSpan index out of range");
    }
    return data_[index];
  }

  constexpr reference front() const { return data_[0]; }

  constexpr reference back() const { return data_[size_ - 1]; }

  constexpr pointer data() const noexcept { return data_; }

  constexpr size_type size() const noexcept { return size_; }

  constexpr size_type size_bytes() const noexcept { return size_ * sizeof(T); }

  constexpr bool empty() const noexcept { return size_ == 0; }

  // Iterator support
  constexpr iterator begin() const noexcept { return data_; }

  constexpr iterator end() const noexcept { return data_ + size_; }

  constexpr const_iterator cbegin() const noexcept { return data_; }

  constexpr const_iterator cend() const noexcept { return data_ + size_; }

  constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator(end()); }

  constexpr reverse_iterator rend() const noexcept { return reverse_iterator(begin()); }

  constexpr const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(cend()); }

  constexpr const_reverse_iterator crend() const noexcept { return const_reverse_iterator(cbegin()); }

  // Subspan operations
  constexpr SafeSpan<T> subspan(size_type offset, size_type count = SIZE_MAX) const {
    if (offset > size_) {
      throw std::out_of_range("SafeSpan subspan offset out of range");
    }
    size_type actual_count = (count == SIZE_MAX) ? (size_ - offset) : count;
    if (offset + actual_count > size_) {
      throw std::out_of_range("SafeSpan subspan count out of range");
    }
    return SafeSpan<T>(data_ + offset, actual_count);
  }

  constexpr SafeSpan<T> first(size_type count) const { return subspan(0, count); }

  constexpr SafeSpan<T> last(size_type count) const { return subspan(size_ - count, count); }

 private:
  pointer data_;
  size_type size_;
};

// Type aliases for common types
using ByteSpan = SafeSpan<uint8_t>;
using ConstByteSpan = SafeSpan<const uint8_t>;
using CharSpan = SafeSpan<char>;
using ConstCharSpan = SafeSpan<const char>;

}  // namespace common
}  // namespace unilink
