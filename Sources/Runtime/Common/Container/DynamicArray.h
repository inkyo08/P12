#pragma once

#include <compare>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <cstring>

#include <mimalloc.h>

namespace Common::Container {
  template <typename T>
  class DynamicArray {
    T *head;
    T *tail;
    T *capacity;

    size_t recommend(size_t newSize) {
      const size_t ms = SIZE_MAX / sizeof(T);
      if (newSize > ms) abort();
      const size_t cur = capacity - head;
      if (cur >= ms / 2) return ms;
      return cur * 2 > newSize ? cur * 2 : newSize;
    }

    void grow(size_t newCap) {
      size_t len = tail - head;
      if constexpr (__is_trivially_copyable(T)) {
        head = static_cast<T *>(mi_realloc(head, newCap * sizeof(T)));
        if (head == nullptr) abort();
      } else {
        T *newHead = static_cast<T *>(mi_malloc(newCap * sizeof(T)));
        if (newHead == nullptr) abort();
        for (size_t i = 0; i < len; ++i) {
          new (newHead + i) T(static_cast<T &&>(head[i]));
          head[i].~T();
        }
        mi_free(head);
        head = newHead;
      }
      tail = head + len;
      capacity = head + newCap;
    }

  public:
    using Iterator = T *;
    using ConstIterator = const T *;

    struct ReverseIterator {
      T *ptr;
      explicit ReverseIterator(T *p) : ptr(p) {}
      T &operator*() const { return *(ptr - 1); }
      T *operator->() const { return ptr - 1; }
      ReverseIterator &operator++() { --ptr; return *this; }
      ReverseIterator &operator--() { ++ptr; return *this; }
      bool operator==(const ReverseIterator &o) const { return ptr == o.ptr; }
    };

    struct ConstReverseIterator {
      const T *ptr;
      explicit ConstReverseIterator(const T *p) : ptr(p) {}
      const T &operator*() const { return *(ptr - 1); }
      const T *operator->() const { return ptr - 1; }
      ConstReverseIterator &operator++() { --ptr; return *this; }
      ConstReverseIterator &operator--() { ++ptr; return *this; }
      bool operator==(const ConstReverseIterator &o) const { return ptr == o.ptr; }
    };

    DynamicArray() : head(nullptr), tail(nullptr), capacity(nullptr) {}

    DynamicArray(const DynamicArray &other) : head(nullptr), tail(nullptr), capacity(nullptr) {
      size_t len = other.tail - other.head;
      if (len == 0) return;
      head = static_cast<T *>(mi_malloc(len * sizeof(T)));
      if (head == nullptr) abort();
      for (size_t i = 0; i < len; ++i)
        new (head + i) T(other.head[i]);
      tail = head + len;
      capacity = head + len;
    }

    DynamicArray(DynamicArray &&other) noexcept : head(other.head), tail(other.tail), capacity(other.capacity) {
      other.head = other.tail = other.capacity = nullptr;
    }

    DynamicArray &operator=(const DynamicArray &other) {
      if (this == &other) return *this;
      clear();
      mi_free(head);
      head = tail = capacity = nullptr;
      size_t len = other.tail - other.head;
      if (len == 0) return *this;
      head = static_cast<T *>(mi_malloc(len * sizeof(T)));
      if (head == nullptr) abort();
      for (size_t i = 0; i < len; ++i)
        new (head + i) T(other.head[i]);
      tail = head + len;
      capacity = head + len;
      return *this;
    }

    DynamicArray &operator=(DynamicArray &&other) noexcept {
      if (this == &other) return *this;
      clear();
      mi_free(head);
      head = other.head;
      tail = other.tail;
      capacity = other.capacity;
      other.head = other.tail = other.capacity = nullptr;
      return *this;
    }

    ~DynamicArray() noexcept {
      if (head != nullptr) {
        clear();
        mi_free(head);
      }
    }

    [[nodiscard]] size_t size() const { return static_cast<size_t>(tail - head); }
    [[nodiscard]] size_t cap() const { return static_cast<size_t>(capacity - head); }
    [[nodiscard]] bool empty() const { return head == tail; }

    void resize(size_t newSize) {
      size_t cur = tail - head;
      if (newSize > cur) {
        if (newSize > static_cast<size_t>(capacity - head))
          grow(recommend(newSize));
        for (T *p = tail; p != head + newSize; ++p)
          new (p) T();
      } else {
        for (T *p = head + newSize; p != tail; ++p)
          p->~T();
      }
      tail = head + newSize;
    }

    void resize(size_t newSize, const T &value) {
      size_t cur = tail - head;
      if (newSize > cur) {
        if (newSize > static_cast<size_t>(capacity - head))
          grow(recommend(newSize));
        for (T *p = tail; p != head + newSize; ++p)
          new (p) T(value);
      } else {
        for (T *p = head + newSize; p != tail; ++p)
          p->~T();
      }
      tail = head + newSize;
    }

    void shrinkToFit() {
      size_t len = tail - head;
      if (len == static_cast<size_t>(capacity - head)) return;
      if (len == 0) {
        mi_free(head);
        head = tail = capacity = nullptr;
        return;
      }
      if constexpr (__is_trivially_copyable(T)) {
        head = static_cast<T *>(mi_realloc(head, len * sizeof(T)));
        if (head == nullptr) abort();
      } else {
        T *newHead = static_cast<T *>(mi_malloc(len * sizeof(T)));
        if (newHead == nullptr) abort();
        for (size_t i = 0; i < len; ++i) {
          new (newHead + i) T(static_cast<T &&>(head[i]));
          head[i].~T();
        }
        mi_free(head);
        head = newHead;
      }
      tail = head + len;
      capacity = head + len;
    }

