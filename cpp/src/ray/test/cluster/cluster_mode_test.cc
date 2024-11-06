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

#include "../../runtime/abstract_strai_runtime.h"
#include "../../runtime/object/native_object_store.h"
#include "../../util/process_helper.h"
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "counter.h"
#include "plus.h"

int cmd_argc = 0;
char **cmd_argv = nullptr;

ABSL_FLAG(bool, external_cluster, false, "");
ABSL_FLAG(std::string, redis_password, "12345678", "");
ABSL_FLAG(int32_t, redis_port, 6379, "");

TEST(StraiClusterModeTest, Initialized) {
  strai::Init();
  EXPECT_TRUE(strai::IsInitialized());
  strai::Shutdown();
  EXPECT_TRUE(!strai::IsInitialized());
}

TEST(StraiClusterModeTest, DefaultActorLifetimeTest) {
  strai::StraiConfig config;
  config.default_actor_lifetime = strai::ActorLifetime::DETACHED;
  strai::Init(config, cmd_argc, cmd_argv);
  strai::ActorHandle<Counter> parent_actor =
      strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).Remote();
  std::string child_actor_name = "child_actor_name";
  parent_actor.Task(&Counter::CreateChildActor).Remote(child_actor_name).Get();
  auto child_actor_optional = strai::GetActor<Counter>(child_actor_name);
  EXPECT_TRUE(child_actor_optional);
  auto child_actor = *child_actor_optional;
  EXPECT_EQ(1, *child_actor.Task(&Counter::Plus1).Remote().Get());
  parent_actor.Kill();
  sleep(4);
  EXPECT_EQ(2, *child_actor.Task(&Counter::Plus1).Remote().Get());
  strai::Shutdown();
}

struct Person {
  std::string name;
  int age;
  MSGPACK_DEFINE(name, age);
};

