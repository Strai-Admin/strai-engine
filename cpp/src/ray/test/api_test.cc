// Copyright 2020-2021 The Strai Authors.
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

#include <filesystem>
#include <fstream>
#include <future>
#include <thread>

#include "../config_internal.h"
#include "strai/util/logging.h"

// using namespace strai;

int Return1() { return 1; }
int Plus1(int x) { return x + 1; }

int Plus(int x, int y) { return x + y; }

int Triple(int x, int y, int z) { return x + y + z; }

std::string GetVal(strai::ObjectRef<std::string> obj) { return *obj.Get(); }

int GetIntVal(strai::ObjectRef<strai::ObjectRef<int>> obj) {
  auto val = *obj.Get();
  return *val.Get();
}

std::vector<std::shared_ptr<int>> GetList(int x, std::vector<strai::ObjectRef<int>> list) {
  return strai::Get(list);
}

STRAI_REMOTE(Return1, Plus1, Plus, Triple, GetVal, GetIntVal, GetList);

std::promise<bool> g_promise;

bool BlockGet() { return g_promise.get_future().get(); }

bool GetValue(strai::ObjectRef<bool> arg) {
  auto result = strai::Wait(std::vector<strai::ObjectRef<bool>>{arg}, 1, 1000);
  EXPECT_EQ(result.ready.size(), 0);

  g_promise.set_value(true);
  bool r = *strai::Get(arg);
  EXPECT_EQ(r, true);
  return r;
}

STRAI_REMOTE(BlockGet, GetValue);

class Counter {
 public:
  int count;

  MSGPACK_DEFINE(count);

  Counter() { count = 0; }

  static Counter *FactoryCreate() {
    Counter *counter = new Counter();
    return counter;
  }

  int Plus1(int x) { return x + 1; }

  int Plus(int x, int y) { return x + y; }

  int Triple(int x, int y, int z) { return x + y + z; }

  int Add(int x) {
    count += x;
    return count;
  }

  std::string GetVal(strai::ObjectRef<std::string> obj) { return *obj.Get(); }

  int GetIntVal(strai::ObjectRef<strai::ObjectRef<int>> obj) {
    auto val = *obj.Get();
    return *val.Get();
  }

  // The dummy x is used to test a heterogeneous case: one is value arg, another is an
  // ObjectRef arg.
  std::vector<std::shared_ptr<int>> GetList(int x,
                                            std::vector<strai::ObjectRef<int>> list) {
    return strai::Get(list);
  }
};

STRAI_REMOTE(Counter::FactoryCreate,
           &Counter::Plus1,
           &Counter::Plus,
           &Counter::Triple,
           &Counter::Add,
           &Counter::GetVal,
           &Counter::GetIntVal,
           &Counter::GetList);

TEST(StraiApiTest, LogTest) {
  auto log_path = std::filesystem::current_path().string() + "/tmp/";
  strai::StraiLog::StartStraiLog("cpp_worker", strai::StraiLogLevel::DEBUG, log_path);
  std::arstrai<std::string, 3> str_arr{"debug test", "info test", "warning test"};
  STRAILOG(DEBUG) << str_arr[0];
  STRAILOG(INFO) << str_arr[1];
  STRAILOG(WARNING) << str_arr[2];
  STRAI_CHECK(true);

  for (auto &it : std::filesystem::directory_iterator(log_path)) {
    if (!std::filesystem::is_directory(it)) {
      std::ifstream in(it.path().string(), std::ios::binary);
      std::string line;
      for (int i = 0; i < 3; i++) {
        std::getline(in, line);
        EXPECT_TRUE(line.find(str_arr[i]) != std::string::npos);
      }
    }
  }

  std::filesystem::remove_all(log_path);
}

TEST(StraiApiTest, TaskOptionsCheckTest) {
  std::unordered_map<std::string, double> map;
  map.emplace("", 1);
  EXPECT_THROW(strai::internal::CheckTaskOptions(map), strai::internal::StraiException);
  map.clear();
  map.emplace("dummy", 0);
  EXPECT_THROW(strai::internal::CheckTaskOptions(map), strai::internal::StraiException);
  map.clear();
  map.emplace("dummy", 2.0);
  strai::internal::CheckTaskOptions(map);
  map.emplace("dummy1", 2.5);
  EXPECT_THROW(strai::internal::CheckTaskOptions(map), strai::internal::StraiException);
  map.clear();
  map.emplace("dummy", 0.5);
  strai::internal::CheckTaskOptions(map);
}

