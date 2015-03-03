// Copyright 2014 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/v8.h"

#include "test/cctest/compiler/function-tester.h"

using namespace v8::internal;
using namespace v8::internal::compiler;

TEST(Throw) {
  i::FLAG_turbo_exceptions = true;
  FunctionTester T("(function(a,b) { if (a) { throw b; } else { return b; }})");

  T.CheckThrows(T.true_value(), T.NewObject("new Error"));
  T.CheckCall(T.Val(23), T.false_value(), T.Val(23));
}


TEST(ThrowSourcePosition) {
  static const char* src =
      "(function(a, b) {        \n"
      "  if (a == 1) throw 1;   \n"
      "  if (a == 2) {throw 2}  \n"
      "  if (a == 3) {0;throw 3}\n"
      "  throw 4;               \n"
      "})                       ";
  FunctionTester T(src);
  v8::Handle<v8::Message> message;

  message = T.CheckThrowsReturnMessage(T.Val(1), T.undefined());
  CHECK(!message.IsEmpty());
  CHECK_EQ(2, message->GetLineNumber());
  CHECK_EQ(40, message->GetStartPosition());

  message = T.CheckThrowsReturnMessage(T.Val(2), T.undefined());
  CHECK(!message.IsEmpty());
  CHECK_EQ(3, message->GetLineNumber());
  CHECK_EQ(67, message->GetStartPosition());

  message = T.CheckThrowsReturnMessage(T.Val(3), T.undefined());
  CHECK(!message.IsEmpty());
  CHECK_EQ(4, message->GetLineNumber());
  CHECK_EQ(95, message->GetStartPosition());
}


// TODO(mstarzinger): Increase test coverage by having similar tests within the
// mjsunit suite to also test integration with other components (e.g. OSR).


TEST(Catch) {
  i::FLAG_turbo_exceptions = true;
  const char* src =
      "(function(a,b) {"
      "  var r = '-';"
      "  try {"
      "    r += 'A-';"
      "    throw 'B-';"
      "  } catch (e) {"
      "    r += e;"
      "  }"
      "  return r;"
      "})";
  FunctionTester T(src);

  T.CheckCall(T.Val("-A-B-"));
}


TEST(CatchNested) {
  i::FLAG_turbo_exceptions = true;
  const char* src =
      "(function(a,b) {"
      "  var r = '-';"
      "  try {"
      "    r += 'A-';"
      "    throw 'C-';"
      "  } catch (e) {"
      "    try {"
      "      throw 'B-';"
      "    } catch (e) {"
      "      r += e;"
      "    }"
      "    r += e;"
      "  }"
      "  return r;"
      "})";
  FunctionTester T(src);

  T.CheckCall(T.Val("-A-B-C-"));
}


TEST(CatchBreak) {
  i::FLAG_turbo_exceptions = true;
  const char* src =
      "(function(a,b) {"
      "  var r = '-';"
      "  L: try {"
      "    r += 'A-';"
      "    if (a) break L;"
      "    r += 'B-';"
      "    throw 'C-';"
      "  } catch (e) {"
      "    if (b) break L;"
      "    r += e;"
      "  }"
      "  r += 'D-';"
      "  return r;"
      "})";
  FunctionTester T(src);

  T.CheckCall(T.Val("-A-D-"), T.true_value(), T.false_value());
  T.CheckCall(T.Val("-A-B-D-"), T.false_value(), T.true_value());
  T.CheckCall(T.Val("-A-B-C-D-"), T.false_value(), T.false_value());
}


TEST(CatchCall) {
  i::FLAG_turbo_exceptions = true;
  const char* src =
      "(function(fun) {"
      "  var r = '-';"
      "  try {"
      "    r += 'A-';"
      "    return r + 'B-' + fun();"
      "  } catch (e) {"
      "    r += e;"
      "  }"
      "  return r;"
      "})";
  FunctionTester T(src);

  CompileRun("function thrower() { throw 'T-'; }");
#if 0  // TODO(mstarzinger): Enable once we have exception handlers.
  T.CheckCall(T.Val("-A-T-"), T.NewFunction("thrower"));
#endif
  CompileRun("function returner() { return 'R-'; }");
  T.CheckCall(T.Val("-A-B-R-"), T.NewFunction("returner"));
}


TEST(Finally) {
  i::FLAG_turbo_exceptions = true;
  const char* src =
      "(function(a,b) {"
      "  var r = '-';"
      "  try {"
      "    r += 'A-';"
      "  } finally {"
      "    r += 'B-';"
      "  }"
      "  return r;"
      "})";
  FunctionTester T(src);

  T.CheckCall(T.Val("-A-B-"));
}


TEST(FinallyBreak) {
  i::FLAG_turbo_exceptions = true;
  const char* src =
      "(function(a,b) {"
      "  var r = '-';"
      "  L: try {"
      "    r += 'A-';"
      "    if (a) return r;"
      "    r += 'B-';"
      "    if (b) break L;"
      "    r += 'C-';"
      "  } finally {"
      "    r += 'D-';"
      "  }"
      "  return r;"
      "})";
  FunctionTester T(src);

  T.CheckCall(T.Val("-A-"), T.true_value(), T.false_value());
  T.CheckCall(T.Val("-A-B-D-"), T.false_value(), T.true_value());
  T.CheckCall(T.Val("-A-B-C-D-"), T.false_value(), T.false_value());
}


TEST(DeoptTry) {
  i::FLAG_turbo_exceptions = true;
  i::FLAG_turbo_deoptimization = true;
  const char* src =
      "(function f(a) {"
      "  try {"
      "    %DeoptimizeFunction(f);"
      "    throw a;"
      "  } catch (e) {"
      "    return e + 1;"
      "  }"
      "})";
  FunctionTester T(src);

#if 0  // TODO(mstarzinger): Enable once we can.
  T.CheckCall(T.Val(2), T.Val(1));
#endif
}


TEST(DeoptCatch) {
  i::FLAG_turbo_exceptions = true;
  i::FLAG_turbo_deoptimization = true;
  const char* src =
      "(function f(a) {"
      "  try {"
      "    throw a;"
      "  } catch (e) {"
      "    %DeoptimizeFunction(f);"
      "    return e + 1;"
      "  }"
      "})";
  FunctionTester T(src);

  T.CheckCall(T.Val(2), T.Val(1));
}


TEST(DeoptFinallyReturn) {
  i::FLAG_turbo_exceptions = true;
  i::FLAG_turbo_deoptimization = true;
  const char* src =
      "(function f(a) {"
      "  try {"
      "    throw a;"
      "  } finally {"
      "    %DeoptimizeFunction(f);"
      "    return a + 1;"
      "  }"
      "})";
  FunctionTester T(src);

  T.CheckCall(T.Val(2), T.Val(1));
}


TEST(DeoptFinallyReThrow) {
  i::FLAG_turbo_exceptions = true;
  i::FLAG_turbo_deoptimization = true;
  const char* src =
      "(function f(a) {"
      "  try {"
      "    throw a;"
      "  } finally {"
      "    %DeoptimizeFunction(f);"
      "  }"
      "})";
  FunctionTester T(src);

#if 0  // TODO(mstarzinger): Enable once we can.
  T.CheckThrows(T.NewObject("new Error"), T.Val(1));
#endif
}
