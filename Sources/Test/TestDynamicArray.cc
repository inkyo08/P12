#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_NO_EXCEPTIONS_BUT_WITH_ALL_ASSERTS
#include "Doctest.h"

#include <Runtime/Common/Container/DynamicArray.h>

#include <memory>
#include <string>

using Common::Container::DynamicArray;

// ─────────────────────────────────────────────────────────────────────────────
// 생성자 / 소멸자
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - 기본 생성자") {
  DynamicArray<int> a;
  CHECK(a.size() == 0);
  CHECK(a.cap() == 0);
  CHECK(a.empty());
  CHECK(a.data() == nullptr);
}

TEST_CASE("DynamicArray - 복사 생성자") {
  SUBCASE("내용이 동일하게 복사된다") {
    DynamicArray<int> src;
    src.append(1);
    src.append(2);
    src.append(3);

    DynamicArray<int> dst(src);

    CHECK(dst.size() == 3);
    CHECK(dst[0] == 1);
    CHECK(dst[1] == 2);
    CHECK(dst[2] == 3);
  }

  SUBCASE("복사본과 원본은 메모리가 독립적이다") {
    DynamicArray<int> src;
    src.append(1);
    src.append(2);

    DynamicArray<int> dst(src);
    dst[0] = 99;

    CHECK(src[0] == 1); // 원본 불변
  }

  SUBCASE("non-trivially copyable 타입도 올바르게 복사된다") {
    DynamicArray<std::string> src;
    src.append("hello");
    src.append("world");

    DynamicArray<std::string> dst(src);

    CHECK(dst.size() == 2);
    CHECK(dst[0] == "hello");
    CHECK(dst[1] == "world");

    dst[0] = "bye";
    CHECK(src[0] == "hello"); // 원본 불변
  }

  SUBCASE("빈 배열을 복사하면 빈 배열이 된다") {
    DynamicArray<int> src;
    DynamicArray<int> dst(src);

    CHECK(dst.empty());
    CHECK(dst.data() == nullptr);
  }
}

TEST_CASE("DynamicArray - 이동 생성자") {
  SUBCASE("데이터가 이전되고 원본은 비어 있다") {
    DynamicArray<int> src;
    src.append(10);
    src.append(20);
    const int* originalData = src.data();

    DynamicArray<int> dst(std::move(src));

    CHECK(dst.size() == 2);
    CHECK(dst[0] == 10);
    CHECK(dst[1] == 20);
    CHECK(dst.data() == originalData);
    CHECK(src.empty());
    CHECK(src.data() == nullptr);
  }

  SUBCASE("빈 배열 이동") {
    DynamicArray<int> src;
    DynamicArray<int> dst(std::move(src));

    CHECK(dst.empty());
    CHECK(src.empty());
  }
}

TEST_CASE("DynamicArray - 복사 대입 연산자") {
  SUBCASE("기존 데이터를 버리고 올바르게 복사된다") {
    DynamicArray<int> src;
    src.append(1);
    src.append(2);

    DynamicArray<int> dst;
    dst.append(99);
    dst = src;

    CHECK(dst.size() == 2);
    CHECK(dst[0] == 1);
    CHECK(dst[1] == 2);
  }

  SUBCASE("복사본과 원본은 메모리가 독립적이다") {
    DynamicArray<int> src;
    src.append(1);

    DynamicArray<int> dst;
    dst = src;
    dst[0] = 42;

    CHECK(src[0] == 1);
  }

  SUBCASE("자기 자신에 대입해도 안전하다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);

    DynamicArray<int>& ref = a;
    ref = a;

    CHECK(a.size() == 2);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
  }
}

