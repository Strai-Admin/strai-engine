// Copyright 2017 The Strai Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//  http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>
#include <strai/api.h>
#include <strai/api/serializer.h>

#include "cpp/src/strai/runtime/task/task_executor.h"
#include "cpp/src/strai/util/function_helper.h"

int Return() { return 1; }
int PlusOne(int x) { return x + 1; }
int PlusTwo(int x, int y) { return x + y; }

int out_for_void_func = 0;
int out_for_void_func_no_args = 0;

void VoidFuncNoArgs() { out_for_void_func = 1; }
void VoidFuncWithArgs(int x, int y) { out_for_void_func_no_args = (x + y); }

int NotRegisteredFunc(int x) { return x; }

void ExceptionFunc(int x) { throw std::invalid_argument(std::to_string(x)); }

std::string Concat1(std::string &&a, std::string &&b) { return a + b; }

std::string Concat2(const std::string &a, std::string &&b) { return a + b; }

std::string Concat3(std::string &a, std::string &b) { return a + b; }

int OverloadFunc() {
  std::cout << "OverloadFunc with no argument\n";
  return 1;
}

int OverloadFunc(int i) {
  std::cout << "OverloadFunc with one argument\n";
  return i + 1;
}

int OverloadFunc(int i, int j) {
  std::cout << "OverloadFunc with two arguments\n";
  return i + j;
}

STRAI_REMOTE(STRAI_FUNC(OverloadFunc));
STRAI_REMOTE(STRAI_FUNC(OverloadFunc, int));
STRAI_REMOTE(STRAI_FUNC(OverloadFunc, int, int));

class DummyObject {
 public:
  int count;

  MSGPACK_DEFINE(count);
  DummyObject() = default;
  DummyObject(int init) { count = init; }

  int Add(int x, int y) { return x + y; }

  std::string Concat1(std::string &&a, std::string &&b) { return a + b; }

  std::string Concat2(const std::string &a, std::string &&b) { return a + b; }

  ~DummyObject() { std::cout << "destruct DummyObject\n"; }

  static DummyObject *FactoryCreate(int init) { return new DummyObject(init); }
};
STRAI_REMOTE(DummyObject::FactoryCreate);
STRAI_REMOTE(&DummyObject::Add, &DummyObject::Concat1, &DummyObject::Concat2);

STRAI_REMOTE(PlusOne, Concat1, Concat2, Concat3);
STRAI_REMOTE(PlusTwo, VoidFuncNoArgs, VoidFuncWithArgs, ExceptionFunc);

struct Base {
  static Base *FactoryCreate() { return new Base(); }
  virtual int Foo() { return 1; }
  virtual int Bar() { return 2; }
  virtual ~Base() {}
};

struct Base1 {
  static Base1 *FactoryCreate() { return new Base1(); }
  virtual int Foo() { return 3; }
  virtual int Bar() { return 4; }
  virtual ~Base1() {}
};
STRAI_REMOTE(Base::FactoryCreate, &Base::Foo, &Base::Bar);
STRAI_REMOTE(Base1::FactoryCreate, &Base1::Foo, &Base1::Bar);

struct Derived : public Base {
  static Derived *FactoryCreate() { return new Derived(); }
  int Foo() override { return 10; }
  int Bar() override { return 20; }
};
STRAI_REMOTE(Derived::FactoryCreate);

TEST(StraiApiTest, DuplicateRegister) {
  bool r = FunctionManager::Instance().RegisterRemoteFunction("Return", Return);
  EXPECT_TRUE(r);

  /// Duplicate register
  EXPECT_THROW(FunctionManager::Instance().RegisterRemoteFunction("Return", Return),
               strai::internal::StraiException);
  EXPECT_THROW(FunctionManager::Instance().RegisterRemoteFunction("PlusOne", PlusOne),
               strai::internal::StraiException);
}

TEST(StraiApiTest, NormalTask) {
  auto r = strai::Task(Return).Remote();
  EXPECT_EQ(1, *(r.Get()));

  auto r1 = strai::Task(PlusOne).Remote(1);
  EXPECT_EQ(2, *(r1.Get()));
}

TEST(StraiApiTest, VoidFunction) {
  auto r2 = strai::Task(VoidFuncNoArgs).Remote();
  r2.Get();
  EXPECT_EQ(1, out_for_void_func);

  auto r3 = strai::Task(VoidFuncWithArgs).Remote(1, 2);
  r3.Get();
  EXPECT_EQ(3, out_for_void_func_no_args);
}