TEST(StraiClusterModeTest, FullTest) {
  strai::StraiConfig config;
  config.head_args = {
      "--num-cpus", "2", "--resources", "{\"resource1\":1,\"resource2\":2}"};
  if (absl::GetFlag<bool>(FLAGS_external_cluster)) {
    auto port = absl::GetFlag<int32_t>(FLAGS_redis_port);
    std::string password = absl::GetFlag<std::string>(FLAGS_redis_password);
    std::string local_ip = strai::internal::GetNodeIpAddress();
    strai::internal::ProcessHelper::GetInstance().StartStraiNode(local_ip, port, password);
    config.address = local_ip + ":" + std::to_string(port);
    config.redis_password_ = password;
  }
  strai::Init(config, cmd_argc, cmd_argv);

  /// put and get object
  auto obj = strai::Put(12345);
  auto get_result = *(strai::Get(obj));
  EXPECT_EQ(12345, get_result);

  EXPECT_EQ(12345, *(strai::Get(obj, 5)));

  auto named_obj =
      strai::Task(Return1).SetName("named_task").SetResources({{"CPU", 1.0}}).Remote();
  EXPECT_EQ(1, *named_obj.Get());

  /// common task without args
  auto task_obj = strai::Task(Return1).Remote();
  int task_result = *(strai::Get(task_obj));
  EXPECT_EQ(1, task_result);

  /// common task with args
  auto task_obj1 = strai::Task(Plus1).Remote(5);
  auto task_result1 = *(strai::Get(task_obj1));
  EXPECT_EQ(6, task_result1);

  strai::ActorHandle<Counter> actor = strai::Actor(STRAI_FUNC(Counter::FactoryCreate))
                                        .SetMaxRestarts(1)
                                        .SetName("named_actor")
                                        .Remote();
  auto initialized_obj = actor.Task(&Counter::Initialized).Remote();
  EXPECT_TRUE(*initialized_obj.Get());
  auto named_actor_obj = actor.Task(&Counter::Plus1)
                             .SetName("named_actor_task")
                             .SetResources({{"CPU", 1.0}})
                             .Remote();
  EXPECT_EQ(1, *named_actor_obj.Get());

  auto named_actor_handle_optional = strai::GetActor<Counter>("named_actor");
  EXPECT_TRUE(named_actor_handle_optional);
  auto &named_actor_handle = *named_actor_handle_optional;
  auto named_actor_obj1 = named_actor_handle.Task(&Counter::Plus1).Remote();
  EXPECT_EQ(2, *named_actor_obj1.Get());
  EXPECT_FALSE(strai::GetActor<Counter>("not_exist_actor"));

  EXPECT_FALSE(
      *named_actor_handle.Task(&Counter::CheckRestartInActorCreationTask).Remote().Get());
  EXPECT_FALSE(
      *named_actor_handle.Task(&Counter::CheckRestartInActorTask).Remote().Get());
  named_actor_handle.Kill(false);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  auto named_actor_obj2 = named_actor_handle.Task(&Counter::Plus1).Remote();
  EXPECT_EQ(1, *named_actor_obj2.Get());
  EXPECT_TRUE(
      *named_actor_handle.Task(&Counter::CheckRestartInActorCreationTask).Remote().Get());
  EXPECT_TRUE(*named_actor_handle.Task(&Counter::CheckRestartInActorTask).Remote().Get());

  named_actor_handle.Kill();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  EXPECT_THROW(named_actor_handle.Task(&Counter::Plus1).Remote().Get(),
               strai::internal::StraiActorException);

  EXPECT_FALSE(strai::GetActor<Counter>("named_actor"));

  /// actor task without args
  auto actor1 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).Remote();
  auto actor_object1 = actor1.Task(&Counter::Plus1).Remote();
  int actor_task_result1 = *(strai::Get(actor_object1));
  EXPECT_EQ(1, actor_task_result1);

  /// actor task with args
  auto actor2 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate, int)).Remote(1);
  auto actor_object2 = actor2.Task(&Counter::Add).Remote(5);
  int actor_task_result2 = *(strai::Get(actor_object2));
  EXPECT_EQ(6, actor_task_result2);

  /// actor task with args which pass by reference
  auto actor3 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate, int, int)).Remote(6, 0);
  auto actor_object3 = actor3.Task(&Counter::Add).Remote(actor_object2);
  int actor_task_result3 = *(strai::Get(actor_object3));
  EXPECT_EQ(12, actor_task_result3);

  /// general function remote call（args passed by value）
  auto r0 = strai::Task(Return1).Remote();
  auto r1 = strai::Task(Plus1).Remote(30);
  auto r2 = strai::Task(Plus).Remote(3, 22);

  std::vector<strai::ObjectRef<int>> objects = {r0, r1, r2};
  auto result = strai::Wait(objects, 3, 5000);
  EXPECT_EQ(result.ready.size(), 3);
  EXPECT_EQ(result.unready.size(), 0);

  auto result_vector = strai::Get(objects);
  int result0 = *(result_vector[0]);
  int result1 = *(result_vector[1]);
  int result2 = *(result_vector[2]);
  EXPECT_EQ(result0, 1);
  EXPECT_EQ(result1, 31);
  EXPECT_EQ(result2, 25);

  result_vector = strai::Get(objects, 5);
  EXPECT_EQ(*(result_vector[0]), 1);
  EXPECT_EQ(*(result_vector[1]), 31);
  EXPECT_EQ(*(result_vector[2]), 25);

  /// general function remote call（args passed by reference）
  auto r3 = strai::Task(Return1).Remote();
  auto r4 = strai::Task(Plus1).Remote(r3);
  auto r5 = strai::Task(Plus).Remote(r4, r3);
  auto r6 = strai::Task(Plus).Remote(r4, 10);

  int result5 = *(strai::Get(r5));
  int result4 = *(strai::Get(r4));
  int result6 = *(strai::Get(r6));
  int result3 = *(strai::Get(r3));
  EXPECT_EQ(result0, 1);
  EXPECT_EQ(result3, 1);
  EXPECT_EQ(result4, 2);
  EXPECT_EQ(result5, 3);
  EXPECT_EQ(result6, 12);

  /// create actor and actor function remote call with args passed by value
  auto actor4 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate, int)).Remote(10);
  auto r7 = actor4.Task(&Counter::Add).Remote(5);
  auto r8 = actor4.Task(&Counter::Add).Remote(1);
  auto r9 = actor4.Task(&Counter::Add).Remote(3);
  auto r10 = actor4.Task(&Counter::Add).Remote(8);

  int result7 = *(strai::Get(r7));
  int result8 = *(strai::Get(r8));
  int result9 = *(strai::Get(r9));
  int result10 = *(strai::Get(r10));
  EXPECT_EQ(result7, 15);
  EXPECT_EQ(result8, 16);
  EXPECT_EQ(result9, 19);
  EXPECT_EQ(result10, 27);

  /// create actor and task function remote call with args passed by reference
  auto actor5 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate, int, int)).Remote(r10, 0);

  auto r11 = actor5.Task(&Counter::Add).Remote(r0);
  auto r12 = actor5.Task(&Counter::Add).Remote(r11);
  auto r13 = actor5.Task(&Counter::Add).Remote(r10);
  auto r14 = actor5.Task(&Counter::Add).Remote(r13);
  auto r15 = strai::Task(Plus).Remote(r0, r11);
  auto r16 = strai::Task(Plus1).Remote(r15);

  int result12 = *(strai::Get(r12));
  int result14 = *(strai::Get(r14));
  int result11 = *(strai::Get(r11));
  int result13 = *(strai::Get(r13));
  int result16 = *(strai::Get(r16));
  int result15 = *(strai::Get(r15));

  EXPECT_EQ(result11, 28);
  EXPECT_EQ(result12, 56);
  EXPECT_EQ(result13, 83);
  EXPECT_EQ(result14, 166);
  EXPECT_EQ(result15, 29);
  EXPECT_EQ(result16, 30);

  /// Test Put, Get & Remote for large objects
  std::arstrai<int, 100000> arr;
  auto r17 = strai::Put(arr);
  auto r18 = strai::Task(ReturnLargeArstrai).Remote(r17);
  EXPECT_EQ(arr, *(strai::Get(r17)));
  EXPECT_EQ(arr, *(strai::Get(r18)));

  uint64_t pid = *actor1.Task(&Counter::GetPid).Remote().Get(5);
  EXPECT_TRUE(Counter::IsProcessAlive(pid));

  auto actor_object4 = actor1.Task(&Counter::Exit).Remote();
  std::this_thread::sleep_for(std::chrono::seconds(2));
  EXPECT_THROW(actor_object4.Get(), strai::internal::StraiActorException);
  EXPECT_FALSE(Counter::IsProcessAlive(pid));
}