    void reserve(size_t minCap) {
      if (minCap <= static_cast<size_t>(capacity - head)) return;
      grow(minCap);
    }

    T &operator[](size_t index) { return head[index]; }
    const T &operator[](size_t index) const { return head[index]; }

    T &at(size_t index) {
      if (index >= static_cast<size_t>(tail - head)) abort();
      return head[index];
    }
    const T &at(size_t index) const {
      if (index >= static_cast<size_t>(tail - head)) abort();
      return head[index];
    }

    T *data() { return head; }
    const T *data() const { return head; }

    T &front() { return *head; }
    const T &front() const { return *head; }

    T &back() { return *(tail - 1); }
    const T &back() const { return *(tail - 1); }

    template <class... Args>
    void emplaceBack(Args &&...args) {
      if (tail == capacity)
        grow(recommend(tail - head + 1));
      new (tail) T(static_cast<Args &&>(args)...);
      ++tail;
    }

    void append(T item) { emplaceBack(static_cast<T &&>(item)); }

    void insert(size_t index, T item) {
      if (tail == capacity)
        grow(recommend(tail - head + 1));
      if constexpr (__is_trivially_copyable(T)) {
        memmove(head + index + 1, head + index, (tail - head - index) * sizeof(T));
        new (head + index) T(static_cast<T &&>(item));
      } else {
        new (tail) T(static_cast<T &&>(tail[-1]));
        for (T *p = tail - 1; p > head + index; --p)
          *p = static_cast<T &&>(*(p - 1));
        head[index] = static_cast<T &&>(item);
      }
      ++tail;
    }

    void popBack() {
      --tail;
      tail->~T();
    }

    void orderedRemove(Iterator first, Iterator last) {
      auto count = static_cast<size_t>(last - first);
      if constexpr (__is_trivially_copyable(T)) {
        memmove(first, last, (tail - last) * sizeof(T));
      } else {
        for (T *dst = first, *src = last; src != tail; ++dst, ++src)
          *dst = static_cast<T &&>(*src);
        for (T *p = tail - count; p != tail; ++p)
          p->~T();
      }
      tail -= count;
    }

    T orderedRemove(size_t index) {
      T val = static_cast<T &&>(head[index]);
      if constexpr (__is_trivially_copyable(T)) {
        memmove(head + index, head + index + 1, (tail - head - index - 1) * sizeof(T));
      } else {
        for (T *p = head + index; p != tail - 1; ++p)
          *p = static_cast<T &&>(*(p + 1));
      }
      --tail;
      tail->~T();
      return val;
    }

    T swapRemove(size_t index) {
      T val = static_cast<T &&>(head[index]);
      --tail;
      if constexpr (__is_trivially_copyable(T)) {
        if (head + index != tail)
          memcpy(head + index, tail, sizeof(T));
      } else {
        if (head + index != tail)
          head[index] = static_cast<T &&>(*tail);
        tail->~T();
      }
      return val;
    }

    bool operator==(const DynamicArray &other) const {
      if (tail - head != other.tail - other.head) return false;
      for (size_t i = 0; i < static_cast<size_t>(tail - head); ++i)
        if (head[i] != other.head[i]) return false;
      return true;
    }

    auto operator<=>(const DynamicArray &other) const {
      size_t len1 = tail - head;
      size_t len2 = other.tail - other.head;
      size_t minLen = len1 < len2 ? len1 : len2;
      for (size_t i = 0; i < minLen; ++i)
        if (auto cmp = head[i] <=> other.head[i]; cmp != 0)
          return cmp;
      return len1 <=> len2;
    }

    DynamicArray &operator+=(const DynamicArray &other) {
      reserve(static_cast<size_t>(tail - head) + static_cast<size_t>(other.tail - other.head));
      for (const T *p = other.head; p != other.tail; ++p)
        append(*p);
      return *this;
    }

    DynamicArray operator+(const DynamicArray &other) const {
      DynamicArray ret(*this);
      ret += other;
      return ret;
    }

    void swap(DynamicArray &other) noexcept {
      T *tmpHead     = head;
      T *tmpTail     = tail;
      T *tmpCapacity = capacity;
      head     = other.head;
      tail     = other.tail;
      capacity = other.capacity;
      other.head     = tmpHead;
      other.tail     = tmpTail;
      other.capacity = tmpCapacity;
    }

    void clear() {
      for (T *p = head; p != tail; ++p)
        p->~T();
      tail = head;
    }

    Iterator begin() { return head; }
    Iterator end() { return tail; }
    ConstIterator begin() const { return head; }
    ConstIterator end() const { return tail; }
    ConstIterator cbegin() const { return head; }
    ConstIterator cend() const { return tail; }

    ReverseIterator rbegin() { return ReverseIterator(tail); }
    ReverseIterator rend() { return ReverseIterator(head); }
    ConstReverseIterator rbegin() const { return ConstReverseIterator(tail); }
    ConstReverseIterator rend() const { return ConstReverseIterator(head); }
    ConstReverseIterator crbegin() const { return ConstReverseIterator(tail); }
    ConstReverseIterator crend() const { return ConstReverseIterator(head); }
  };
}