TEST(StraiApiTest, ReferenceArgs) {
  auto r = strai::Task(Concat1).Remote("a", "b");
  EXPECT_EQ(*(r.Get()), "ab");
  std::string a = "a";
  std::string b = "b";
  auto r1 = strai::Task(Concat1).Remote(std::move(a), std::move(b));
  EXPECT_EQ(*(r.Get()), *(r1.Get()));

  std::string str = "a";
  std::string str1 = "b";
  auto r2 = strai::Task(Concat2).Remote(str, std::move(str1));

  std::string str2 = "b";
  auto r3 = strai::Task(Concat3).Remote(str, str2);
  EXPECT_EQ(*(r2.Get()), *(r3.Get()));

  strai::ActorHandle<DummyObject> actor = strai::Actor(DummyObject::FactoryCreate).Remote(1);
  auto r4 = actor.Task(&DummyObject::Concat1).Remote("a", "b");
  auto r5 = actor.Task(&DummyObject::Concat2).Remote(str, "b");
  EXPECT_EQ(*(r4.Get()), *(r5.Get()));
}

TEST(StraiApiTest, VirtualFunctions) {
  auto actor = strai::Actor(Base::FactoryCreate).Remote();
  auto r = actor.Task(&Base::Foo).Remote();
  auto r1 = actor.Task(&Base::Bar).Remote();
  auto actor1 = strai::Actor(Base1::FactoryCreate).Remote();
  auto r2 = actor1.Task(&Base1::Foo).Remote();
  auto r3 = actor1.Task(&Base1::Bar).Remote();
  EXPECT_EQ(*(r.Get()), 1);
  EXPECT_EQ(*(r1.Get()), 2);
  EXPECT_EQ(*(r2.Get()), 3);
  EXPECT_EQ(*(r3.Get()), 4);

  auto derived = strai::Actor(Derived::FactoryCreate).Remote();
  auto r4 = derived.Task(&Base::Foo).Remote();
  auto r5 = derived.Task(&Base::Bar).Remote();
  EXPECT_EQ(*(r4.Get()), 10);
  EXPECT_EQ(*(r5.Get()), 20);
}

TEST(StraiApiTest, CallWithObjectRef) {
  auto rt0 = strai::Task(Return).Remote();
  auto rt1 = strai::Task(PlusOne).Remote(rt0);
  auto rt2 = strai::Task(PlusTwo).Remote(rt1, 3);
  auto rt3 = strai::Task(PlusOne).Remote(3);
  auto rt4 = strai::Task(PlusTwo).Remote(rt2, rt3);

  int return0 = *(rt0.Get());
  int return1 = *(rt1.Get());
  int return2 = *(rt2.Get());
  int return3 = *(rt3.Get());
  int return4 = *(rt4.Get());

  EXPECT_EQ(return0, 1);
  EXPECT_EQ(return1, 2);
  EXPECT_EQ(return2, 5);
  EXPECT_EQ(return3, 4);
  EXPECT_EQ(return4, 9);
}

TEST(StraiApiTest, OverloadTest) {
  auto rt0 = strai::Task(STRAI_FUNC(OverloadFunc)).Remote();
  auto rt1 = strai::Task(STRAI_FUNC(OverloadFunc, int)).Remote(rt0);
  auto rt2 = strai::Task(STRAI_FUNC(OverloadFunc, int, int)).Remote(rt1, 3);

  int return0 = *(rt0.Get());
  int return1 = *(rt1.Get());
  int return2 = *(rt2.Get());

  EXPECT_EQ(return0, 1);
  EXPECT_EQ(return1, 2);
  EXPECT_EQ(return2, 5);
}

/// We should consider the driver so is not same with the worker so, and find the error
/// reason.
TEST(StraiApiTest, NotExistFunction) {
  EXPECT_THROW(strai::Task(NotRegisteredFunc), strai::internal::StraiException);
}

TEST(StraiApiTest, ExceptionTask) {
  /// Normal task Exception.
  auto r4 = strai::Task(ExceptionFunc).Remote(2);
  EXPECT_THROW(r4.Get(), strai::internal::StraiTaskException);
}

TEST(StraiApiTest, GetClassNameByFuncNameTest) {
  using strai::internal::FunctionManager;
  EXPECT_EQ(FunctionManager::GetClassNameByFuncName("STRAI_FUNC(Counter::FactoryCreate)"),
            "Counter");
  EXPECT_EQ(FunctionManager::GetClassNameByFuncName("Counter::FactoryCreate"), "Counter");
  EXPECT_EQ(FunctionManager::GetClassNameByFuncName("FactoryCreate"), "");
  EXPECT_EQ(FunctionManager::GetClassNameByFuncName(""), "");
  EXPECT_EQ(FunctionManager::GetClassNameByFuncName("::FactoryCreate"), "");
}