TEST(StraiClusterModeTest, ActorHandleTest) {
  auto actor1 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).Remote();
  auto obj1 = actor1.Task(&Counter::Plus1).Remote();
  EXPECT_EQ(1, *obj1.Get());
  // Test `ActorHandle` type object as parameter.
  auto actor2 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).Remote();
  auto obj2 = actor2.Task(&Counter::Plus1ForActor).Remote(actor1);
  EXPECT_EQ(2, *obj2.Get());
  // Test `ActorHandle` type object as return value.
  std::string child_actor_name = "child_actor_name";
  auto child_actor =
      actor1.Task(&Counter::CreateChildActor).Remote(child_actor_name).Get();
  EXPECT_EQ(1, *child_actor->Task(&Counter::Plus1).Remote().Get());

  auto named_actor_handle_optional = strai::GetActor<Counter>(child_actor_name);
  EXPECT_TRUE(named_actor_handle_optional);
  auto &named_actor_handle = *named_actor_handle_optional;
  auto named_actor_obj1 = named_actor_handle.Task(&Counter::Plus1).Remote();
  EXPECT_EQ(2, *named_actor_obj1.Get());
}

TEST(StraiClusterModeTest, PythonInvocationTest) {
  strai::ActorHandleXlang py_actor_handle =
      strai::Actor(strai::PyActorClass{"test_cross_language_invocation", "Counter"})
          .Remote(1);
  EXPECT_TRUE(!py_actor_handle.ID().empty());

  auto py_actor_ret =
      py_actor_handle.Task(strai::PyActorMethod<std::string>{"increase"}).Remote(1);
  EXPECT_EQ("2", *py_actor_ret.Get());

  auto py_obj =
      strai::Task(strai::PyFunction<int>{"test_cross_language_invocation", "py_return_val"})
          .Remote();
  EXPECT_EQ(42, *py_obj.Get());

  auto py_obj1 =
      strai::Task(strai::PyFunction<int>{"test_cross_language_invocation", "py_return_input"})
          .Remote(42);
  EXPECT_EQ(42, *py_obj1.Get());

  auto py_obj2 = strai::Task(strai::PyFunction<std::string>{"test_cross_language_invocation",
                                                        "py_return_input"})
                     .Remote("hello");
  EXPECT_EQ("hello", *py_obj2.Get());

  Person p{"tom", 20};
  auto py_obj3 = strai::Task(strai::PyFunction<Person>{"test_cross_language_invocation",
                                                   "py_return_input"})
                     .Remote(p);
  auto py_result = *py_obj3.Get();
  EXPECT_EQ(p.age, py_result.age);
  EXPECT_EQ(p.name, py_result.name);
}

