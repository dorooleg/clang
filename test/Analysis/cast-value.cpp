// RUN: %clang_analyze_cc1 \
// RUN:  -analyzer-checker=core,apiModeling.llvm.CastValue,debug.ExprInspection\
// RUN:  -verify=logic %s
// RUN: %clang_analyze_cc1 \
// RUN:  -analyzer-checker=core,apiModeling.llvm.CastValue \
// RUN:  -analyzer-output=text -verify %s

void clang_analyzer_numTimesReached();
void clang_analyzer_warnIfReached();
void clang_analyzer_eval(bool);

namespace llvm {
template <class X, class Y>
const X *cast(Y Value);

template <class X, class Y>
const X *dyn_cast(Y *Value);
template <class X, class Y>
const X &dyn_cast(Y &Value);

template <class X, class Y>
const X *cast_or_null(Y Value);

template <class X, class Y>
const X *dyn_cast_or_null(Y *Value);
template <class X, class Y>
const X *dyn_cast_or_null(Y &Value);
} // namespace llvm

namespace clang {
struct Shape {
  template <typename T>
  const T *castAs() const;

  template <typename T>
  const T *getAs() const;
};
class Triangle : public Shape {};
class Circle : public Shape {};
} // namespace clang

using namespace llvm;
using namespace clang;

namespace test_cast {
void evalLogic(const Shape *S) {
  const Circle *C = cast<Circle>(S);
  clang_analyzer_numTimesReached(); // logic-warning {{1}}

  if (S && C)
    clang_analyzer_eval(C == S); // logic-warning {{TRUE}}

  if (S && !C)
    clang_analyzer_warnIfReached(); // no-warning

  if (!S)
    clang_analyzer_warnIfReached(); // no-warning
}
} // namespace test_cast

namespace test_dyn_cast {
void evalLogic(const Shape *S) {
  const Circle *C = dyn_cast<Circle>(S);
  clang_analyzer_numTimesReached(); // logic-warning {{2}}

  if (S && C)
    clang_analyzer_eval(C == S); // logic-warning {{TRUE}}

  if (S && !C)
    clang_analyzer_warnIfReached(); // logic-warning {{REACHABLE}}

  if (!S)
    clang_analyzer_warnIfReached(); // no-warning
}
} // namespace test_dyn_cast

namespace test_cast_or_null {
void evalLogic(const Shape *S) {
  const Circle *C = cast_or_null<Circle>(S);
  clang_analyzer_numTimesReached(); // logic-warning {{2}}

  if (S && C)
    clang_analyzer_eval(C == S); // logic-warning {{TRUE}}

  if (S && !C)
    clang_analyzer_warnIfReached(); // no-warning

  if (!S)
    clang_analyzer_eval(!C); // logic-warning {{TRUE}}
}
} // namespace test_cast_or_null

namespace test_dyn_cast_or_null {
void evalLogic(const Shape *S) {
  const Circle *C = dyn_cast_or_null<Circle>(S);
  clang_analyzer_numTimesReached(); // logic-warning {{3}}

  if (S && C)
    clang_analyzer_eval(C == S); // logic-warning {{TRUE}}

  if (S && !C)
    clang_analyzer_warnIfReached(); // logic-warning {{REACHABLE}}

  if (!S)
    clang_analyzer_eval(!C); // logic-warning {{TRUE}}
}
} // namespace test_dyn_cast_or_null

namespace test_cast_as {
void evalLogic(const Shape *S) {
  const Circle *C = S->castAs<Circle>();
  clang_analyzer_numTimesReached(); // logic-warning {{1}}

  if (S && C)
    clang_analyzer_eval(C == S);
  // logic-warning@-1 {{TRUE}}

  if (S && !C)
    clang_analyzer_warnIfReached(); // no-warning

  if (!S)
    clang_analyzer_warnIfReached(); // no-warning
}
} // namespace test_cast_as

namespace test_get_as {
void evalLogic(const Shape *S) {
  const Circle *C = S->getAs<Circle>();
  clang_analyzer_numTimesReached(); // logic-warning {{2}}

  if (S && C)
    clang_analyzer_eval(C == S);
  // logic-warning@-1 {{TRUE}}

  if (S && !C)
    clang_analyzer_warnIfReached(); // logic-warning {{REACHABLE}}

  if (!S)
    clang_analyzer_warnIfReached(); // no-warning
}
} // namespace test_get_as