TEST(StraiApiTest, PutTest) {
  strai::StraiConfig config;
  config.local_mode = true;
  strai::Init(config);

  auto obj1 = strai::Put(1);
  auto i1 = obj1.Get();
  EXPECT_EQ(1, *i1);
}

TEST(StraiApiTest, StaticGetTest) {
  strai::StraiConfig config;
  config.local_mode = true;
  strai::Init(config);
  /// `Get` member function
  auto obj_ref1 = strai::Put(100);
  auto res1 = obj_ref1.Get();
  EXPECT_EQ(100, *res1);

  /// `Get` static function
  auto obj_ref2 = strai::Put(200);
  auto res2 = strai::Get(obj_ref2);
  EXPECT_EQ(200, *res2);
}

TEST(StraiApiTest, WaitTest) {
  strai::StraiConfig config;
  config.local_mode = true;
  strai::Init(config);
  auto r0 = strai::Task(Return1).Remote();
  auto r1 = strai::Task(Plus1).Remote(3);
  auto r2 = strai::Task(Plus).Remote(2, 3);
  std::vector<strai::ObjectRef<int>> objects = {r0, r1, r2};
  auto result = strai::Wait(objects, 3, 1000);
  EXPECT_EQ(result.ready.size(), 3);
  EXPECT_EQ(result.unready.size(), 0);
  std::vector<std::shared_ptr<int>> getResult = strai::Get<int>(objects);
  EXPECT_EQ(getResult.size(), 3);
  EXPECT_EQ(*getResult[0], 1);
  EXPECT_EQ(*getResult[1], 4);
  EXPECT_EQ(*getResult[2], 5);
}

TEST(StraiApiTest, ObjectRefArgsTest) {
  auto obj = strai::Put(std::string("aaa"));
  auto r = strai::Task(GetVal).Remote(obj);
  EXPECT_EQ(*r.Get(), "aaa");

  auto obj0 = strai::Put(42);
  auto obj1 = strai::Put(obj0);
  auto r1 = strai::Task(GetIntVal).Remote(obj1);
  EXPECT_EQ(*r1.Get(), 42);

  std::vector<strai::ObjectRef<int>> list{obj0, obj0};
  auto r2 = strai::Task(GetList).Remote(1, list);
  auto result2 = *r2.Get();
  EXPECT_EQ(result2.size(), 2);
  EXPECT_EQ(*result2[0], 42);
  EXPECT_EQ(*result2[1], 42);

  auto r4 = strai::Task(BlockGet).Remote();
  auto r5 = strai::Task(GetValue).Remote(r4);
  EXPECT_EQ(*r5.Get(), true);
}

TEST(StraiApiTest, CallWithValueTest) {
  auto r0 = strai::Task(Return1).Remote();
  auto r1 = strai::Task(Plus1).Remote(3);
  auto r2 = strai::Task(Plus).Remote(2, 3);
  auto r3 = strai::Task(Triple).Remote(1, 2, 3);

  int result0 = *(r0.Get());
  int result1 = *(r1.Get());
  int result2 = *(r2.Get());
  int result3 = *(r3.Get());

  EXPECT_EQ(result0, 1);
  EXPECT_EQ(result1, 4);
  EXPECT_EQ(result2, 5);
  EXPECT_EQ(result3, 6);
}