TEST(StraiClusterModeTest, MaxConcurrentTest) {
  auto actor1 =
      strai::Actor(ActorConcurrentCall::FactoryCreate).SetMaxConcurrency(3).Remote();
  auto object1 = actor1.Task(&ActorConcurrentCall::CountDown).Remote();
  auto object2 = actor1.Task(&ActorConcurrentCall::CountDown).Remote();
  auto object3 = actor1.Task(&ActorConcurrentCall::CountDown).Remote();

  EXPECT_EQ(*object1.Get(), "ok");
  EXPECT_EQ(*object2.Get(), "ok");
  EXPECT_EQ(*object3.Get(), "ok");

  auto actor2 =
      strai::Actor(ActorConcurrentCall::FactoryCreate).SetMaxConcurrency(2).Remote();
  auto object2_1 = actor2.Task(&ActorConcurrentCall::CountDown).Remote();
  auto object2_2 = actor2.Task(&ActorConcurrentCall::CountDown).Remote();
  auto object2_3 = actor2.Task(&ActorConcurrentCall::CountDown).Remote();

  EXPECT_THROW(object2_1.Get(2), strai::internal::StraiTimeoutException);
  EXPECT_THROW(object2_2.Get(2), strai::internal::StraiTimeoutException);
  EXPECT_THROW(object2_3.Get(2), strai::internal::StraiTimeoutException);
}

TEST(StraiClusterModeTest, ResourcesManagementTest) {
  auto actor1 =
      strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).SetResources({{"CPU", 1.0}}).Remote();
  auto r1 = actor1.Task(&Counter::Plus1).Remote();
  EXPECT_EQ(*r1.Get(), 1);

  auto actor2 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate))
                    .SetResources({{"CPU", 10000.0}})
                    .Remote();
  auto r2 = actor2.Task(&Counter::Plus1).Remote();
  std::vector<strai::ObjectRef<int>> objects{r2};
  auto result = strai::Wait(objects, 1, 5000);
  EXPECT_EQ(result.ready.size(), 0);
  EXPECT_EQ(result.unready.size(), 1);

  auto r3 = strai::Task(Return1).SetResource("CPU", 1.0).Remote();
  EXPECT_EQ(*r3.Get(), 1);

  auto r4 = strai::Task(Return1).SetResource("CPU", 100.0).Remote();
  std::vector<strai::ObjectRef<int>> objects1{r4};
  auto result2 = strai::Wait(objects1, 1, 5000);
  EXPECT_EQ(result2.ready.size(), 0);
  EXPECT_EQ(result2.unready.size(), 1);
}

TEST(StraiClusterModeTest, ExceptionTest) {
  EXPECT_THROW(strai::Task(ThrowTask).Remote().Get(), strai::internal::StraiTaskException);
  try {
    strai::Task(ThrowTask).Remote().Get();
  } catch (strai::internal::StraiTaskException &e) {
    EXPECT_TRUE(std::string(e.what()).find("std::logic_error") != std::string::npos);
  }

  auto actor1 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate, int)).Remote(1);
  auto object1 = actor1.Task(&Counter::ExceptionFunc).Remote();
  EXPECT_THROW(object1.Get(), strai::internal::StraiTaskException);

  auto actor2 = strai::Actor(Counter::FactoryCreateException).Remote();
  auto object2 = actor2.Task(&Counter::Plus1).Remote();
  EXPECT_THROW(object2.Get(), strai::internal::StraiActorException);
}