TEST_CASE("DynamicArray - 이동 대입 연산자") {
  SUBCASE("데이터가 이전되고 원본은 비어 있다") {
    DynamicArray<int> src;
    src.append(10);
    src.append(20);
    const int* originalData = src.data();

    DynamicArray<int> dst;
    dst.append(99);
    dst = std::move(src);

    CHECK(dst.size() == 2);
    CHECK(dst[0] == 10);
    CHECK(dst[1] == 20);
    CHECK(dst.data() == originalData);
    CHECK(src.empty());
    CHECK(src.data() == nullptr);
  }

  SUBCASE("자기 자신에 이동 대입해도 안전하다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);

    DynamicArray<int>& ref = a;
    ref = std::move(a);

    // self-assign guard로 데이터가 유지된다
    CHECK(a.size() == 2);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// 용량/크기 쿼리 (size, cap, empty)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - empty") {
  SUBCASE("기본 생성 직후에는 비어 있다") {
    DynamicArray<int> a;
    CHECK(a.empty());
  }

  SUBCASE("원소 추가 후에는 비어 있지 않다") {
    DynamicArray<int> a;
    a.append(1);
    CHECK_FALSE(a.empty());
  }
}

TEST_CASE("DynamicArray - size") {
  SUBCASE("append할 때마다 1씩 증가한다") {
    DynamicArray<int> a;
    CHECK(a.size() == 0);
    a.append(1);
    CHECK(a.size() == 1);
    a.append(2);
    CHECK(a.size() == 2);
    a.append(3);
    CHECK(a.size() == 3);
  }
}

TEST_CASE("DynamicArray - cap") {
  // recommend()의 grow 규칙:
  //   cur == 0 → newCap = 1
  //   cur == 1 → newCap = 2
  //   cur == 2 → newCap = 4  (cur*2=4 > newSize=3)
  //   cur == 4 → newCap = 8  (cur*2=8 > newSize=5)
  SUBCASE("grow 시 cap이 두 배로 늘어난다") {
    DynamicArray<int> a;
    CHECK(a.cap() == 0);

    a.append(0);
    CHECK(a.cap() == 1);

    a.append(0);
    CHECK(a.cap() == 2);

    a.append(0); // size=3, 초과 → newCap = 4
    CHECK(a.cap() == 4);

    a.append(0); // size=4, cap 여유 있음
    CHECK(a.cap() == 4);

    a.append(0); // size=5, 초과 → newCap = 8
    CHECK(a.cap() == 8);
  }

  SUBCASE("cap은 항상 size 이상이다") {
    DynamicArray<int> a;
    for (int i = 0; i < 100; ++i) {
      a.append(i);
      CHECK(a.cap() >= a.size());
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// append / emplaceBack
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - append") {
  SUBCASE("추가한 값이 순서대로 저장된다") {
    DynamicArray<int> a;
    a.append(10);
    a.append(20);
    a.append(30);

    CHECK(a.size() == 3);
    CHECK(a[0] == 10);
    CHECK(a[1] == 20);
    CHECK(a[2] == 30);
  }

  SUBCASE("재할당이 발생해도 기존 데이터가 유지된다") {
    DynamicArray<int> a;
    // cap을 초과해 grow를 여러 번 유발한다
    for (int i = 0; i < 100; ++i)
      a.append(i);

    for (int i = 0; i < 100; ++i)
      CHECK(a[i] == i);
  }

  SUBCASE("non-trivially copyable 타입도 올바르게 추가된다") {
    DynamicArray<std::string> a;
    a.append("foo");
    a.append("bar");

    CHECK(a.size() == 2);
    CHECK(a[0] == "foo");
    CHECK(a[1] == "bar");
  }
}

TEST_CASE("DynamicArray - emplaceBack") {
  struct Point {
    int x, y;
    Point(int x, int y) : x(x), y(y) {}
  };

  SUBCASE("여러 인자를 받아 제자리에서 생성한다") {
    DynamicArray<Point> a;
    a.emplaceBack(1, 2);
    a.emplaceBack(3, 4);

    CHECK(a.size() == 2);
    CHECK(a[0].x == 1);
    CHECK(a[0].y == 2);
    CHECK(a[1].x == 3);
    CHECK(a[1].y == 4);
  }

  SUBCASE("재할당이 발생해도 기존 데이터가 유지된다") {
    DynamicArray<Point> a;
    for (int i = 0; i < 50; ++i)
      a.emplaceBack(i, i * 2);

    for (int i = 0; i < 50; ++i) {
      CHECK(a[i].x == i);
      CHECK(a[i].y == i * 2);
    }
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// insert
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - insert") {
  SUBCASE("index == 0: 기존 원소들이 뒤로 밀린다") {
    DynamicArray<int> a;
    a.append(2);
    a.append(3);
    a.insert(0, 1);

    CHECK(a.size() == 3);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
  }

  SUBCASE("index == size: append와 동일한 결과이다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.insert(a.size(), 3);

    CHECK(a.size() == 3);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
  }

  SUBCASE("중간 삽입 후 앞뒤 원소 순서가 유지된다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(3);
    a.append(4);
    a.insert(1, 2); // [1, 2, 3, 4]

    CHECK(a.size() == 4);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
    CHECK(a[3] == 4);
  }

  SUBCASE("capacity 경계에서 삽입해도 데이터가 보존된다") {
    DynamicArray<int> a;
    a.reserve(4);
    a.append(1);
    a.append(2);
    a.append(3);
    a.append(4); // size == cap == 4, 다음 insert에서 grow 발생

    a.insert(2, 99); // [1, 2, 99, 3, 4]

    CHECK(a.size() == 5);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 99);
    CHECK(a[3] == 3);
    CHECK(a[4] == 4);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// reserve
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - reserve") {
  SUBCASE("현재 cap보다 큰 값: cap이 늘고 기존 데이터가 유지된다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);
    a.reserve(100);

    CHECK(a.cap() >= 100);
    CHECK(a.size() == 3);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
  }

  SUBCASE("현재 cap보다 작은 값: 아무것도 변하지 않는다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.reserve(100);
    size_t capBefore = a.cap();
    a.reserve(10);

    CHECK(a.cap() == capBefore);
    CHECK(a.size() == 2);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
  }

  SUBCASE("현재 cap과 같은 값: 아무것도 변하지 않는다") {
    DynamicArray<int> a;
    a.reserve(10);
    size_t capBefore = a.cap();
    a.reserve(capBefore);

    CHECK(a.cap() == capBefore);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// popBack
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - popBack") {
  SUBCASE("size가 줄고 back()이 바뀐다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);

    a.popBack();
    CHECK(a.size() == 2);
    CHECK(a.back() == 2);

    a.popBack();
    CHECK(a.size() == 1);
    CHECK(a.back() == 1);
  }

  SUBCASE("원소가 하나일 때 pop 하면 empty가 된다") {
    DynamicArray<int> a;
    a.append(42);
    a.popBack();
    CHECK(a.empty());
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// orderedRemove(first, last)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - orderedRemove(first, last)") {
  SUBCASE("중간 범위 제거 후 나머지 원소 순서가 유지된다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);
    a.append(4);
    a.append(5);

    a.orderedRemove(a.begin() + 1, a.begin() + 3); // [2, 3] 제거 → [1, 4, 5]

    CHECK(a.size() == 3);
    CHECK(a[0] == 1);
    CHECK(a[1] == 4);
    CHECK(a[2] == 5);
  }

  SUBCASE("전체 범위 제거 후 empty가 된다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);

    a.orderedRemove(a.begin(), a.end());

    CHECK(a.empty());
  }

  SUBCASE("단일 원소 범위 제거") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);

    a.orderedRemove(a.begin() + 1, a.begin() + 2); // [2] 제거 → [1, 3]

    CHECK(a.size() == 2);
    CHECK(a[0] == 1);
    CHECK(a[1] == 3);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// orderedRemove(index)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - orderedRemove(index)") {
  SUBCASE("반환값이 제거된 원소와 동일하고 뒤 원소가 앞으로 당겨진다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);
    a.append(4);

    int removed = a.orderedRemove(1); // [1, 3, 4]

    CHECK(removed == 2);
    CHECK(a.size() == 3);
    CHECK(a[0] == 1);
    CHECK(a[1] == 3);
    CHECK(a[2] == 4);
  }

  SUBCASE("index == 0: 첫 원소 제거 후 나머지가 앞으로 당겨진다") {
    DynamicArray<int> a;
    a.append(10);
    a.append(20);
    a.append(30);

    int removed = a.orderedRemove(0); // [20, 30]

    CHECK(removed == 10);
    CHECK(a.size() == 2);
    CHECK(a[0] == 20);
    CHECK(a[1] == 30);
  }

  SUBCASE("index == size-1: 마지막 원소를 제거한다") {
    DynamicArray<int> a;
    a.append(10);
    a.append(20);
    a.append(30);

    int removed = a.orderedRemove(a.size() - 1); // [10, 20]

    CHECK(removed == 30);
    CHECK(a.size() == 2);
    CHECK(a[0] == 10);
    CHECK(a[1] == 20);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// swapRemove
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - swapRemove") {
  SUBCASE("반환값이 제거된 원소와 동일하고 마지막 원소가 그 자리로 이동한다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);
    a.append(4);

    int removed = a.swapRemove(1); // 2 제거, 4가 index 1로 이동 → [1, 4, 3]

    CHECK(removed == 2);
    CHECK(a.size() == 3);
    CHECK(a[0] == 1);
    CHECK(a[1] == 4);
    CHECK(a[2] == 3);
  }

  SUBCASE("마지막 원소 제거 시 단순 pop처럼 동작한다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);

    int removed = a.swapRemove(a.size() - 1); // [1, 2]

    CHECK(removed == 3);
    CHECK(a.size() == 2);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// resize
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - resize(n)") {
  SUBCASE("크게 늘릴 때 새 원소가 기본값으로 채워진다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.resize(5);

    CHECK(a.size() == 5);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 0); // int{} == 0
    CHECK(a[3] == 0);
    CHECK(a[4] == 0);
  }

  SUBCASE("줄일 때 size가 감소하고 앞 원소가 유지된다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);
    a.append(4);
    a.resize(2);

    CHECK(a.size() == 2);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
  }

  SUBCASE("현재 size와 동일하면 아무것도 변하지 않는다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);
    size_t capBefore = a.cap();
    a.resize(3);

    CHECK(a.size() == 3);
    CHECK(a.cap() == capBefore);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
  }
}

TEST_CASE("DynamicArray - resize(n, value)") {
  SUBCASE("크게 늘릴 때 지정 값으로 채워진다") {
    DynamicArray<int> a;
    a.append(1);
    a.resize(4, 99);

    CHECK(a.size() == 4);
    CHECK(a[0] == 1);
    CHECK(a[1] == 99);
    CHECK(a[2] == 99);
    CHECK(a[3] == 99);
  }

  SUBCASE("줄일 때 size가 감소하고 앞 원소가 유지된다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);
    a.resize(1, 99); // value는 무시됨

    CHECK(a.size() == 1);
    CHECK(a[0] == 1);
  }

  SUBCASE("현재 size와 동일하면 아무것도 변하지 않는다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    size_t capBefore = a.cap();
    a.resize(2, 99);

    CHECK(a.size() == 2);
    CHECK(a.cap() == capBefore);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// shrinkToFit
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - shrinkToFit") {
  SUBCASE("여유 capacity가 있을 때 cap이 size와 같아진다") {
    DynamicArray<int> a;
    a.reserve(100);
    a.append(1);
    a.append(2);
    a.append(3);

    a.shrinkToFit();

    CHECK(a.cap() == a.size());
    CHECK(a.size() == 3);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
  }

  SUBCASE("size == 0이면 data()가 nullptr이 된다") {
    DynamicArray<int> a;
    a.reserve(100);
    a.shrinkToFit();

    CHECK(a.data() == nullptr);
    CHECK(a.empty());
  }

  SUBCASE("이미 cap == size이면 아무것도 변하지 않는다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.shrinkToFit(); // cap == size가 되도록 먼저 줄임
    size_t capBefore = a.cap();
    a.shrinkToFit(); // 두 번째 호출은 no-op

    CHECK(a.cap() == capBefore);
    CHECK(a.size() == 2);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// 원소 접근 (operator[], at, data, front, back)
// ─────────────────────────────────────────────────────────────────────────────

// NOTE: at() 범위 초과 시 abort()를 호출한다.
//       abort()는 예외가 아니므로 doctest의 CHECK_THROWS로는 테스트 불가.

TEST_CASE("DynamicArray - operator[] / at") {
  SUBCASE("같은 index에서 동일한 값을 반환한다") {
    DynamicArray<int> a;
    a.append(10);
    a.append(20);
    a.append(30);

    CHECK(a[0] == a.at(0));
    CHECK(a[1] == a.at(1));
    CHECK(a[2] == a.at(2));
  }

  SUBCASE("const 버전도 동일한 값을 반환한다") {
    DynamicArray<int> tmp;
    tmp.append(10);
    tmp.append(20);
    const DynamicArray<int>& a = tmp;

    CHECK(a[0] == a.at(0));
    CHECK(a[1] == a.at(1));
  }
}

TEST_CASE("DynamicArray - data") {
  SUBCASE("첫 원소의 주소와 일치한다") {
    DynamicArray<int> a;
    a.append(42);
    a.append(99);

    CHECK(a.data() == &a[0]);
    CHECK(*a.data() == 42);
  }

  SUBCASE("const 버전도 첫 원소의 주소와 일치한다") {
    DynamicArray<int> tmp;
    tmp.append(42);
    const DynamicArray<int>& a = tmp;

    CHECK(a.data() == &a[0]);
    CHECK(*a.data() == 42);
  }
}

TEST_CASE("DynamicArray - front / back") {
  SUBCASE("front()는 operator[](0), back()은 operator[](size-1)과 같다") {
    DynamicArray<int> a;
    a.append(1);
    a.append(2);
    a.append(3);

    CHECK(a.front() == a[0]);
    CHECK(a.back() == a[a.size() - 1]);
    CHECK(a.front() == 1);
    CHECK(a.back() == 3);
  }

  SUBCASE("const 버전도 동일하게 동작한다") {
    DynamicArray<int> tmp;
    tmp.append(1);
    tmp.append(2);
    tmp.append(3);
    const DynamicArray<int>& a = tmp;

    CHECK(a.front() == a[0]);
    CHECK(a.back() == a[a.size() - 1]);
    CHECK(a.front() == 1);
    CHECK(a.back() == 3);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// 비교 연산자 (==, <=>)
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - operator==") {
  SUBCASE("동일 내용이면 true이다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(2); a.append(3);
    b.append(1); b.append(2); b.append(3);
    CHECK(a == b);
  }

  SUBCASE("내용이 다르고 크기가 같으면 false이다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(2); a.append(3);
    b.append(1); b.append(9); b.append(3);
    CHECK_FALSE(a == b);
  }

  SUBCASE("크기가 다르면 false이다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(2);
    b.append(1); b.append(2); b.append(3);
    CHECK_FALSE(a == b);
  }

  SUBCASE("빈 배열끼리는 같다") {
    DynamicArray<int> a, b;
    CHECK(a == b);
  }
}

TEST_CASE("DynamicArray - operator<=>") {
  SUBCASE("앞 원소가 다르면 그 원소로 대소가 결정된다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(3);
    b.append(1); b.append(2);
    CHECK(a > b);
    CHECK(b < a);
  }

  SUBCASE("접두사가 같으면 길이가 짧은 쪽이 작다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(2);
    b.append(1); b.append(2); b.append(3);
    CHECK(a < b);
    CHECK(b > a);
  }

  SUBCASE("동일 내용이면 <= 와 >= 가 모두 성립한다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(2);
    b.append(1); b.append(2);
    CHECK(a <= b);
    CHECK(a >= b);
    CHECK_FALSE(a < b);
    CHECK_FALSE(a > b);
  }

  SUBCASE("빈 배열끼리는 같다") {
    DynamicArray<int> a, b;
    CHECK(a <= b);
    CHECK(a >= b);
    CHECK_FALSE(a < b);
    CHECK_FALSE(a > b);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// operator+= / operator+
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - operator+=") {
  SUBCASE("b의 원소가 a 뒤에 이어붙여진다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(2);
    b.append(3); b.append(4);
    a += b;

    CHECK(a.size() == 4);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
    CHECK(a[3] == 4);
  }

  SUBCASE("b는 변하지 않는다") {
    DynamicArray<int> a, b;
    a.append(1);
    b.append(2); b.append(3);
    a += b;

    CHECK(b.size() == 2);
    CHECK(b[0] == 2);
    CHECK(b[1] == 3);
  }

  SUBCASE("빈 배열과의 연산") {
    DynamicArray<int> a, empty;
    a.append(1); a.append(2);

    a += empty;
    CHECK(a.size() == 2);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);

    empty += a;
    CHECK(empty.size() == 2);
    CHECK(empty[0] == 1);
    CHECK(empty[1] == 2);
  }
}

TEST_CASE("DynamicArray - operator+") {
  SUBCASE("두 배열이 합쳐진 새 배열이 반환된다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(2);
    b.append(3); b.append(4);
    DynamicArray<int> c = a + b;

    CHECK(c.size() == 4);
    CHECK(c[0] == 1);
    CHECK(c[1] == 2);
    CHECK(c[2] == 3);
    CHECK(c[3] == 4);
  }

  SUBCASE("a와 b는 변하지 않는다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(2);
    b.append(3); b.append(4);
    DynamicArray<int> c = a + b;

    CHECK(a.size() == 2);
    CHECK(a[0] == 1); CHECK(a[1] == 2);
    CHECK(b.size() == 2);
    CHECK(b[0] == 3); CHECK(b[1] == 4);
  }

  SUBCASE("반환된 배열은 a, b와 다른 메모리이다") {
    DynamicArray<int> a, b;
    a.append(1);
    b.append(2);
    DynamicArray<int> c = a + b;

    CHECK(c.data() != a.data());
    CHECK(c.data() != b.data());
  }

  SUBCASE("빈 배열과의 연산") {
    DynamicArray<int> a, empty;
    a.append(1); a.append(2);

    DynamicArray<int> r1 = a + empty;
    CHECK(r1.size() == 2);
    CHECK(r1[0] == 1); CHECK(r1[1] == 2);

    DynamicArray<int> r2 = empty + a;
    CHECK(r2.size() == 2);
    CHECK(r2[0] == 1); CHECK(r2[1] == 2);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// swap
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - swap") {
  SUBCASE("두 배열의 내용, size, cap이 서로 교환된다") {
    DynamicArray<int> a, b;
    a.append(1); a.append(2); a.append(3);
    b.reserve(10);
    b.append(9); b.append(8);

    size_t sizeA = a.size(), capA = a.cap();
    size_t sizeB = b.size(), capB = b.cap();

    a.swap(b);

    CHECK(a.size() == sizeB);
    CHECK(a.cap() == capB);
    CHECK(a[0] == 9);
    CHECK(a[1] == 8);

    CHECK(b.size() == sizeA);
    CHECK(b.cap() == capA);
    CHECK(b[0] == 1);
    CHECK(b[1] == 2);
    CHECK(b[2] == 3);
  }

  SUBCASE("자기 자신과 swap해도 안전하다") {
    DynamicArray<int> a;
    a.append(1); a.append(2); a.append(3);

    a.swap(a);

    CHECK(a.size() == 3);
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// clear
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - clear") {
  SUBCASE("호출 후 size == 0이고 empty가 된다") {
    DynamicArray<int> a;
    a.append(1); a.append(2); a.append(3);
    a.clear();

    CHECK(a.size() == 0);
    CHECK(a.empty());
  }

  SUBCASE("cap은 그대로 유지된다") {
    DynamicArray<int> a;
    a.reserve(100);
    a.append(1); a.append(2); a.append(3);
    size_t capBefore = a.cap();
    a.clear();

    CHECK(a.cap() == capBefore);
  }

  SUBCASE("이미 빈 배열에서 호출해도 안전하다") {
    DynamicArray<int> a;
    a.clear();

    CHECK(a.empty());
    CHECK(a.size() == 0);
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// 이터레이터
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - 이터레이터") {
  SUBCASE("begin/end로 순서대로 순회한다") {
    DynamicArray<int> a;
    a.append(1); a.append(2); a.append(3);

    int expected = 1;
    for (auto v : a)
      CHECK(v == expected++);
  }

  SUBCASE("cbegin/cend로 const 순회한다") {
    DynamicArray<int> tmp;
    tmp.append(1); tmp.append(2); tmp.append(3);
    const DynamicArray<int>& a = tmp;

    int expected = 1;
    for (auto it = a.cbegin(); it != a.cend(); ++it)
      CHECK(*it == expected++);
  }

  SUBCASE("rbegin/rend로 역순 순회한다") {
    DynamicArray<int> a;
    a.append(1); a.append(2); a.append(3);

    int expected = 3;
    for (auto it = a.rbegin(); it != a.rend(); ++it)
      CHECK(*it == expected--);
  }

  SUBCASE("crbegin/crend로 const 역순 순회한다") {
    DynamicArray<int> tmp;
    tmp.append(1); tmp.append(2); tmp.append(3);
    const DynamicArray<int>& a = tmp;

    int expected = 3;
    for (auto it = a.crbegin(); it != a.crend(); ++it)
      CHECK(*it == expected--);
  }

  SUBCASE("빈 배열에서 begin() == end()이다") {
    DynamicArray<int> a;
    CHECK(a.begin() == a.end());
    CHECK(a.cbegin() == a.cend());
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Non-trivially copyable 타입
// ─────────────────────────────────────────────────────────────────────────────

TEST_CASE("DynamicArray - non-trivially copyable 타입") {
  // 복사 발생 여부를 추적하는 헬퍼 타입
  struct NoCopyTracker {
    int   value;
    bool* copyOccurred;

    NoCopyTracker(int v, bool* c) : value(v), copyOccurred(c) {}
    NoCopyTracker(const NoCopyTracker& o)
      : value(o.value), copyOccurred(o.copyOccurred) { *copyOccurred = true; }
    NoCopyTracker(NoCopyTracker&& o) noexcept
      : value(o.value), copyOccurred(o.copyOccurred) {}
    NoCopyTracker& operator=(const NoCopyTracker& o) {
      value = o.value; copyOccurred = o.copyOccurred; *copyOccurred = true; return *this;
    }
    NoCopyTracker& operator=(NoCopyTracker&& o) noexcept {
      value = o.value; copyOccurred = o.copyOccurred; return *this;
    }
    ~NoCopyTracker() {}
  };

  SUBCASE("복사 생성자: shallow copy 없이 깊은 복사가 수행된다") {
    DynamicArray<std::string> a;
    a.append("hello"); a.append("world");

    DynamicArray<std::string> b(a);
    b[0] = "bye";

    CHECK(a[0] == "hello"); // shallow copy였다면 같이 바뀜
  }

  SUBCASE("orderedRemove 후 제거된 원소가 올바르게 소멸된다") {
    // shared_ptr의 use_count로 소멸 여부를 간접 검증
    auto sp1 = std::make_shared<int>(1);
    auto sp2 = std::make_shared<int>(2);
    auto sp3 = std::make_shared<int>(3);

    DynamicArray<std::shared_ptr<int>> a;
    a.append(sp1); a.append(sp2); a.append(sp3);

    a.orderedRemove(1); // sp2 제거, 반환값 버림

    CHECK(sp1.use_count() == 2); // a[0]에 여전히 존재
    CHECK(sp2.use_count() == 1); // 제거되어 a에 없음
    CHECK(sp3.use_count() == 2); // a[1]으로 이동
  }

  SUBCASE("swapRemove 후 제거된 원소가 올바르게 소멸된다") {
    auto sp1 = std::make_shared<int>(1);
    auto sp2 = std::make_shared<int>(2);
    auto sp3 = std::make_shared<int>(3);

    DynamicArray<std::shared_ptr<int>> a;
    a.append(sp1); a.append(sp2); a.append(sp3);

    a.swapRemove(0); // sp1 제거, sp3이 index 0으로 이동, 반환값 버림

    CHECK(sp1.use_count() == 1); // 제거됨
    CHECK(sp2.use_count() == 2); // a[1]에 여전히 존재
    CHECK(sp3.use_count() == 2); // a[0]으로 이동
  }

  SUBCASE("grow 시 복사가 아닌 이동이 사용된다") {
    bool copyOccurred = false;

    DynamicArray<NoCopyTracker> a;
    a.reserve(2);
    a.emplaceBack(1, &copyOccurred);
    a.emplaceBack(2, &copyOccurred); // size == cap == 2
    copyOccurred = false;            // reset

    a.emplaceBack(3, &copyOccurred); // grow 발생

    CHECK_FALSE(copyOccurred);
    CHECK(a[0].value == 1);
    CHECK(a[1].value == 2);
    CHECK(a[2].value == 3);
  }

  SUBCASE("insert 시 원소 이동에 복사가 사용되지 않는다") {
    bool copyOccurred = false;

    DynamicArray<NoCopyTracker> a;
    a.emplaceBack(1, &copyOccurred);
    a.emplaceBack(3, &copyOccurred);
    a.emplaceBack(4, &copyOccurred);
    copyOccurred = false; // reset

    a.insert(1, NoCopyTracker(2, &copyOccurred)); // index 1에 삽입

    CHECK_FALSE(copyOccurred);
    CHECK(a[0].value == 1);
    CHECK(a[1].value == 2);
    CHECK(a[2].value == 3);
    CHECK(a[3].value == 4);
  }
}