TEST(StraiApiTest, CallWithObjectTest) {
  auto rt0 = strai::Task(Return1).Remote();
  auto rt1 = strai::Task(Plus1).Remote(rt0);
  auto rt2 = strai::Task(Plus).Remote(rt1, 3);
  auto rt3 = strai::Task(Plus1).Remote(3);
  auto rt4 = strai::Task(Plus).Remote(rt2, rt3);

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

TEST(StraiApiTest, ActorTest) {
  strai::StraiConfig config;
  config.local_mode = true;
  strai::Init(config);
  auto actor = strai::Actor(Counter::FactoryCreate).Remote();
  auto obj = strai::Put(std::string("aaa"));
  auto r = actor.Task(&Counter::GetVal).Remote(obj);
  EXPECT_EQ(*r.Get(), "aaa");

  auto obj0 = strai::Put(42);
  auto obj1 = strai::Put(obj0);
  auto r1 = actor.Task(&Counter::GetIntVal).Remote(obj1);
  EXPECT_EQ(*r1.Get(), 42);

  std::vector<strai::ObjectRef<int>> list{obj0, obj0};
  auto r2 = actor.Task(&Counter::GetList).Remote(1, list);
  auto result2 = *r2.Get();
  EXPECT_EQ(result2.size(), 2);

  auto r3 = actor.Task(&Counter::GetList).Remote(obj0, list);
  auto result3 = *r3.Get();
  EXPECT_EQ(result3.size(), 2);

  auto rt1 = actor.Task(&Counter::Add).Remote(1);
  auto rt2 = actor.Task(&Counter::Add).Remote(2);
  auto rt3 = actor.Task(&Counter::Add).Remote(3);
  auto rt4 = actor.Task(&Counter::Add).Remote(rt3);
  auto rt5 = actor.Task(&Counter::Triple).Remote(1, 2, 3);

  int return1 = *(rt1.Get());
  int return2 = *(rt2.Get());
  int return3 = *(rt3.Get());
  int return4 = *(rt4.Get());
  int return5 = *(rt5.Get());

  EXPECT_EQ(return1, 1);
  EXPECT_EQ(return2, 3);
  EXPECT_EQ(return3, 6);
  EXPECT_EQ(return4, 12);
  EXPECT_EQ(return5, 6);
}

TEST(StraiApiTest, GetActorTest) {
  strai::ActorHandle<Counter> actor =
      strai::Actor(Counter::FactoryCreate).SetName("named_actor").Remote();
  auto named_actor_obj = actor.Task(&Counter::Add).Remote(1);
  EXPECT_EQ(1, *named_actor_obj.Get());

  auto named_actor_handle_optional = strai::GetActor<Counter>("named_actor");
  EXPECT_TRUE(named_actor_handle_optional);
  auto &named_actor_handle = *named_actor_handle_optional;
  auto named_actor_obj1 = named_actor_handle.Task(&Counter::Plus1).Remote(1);
  EXPECT_EQ(2, *named_actor_obj1.Get());
  EXPECT_FALSE(strai::GetActor<Counter>("not_exist_actor"));
}

TEST(StraiApiTest, CompareWithFuture) {
  // future from a packaged_task
  std::packaged_task<int(int)> task(Plus1);
  std::future<int> f1 = task.get_future();
  std::thread t(std::move(task), 1);
  int rt1 = f1.get();

  // future from an async()
  std::future<int> f2 = std::async(std::launch::async, Plus1, 1);
  int rt2 = f2.get();

  // Strai API
  strai::StraiConfig config;
  config.local_mode = true;
  strai::Init(config);
  auto f3 = strai::Task(Plus1).Remote(1);
  int rt3 = *f3.Get();

  EXPECT_EQ(rt1, 2);
  EXPECT_EQ(rt2, 2);
  EXPECT_EQ(rt3, 2);
  t.join();
}

TEST(StraiApiTest, CreateAndRemovePlacementGroup) {
  std::vector<std::unordered_map<std::string, double>> bundles{{{"CPU", 1}}};
  strai::PlacementGroupCreationOptions options1{
      "first_placement_group", bundles, strai::PlacementStrategy::PACK};
  auto first_placement_group = strai::CreatePlacementGroup(options1);
  EXPECT_TRUE(first_placement_group.Wait(10));

  strai::RemovePlacementGroup(first_placement_group.GetID());
}

TEST(StraiApiTest, DefaultActorLifetimeTest) {
  strai::StraiConfig config;
  strai::internal::ConfigInternal::Instance().Init(config, 0, nullptr);
  EXPECT_EQ(strai::rpc::JobConfig_ActorLifetime_NON_DETACHED,
            strai::internal::ConfigInternal::Instance().default_actor_lifetime);
  config.default_actor_lifetime = strai::ActorLifetime::DETACHED;
  strai::internal::ConfigInternal::Instance().Init(config, 0, nullptr);
  EXPECT_EQ(strai::rpc::JobConfig_ActorLifetime_DETACHED,
            strai::internal::ConfigInternal::Instance().default_actor_lifetime);
  std::string str = "--strai_default_actor_lifetime=NON_DETACHED";
  char exec_name[] = {' '};
  char *args[] = {exec_name, const_cast<char *>(str.c_str())};
  strai::internal::ConfigInternal::Instance().Init(config, 2, args);
  EXPECT_EQ(strai::rpc::JobConfig_ActorLifetime_NON_DETACHED,
            strai::internal::ConfigInternal::Instance().default_actor_lifetime);
  std::string str2 = "--strai_default_actor_lifetime=detached";
  char *args2[] = {exec_name, const_cast<char *>(str2.c_str())};
  strai::internal::ConfigInternal::Instance().Init(config, 2, args2);
  EXPECT_EQ(strai::rpc::JobConfig_ActorLifetime_DETACHED,
            strai::internal::ConfigInternal::Instance().default_actor_lifetime);
}