TEST(StraiClusterModeTest, GetAllNodeInfoTest) {
  const auto &gcs_client =
      strai::internal::AbstractStraiRuntime::GetInstance()->GetGlobalStateAccessor();
  auto all_node_info = gcs_client->GetAllNodeInfo();
  EXPECT_EQ(all_node_info.size(), 1);

  strai::rpc::GcsNodeInfo node_info;
  node_info.ParseFromString(all_node_info[0]);
  EXPECT_EQ(node_info.state(),
            strai::rpc::GcsNodeInfo_GcsNodeState::GcsNodeInfo_GcsNodeState_ALIVE);
}

bool CheckRefCount(
    std::unordered_map<strai::ObjectID, std::pair<size_t, size_t>> expected) {
  auto object_store = std::make_unique<strai::internal::NativeObjectStore>();
  auto map = object_store->GetAllReferenceCounts();
  return expected == map;
}

TEST(StraiClusterModeTest, LocalRefrenceTest) {
  auto r1 = std::make_unique<strai::ObjectRef<int>>(strai::Task(Return1).Remote());
  auto object_id = strai::ObjectID::FromBinary(r1->ID());
  EXPECT_TRUE(CheckRefCount({{object_id, std::make_pair(1, 0)}}));
  auto r2 = std::make_unique<strai::ObjectRef<int>>(*r1);
  EXPECT_TRUE(CheckRefCount({{object_id, std::make_pair(2, 0)}}));
  r1.reset();
  EXPECT_TRUE(CheckRefCount({{object_id, std::make_pair(1, 0)}}));
  r2.reset();
  EXPECT_TRUE(CheckRefCount({}));
}

TEST(StraiClusterModeTest, DependencyRefrenceTest) {
  {
    auto r1 = strai::Task(Return1).Remote();
    auto object_id = strai::ObjectID::FromBinary(r1.ID());
    EXPECT_TRUE(CheckRefCount({{object_id, std::make_pair(1, 0)}}));

    auto r2 = strai::Task(Plus1).Remote(r1);
    EXPECT_TRUE(
        CheckRefCount({{object_id, std::make_pair(1, 1)},
                       {strai::ObjectID::FromBinary(r2.ID()), std::make_pair(1, 0)}}));
    r2.Get();
    EXPECT_TRUE(
        CheckRefCount({{object_id, std::make_pair(1, 0)},
                       {strai::ObjectID::FromBinary(r2.ID()), std::make_pair(1, 0)}}));
  }
  EXPECT_TRUE(CheckRefCount({}));
}

TEST(StraiClusterModeTest, GetActorTest) {
  strai::ActorHandle<Counter> actor = strai::Actor(STRAI_FUNC(Counter::FactoryCreate))
                                        .SetMaxRestarts(1)
                                        .SetName("named_actor")
                                        .Remote();
  auto named_actor_obj = actor.Task(&Counter::Plus1).Remote();
  EXPECT_EQ(1, *named_actor_obj.Get());

  auto named_actor_handle_optional = strai::GetActor<Counter>("named_actor");
  EXPECT_TRUE(named_actor_handle_optional);
  auto &named_actor_handle = *named_actor_handle_optional;
  auto named_actor_obj1 = named_actor_handle.Task(&Counter::Plus1).Remote();
  EXPECT_EQ(2, *named_actor_obj1.Get());
  EXPECT_FALSE(strai::GetActor<Counter>("not_exist_actor"));
}

strai::PlacementGroup CreateSimplePlacementGroup(const std::string &name) {
  std::vector<std::unordered_map<std::string, double>> bundles{{{"CPU", 1}}};

  strai::PlacementGroupCreationOptions options{name, bundles, strai::PlacementStrategy::PACK};
  return strai::CreatePlacementGroup(options);
}