namespace test_notes {
void evalReferences(const Shape &S) {
  const auto &C = dyn_cast<Circle>(S);
  // expected-note@-1 {{Assuming dynamic cast from 'Shape' to 'Circle' fails}}
  // expected-note@-2 {{Dereference of null pointer}}
  // expected-warning@-3 {{Dereference of null pointer}}
  // logic-warning@-4 {{Dereference of null pointer}}
}

void evalNonNullParamNonNullReturnReference(const Shape &S) {
  const auto *C = dyn_cast_or_null<Circle>(S);
  // expected-note@-1 {{Assuming dynamic cast from 'Shape' to 'Circle' succeeds}}
  // expected-note@-2 {{Assuming pointer value is null}}
  // expected-note@-3 {{'C' initialized here}}

  (void)(1 / !(bool)C);
  // expected-note@-1 {{'C' is non-null}}
  // expected-note@-2 {{Division by zero}}
  // expected-warning@-3 {{Division by zero}}
  // logic-warning@-4 {{Division by zero}}
}

void evalNonNullParamNonNullReturn(const Shape *S) {
  const auto *C = cast<Circle>(S);
  // expected-note@-1 {{Checked cast from 'Shape' to 'Circle' succeeds}}
  // expected-note@-2 {{Assuming pointer value is null}}
  // expected-note@-3 {{'C' initialized here}}

  (void)(1 / !(bool)C);
  // expected-note@-1 {{'C' is non-null}}
  // expected-note@-2 {{Division by zero}}
  // expected-warning@-3 {{Division by zero}}
  // logic-warning@-4 {{Division by zero}}
}

void evalNonNullParamNullReturn(const Shape *S) {
  const auto *C = dyn_cast_or_null<Circle>(S);
  // expected-note@-1 {{Assuming dynamic cast from 'Shape' to 'Circle' fails}}
  // expected-note@-2 {{Assuming pointer value is null}}

  if (const auto *T = dyn_cast_or_null<Triangle>(S)) {
    // expected-note@-1 {{Assuming dynamic cast from 'Shape' to 'Triangle' succeeds}}
    // expected-note@-2 {{'T' initialized here}}
    // expected-note@-3 {{'T' is non-null}}
    // expected-note@-4 {{Taking true branch}}

    (void)(1 / !T);
    // expected-note@-1 {{'T' is non-null}}
    // expected-note@-2 {{Division by zero}}
    // expected-warning@-3 {{Division by zero}}
    // logic-warning@-4 {{Division by zero}}
  }
}

void evalNullParamNullReturn(const Shape *S) {
  const auto *C = dyn_cast_or_null<Circle>(S);
  // expected-note@-1 {{Assuming null pointer is passed into cast}}
  // expected-note@-2 {{'C' initialized to a null pointer value}}

  (void)(1 / (bool)C);
  // expected-note@-1 {{Division by zero}}
  // expected-warning@-2 {{Division by zero}}
  // logic-warning@-3 {{Division by zero}}
}

void evalZeroParamNonNullReturnPointer(const Shape *S) {
  const auto *C = S->castAs<Circle>();
  // expected-note@-1 {{Assuming pointer value is null}}
  // expected-note@-2 {{Checked cast to 'Circle' succeeds}}
  // expected-note@-3 {{'C' initialized here}}

  (void)(1 / !(bool)C);
  // expected-note@-1 {{'C' is non-null}}
  // expected-note@-2 {{Division by zero}}
  // expected-warning@-3 {{Division by zero}}
  // logic-warning@-4 {{Division by zero}}
}

void evalZeroParamNonNullReturn(const Shape &S) {
  const auto *C = S.castAs<Circle>();
  // expected-note@-1 {{Checked cast to 'Circle' succeeds}}
  // expected-note@-2 {{'C' initialized here}}

  (void)(1 / !(bool)C);
  // expected-note@-1 {{'C' is non-null}}
  // expected-note@-2 {{Division by zero}}
  // expected-warning@-3 {{Division by zero}}
  // logic-warning@-4 {{Division by zero}}
}

void evalZeroParamNullReturn(const Shape &S) {
  const auto *C = S.getAs<Circle>();
  // expected-note@-1 {{Assuming dynamic cast to 'Circle' fails}}
  // expected-note@-2 {{'C' initialized to a null pointer value}}

  (void)(1 / (bool)C);
  // expected-note@-1 {{Division by zero}}
  // expected-warning@-2 {{Division by zero}}
  // logic-warning@-3 {{Division by zero}}
}
} // namespace test_notes