TEST(StraiClusterModeTest, CreateAndRemovePlacementGroup) {
  auto first_placement_group = CreateSimplePlacementGroup("first_placement_group");
  EXPECT_TRUE(first_placement_group.Wait(10));
  EXPECT_THROW(CreateSimplePlacementGroup("first_placement_group"),
               strai::internal::StraiException);

  auto groups = strai::GetAllPlacementGroups();
  EXPECT_EQ(groups.size(), 1);

  auto placement_group = strai::GetPlacementGroupById(first_placement_group.GetID());
  EXPECT_EQ(placement_group.GetID(), first_placement_group.GetID());

  auto placement_group1 = strai::GetPlacementGroup("first_placement_group");
  EXPECT_EQ(placement_group1.GetID(), first_placement_group.GetID());

  strai::RemovePlacementGroup(first_placement_group.GetID());
  auto deleted_group = strai::GetPlacementGroupById(first_placement_group.GetID());
  EXPECT_EQ(deleted_group.GetState(), strai::PlacementGroupState::REMOVED);

  auto not_exist_group = strai::GetPlacementGroup("not_exist_placement_group");
  EXPECT_TRUE(not_exist_group.GetID().empty());

  strai::RemovePlacementGroup(first_placement_group.GetID());
}

TEST(StraiClusterModeTest, CreatePlacementGroupExceedsClusterResource) {
  std::vector<std::unordered_map<std::string, double>> bundles{{{"CPU", 10000}}};

  strai::PlacementGroupCreationOptions options{
      "first_placement_group", bundles, strai::PlacementStrategy::PACK};
  auto first_placement_group = strai::CreatePlacementGroup(options);
  EXPECT_FALSE(first_placement_group.Wait(3));
  strai::RemovePlacementGroup(first_placement_group.GetID());
  auto deleted_group = strai::GetPlacementGroupById(first_placement_group.GetID());
  EXPECT_EQ(deleted_group.GetState(), strai::PlacementGroupState::REMOVED);

  auto not_exist_group = strai::GetPlacementGroup("not_exist_placement_group");
  EXPECT_TRUE(not_exist_group.GetID().empty());
}

TEST(StraiClusterModeTest, CreateActorWithPlacementGroup) {
  auto placement_group = CreateSimplePlacementGroup("first_placement_group");
  EXPECT_TRUE(placement_group.Wait(10));

  auto actor1 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate))
                    .SetResources({{"CPU", 1.0}})
                    .SetPlacementGroup(placement_group, 0)
                    .Remote();
  auto r1 = actor1.Task(&Counter::Plus1).Remote();
  std::vector<strai::ObjectRef<int>> objects{r1};
  auto result = strai::Wait(objects, 1, 5000);
  EXPECT_EQ(result.ready.size(), 1);
  EXPECT_EQ(result.unready.size(), 0);
  auto result_vector = strai::Get(objects);
  EXPECT_EQ(*(result_vector[0]), 1);

  // Exceeds the resources of PlacementGroup.
  auto actor2 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate))
                    .SetResources({{"CPU", 2.0}})
                    .SetPlacementGroup(placement_group, 0)
                    .Remote();
  auto r2 = actor2.Task(&Counter::Plus1).Remote();
  std::vector<strai::ObjectRef<int>> objects2{r2};
  auto result2 = strai::Wait(objects2, 1, 5000);
  EXPECT_EQ(result2.ready.size(), 0);
  EXPECT_EQ(result2.unready.size(), 1);
  strai::RemovePlacementGroup(placement_group.GetID());
}

TEST(StraiClusterModeTest, TaskWithPlacementGroup) {
  auto placement_group = CreateSimplePlacementGroup("first_placement_group");
  EXPECT_TRUE(placement_group.Wait(10));

  auto r = strai::Task(Return1)
               .SetResources({{"CPU", 1.0}})
               .SetPlacementGroup(placement_group, 0)
               .Remote();
  EXPECT_EQ(*r.Get(), 1);
  strai::RemovePlacementGroup(placement_group.GetID());
}

TEST(StraiClusterModeTest, NamespaceTest) {
  if (strai::IsInitialized()) {
    strai::Shutdown();
  }
  strai::Init();
  // Create a named actor in namespace `isolated_ns`.
  std::string actor_name_in_isolated_ns = "named_actor_in_isolated_ns";
  std::string isolated_ns_name = "isolated_ns";
  strai::ActorHandle<Counter> actor =
      strai::Actor(STRAI_FUNC(Counter::FactoryCreate))
          .SetName(actor_name_in_isolated_ns, isolated_ns_name)
          .Remote();
  auto initialized_obj = actor.Task(&Counter::Initialized).Remote();
  EXPECT_TRUE(*initialized_obj.Get());
  // It is invisible to job default namespace.
  auto actor_optional = strai::GetActor<Counter>(actor_name_in_isolated_ns);
  EXPECT_TRUE(!actor_optional);
  // It is visible to the namespace it belongs.
  actor_optional = strai::GetActor<Counter>(actor_name_in_isolated_ns, isolated_ns_name);
  EXPECT_TRUE(actor_optional);
  // It is invisible to any other namespaces.
  actor_optional = strai::GetActor<Counter>(actor_name_in_isolated_ns, "other_ns");
  EXPECT_TRUE(!actor_optional);

  // Create a named actor in job default namespace.
  std::string actor_name_in_default_ns = "actor_name_in_default_ns";
  auto actor1 = strai::Actor(STRAI_FUNC(Counter::FactoryCreate))
                    .SetName(actor_name_in_default_ns)
                    .Remote();
  auto initialized_obj1 = actor1.Task(&Counter::Initialized).Remote();
  EXPECT_TRUE(*initialized_obj1.Get());
  // It is visible to job default namespace.
  actor_optional = strai::GetActor<Counter>(actor_name_in_default_ns);
  EXPECT_TRUE(actor_optional);
  // It is invisible to any other namespaces.
  actor_optional = strai::GetActor<Counter>(actor_name_in_default_ns, isolated_ns_name);
  EXPECT_TRUE(!actor_optional);
  strai::Shutdown();
}

TEST(StraiClusterModeTest, GetNamespaceApiTest) {
  std::string ns = "test_get_current_namespace";
  strai::StraiConfig config;
  config.strai_namespace = ns;
  if (strai::IsInitialized()) {
    strai::Shutdown();
  }
  strai::Init(config, cmd_argc, cmd_argv);
  // Get namespace in driver.
  EXPECT_EQ(strai::GetNamespace(), ns);
  // Get namespace in task.
  auto task_ns = strai::Task(GetNamespaceInTask).Remote();
  EXPECT_EQ(*task_ns.Get(), ns);
  // Get namespace in actor.
  auto actor_handle = strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).Remote();
  auto actor_ns = actor_handle.Task(&Counter::GetNamespaceInActor).Remote();
  EXPECT_EQ(*actor_ns.Get(), ns);
  strai::Shutdown();
}

class Pip {
 public:
  std::vector<std::string> packages;
  bool pip_check = false;
  Pip() = default;
  Pip(const std::vector<std::string> &packages, bool pip_check)
      : packages(packages), pip_check(pip_check) {}
};

void to_json(nlohmann::json &j, const Pip &pip) {
  j = nlohmann::json{{"packages", pip.packages}, {"pip_check", pip.pip_check}};
};

void from_json(const nlohmann::json &j, Pip &pip) {
  j.at("packages").get_to(pip.packages);
  j.at("pip_check").get_to(pip.pip_check);
};

TEST(StraiClusterModeTest, RuntimeEnvApiTest) {
  strai::RuntimeEnv runtime_env;
  // Set pip
  std::vector<std::string> packages = {"requests"};
  Pip pip(packages, true);
  runtime_env.Set("pip", pip);
  // Set working_dir
  std::string working_dir = "https://path/to/working_dir.zip";
  runtime_env.Set("working_dir", working_dir);

  // Serialize
  auto serialized_runtime_env = runtime_env.Serialize();

  // Deserialize
  auto runtime_env_2 = strai::RuntimeEnv::Deserialize(serialized_runtime_env);
  auto pip2 = runtime_env_2.Get<Pip>("pip");
  EXPECT_EQ(pip2.packages, pip.packages);
  EXPECT_EQ(pip2.pip_check, pip.pip_check);
  auto working_dir2 = runtime_env_2.Get<std::string>("working_dir");
  EXPECT_EQ(working_dir2, working_dir);

  // Construct runtime env with raw json string
  strai::RuntimeEnv runtime_env_3;
  std::string pip_raw_json_string =
      R"({"packages":["requests","tensorflow"],"pip_check":false})";
  runtime_env_3.SetJsonStr("pip", pip_raw_json_string);
  auto get_json_result = runtime_env_3.GetJsonStr("pip");
  EXPECT_EQ(get_json_result, pip_raw_json_string);
}

TEST(StraiClusterModeTest, RuntimeEnvApiExceptionTest) {
  strai::RuntimeEnv runtime_env;
  EXPECT_THROW(runtime_env.Get<std::string>("working_dir"),
               strai::internal::StraiRuntimeEnvException);
  runtime_env.Set("working_dir", "https://path/to/working_dir.zip");
  EXPECT_THROW(runtime_env.Get<Pip>("working_dir"),
               strai::internal::StraiRuntimeEnvException);
  EXPECT_THROW(runtime_env.SetJsonStr("pip", "{123"),
               strai::internal::StraiRuntimeEnvException);
  EXPECT_THROW(runtime_env.GetJsonStr("pip"), strai::internal::StraiRuntimeEnvException);
  EXPECT_EQ(runtime_env.Empty(), false);
  EXPECT_EQ(runtime_env.Remove("working_dir"), true);
  // Do nothing when removing a non-existent key.
  EXPECT_EQ(runtime_env.Remove("pip"), false);
  EXPECT_EQ(runtime_env.Empty(), true);
}

TEST(StraiClusterModeTest, RuntimeEnvTaskLevelEnvVarsTest) {
  strai::StraiConfig config;
  strai::Init(config, cmd_argc, cmd_argv);
  auto r0 = strai::Task(GetEnvVar).Remote("KEY1");
  auto get_result0 = *(strai::Get(r0));
  EXPECT_EQ("", get_result0);

  auto actor_handle = strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).Remote();
  auto r1 = actor_handle.Task(&Counter::GetEnvVar).Remote("KEY1");
  auto get_result1 = *(strai::Get(r1));
  EXPECT_EQ("", get_result1);

  strai::RuntimeEnv runtime_env;
  std::map<std::string, std::string> env_vars{{"KEY1", "value1"}};
  runtime_env.Set("env_vars", env_vars);
  auto r2 = strai::Task(GetEnvVar).SetRuntimeEnv(runtime_env).Remote("KEY1");
  auto get_result2 = *(strai::Get(r2));
  EXPECT_EQ("value1", get_result2);

  strai::RuntimeEnv runtime_env2;
  std::map<std::string, std::string> env_vars2{{"KEY1", "value2"}};
  runtime_env2.Set("env_vars", env_vars2);
  auto actor_handle2 =
      strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).SetRuntimeEnv(runtime_env2).Remote();
  auto r3 = actor_handle2.Task(&Counter::GetEnvVar).Remote("KEY1");
  auto get_result3 = *(strai::Get(r3));
  EXPECT_EQ("value2", get_result3);

  strai::Shutdown();
}

TEST(StraiClusterModeTest, RuntimeEnvJobLevelEnvVarsTest) {
  strai::StraiConfig config;
  strai::RuntimeEnv runtime_env;
  std::map<std::string, std::string> env_vars{{"KEY1", "value1"}};
  runtime_env.Set("env_vars", env_vars);
  config.runtime_env = runtime_env;
  strai::Init(config, cmd_argc, cmd_argv);
  auto r0 = strai::Task(GetEnvVar).Remote("KEY1");
  auto get_result0 = *(strai::Get(r0));
  EXPECT_EQ("value1", get_result0);

  auto actor_handle = strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).Remote();
  auto r1 = actor_handle.Task(&Counter::GetEnvVar).Remote("KEY1");
  auto get_result1 = *(strai::Get(r1));
  EXPECT_EQ("value1", get_result1);

  strai::Shutdown();
}

TEST(StraiClusterModeTest, UnsupportObjectRefTest) {
  strai::StraiConfig config;
  strai::Init(config, cmd_argc, cmd_argv);
  strai::ActorHandle<Counter> actor = strai::Actor(STRAI_FUNC(Counter::FactoryCreate)).Remote();
  auto int_ref = strai::Put(1);
  EXPECT_THROW(actor.Task(&Counter::GetIntByObjectRef).Remote(int_ref),
               std::invalid_argument);

  strai::Shutdown();
}

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  cmd_argc = argc;
  cmd_argv = argv;
  ::testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();

  strai::Shutdown();

  if (absl::GetFlag<bool>(FLAGS_external_cluster)) {
    strai::internal::ProcessHelper::GetInstance().StopStraiNode();
  }

  return ret;
}
