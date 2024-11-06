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

#pragma once

#include <strai/api/strai_runtime.h>

#include <msgpack.hpp>
#include <mutex>

#include "../config_internal.h"
#include "../util/process_helper.h"
#include "./object/object_store.h"
#include "./task/task_executor.h"
#include "./task/task_submitter.h"
#include "strai/common/id.h"
#include "strai/core_worker/context.h"
#include "strai/core_worker/core_worker.h"

namespace strai {
namespace internal {

using strai::core::WorkerContext;

class StraiIntentionalSystemExitException : public StraiException {
 public:
  StraiIntentionalSystemExitException(const std::string &msg) : StraiException(msg){};
};

class AbstractStraiRuntime : public StraiRuntime {
 public:
  virtual ~AbstractStraiRuntime(){};

  void Put(std::shared_ptr<msgpack::sbuffer> data, ObjectID *object_id);

  void Put(std::shared_ptr<msgpack::sbuffer> data, const ObjectID &object_id);

  void Put(strai::rpc::ErrorType type, const ObjectID &object_id);

  std::string Put(std::shared_ptr<msgpack::sbuffer> data);

  std::shared_ptr<msgpack::sbuffer> Get(const std::string &id);

  std::vector<std::shared_ptr<msgpack::sbuffer>> Get(const std::vector<std::string> &ids);

  std::shared_ptr<msgpack::sbuffer> Get(const std::string &object_id,
                                        const int &timeout_ms);

  std::vector<std::shared_ptr<msgpack::sbuffer>> Get(const std::vector<std::string> &ids,
                                                     const int &timeout_ms);

  std::vector<bool> Wait(const std::vector<std::string> &ids,
                         int num_objects,
                         int timeout_ms);

  std::string Call(const RemoteFunctionHolder &remote_function_holder,
                   std::vector<strai::internal::TaskArg> &args,
                   const CallOptions &task_options);

  std::string CreateActor(const RemoteFunctionHolder &remote_function_holder,
                          std::vector<strai::internal::TaskArg> &args,
                          const ActorCreationOptions &create_options);

  std::string CallActor(const RemoteFunctionHolder &remote_function_holder,
                        const std::string &actor,
                        std::vector<strai::internal::TaskArg> &args,
                        const CallOptions &call_options);

  void AddLocalReference(const std::string &id);

  void RemoveLocalReference(const std::string &id);

  std::string GetActorId(const std::string &actor_name, const std::string &strai_namespace);

  void KillActor(const std::string &str_actor_id, bool no_restart);

  void ExitActor();

  strai::PlacementGroup CreatePlacementGroup(
      const strai::PlacementGroupCreationOptions &create_options);
  void RemovePlacementGroup(const std::string &group_id);
  bool WaitPlacementGroupReady(const std::string &group_id, int64_t timeout_seconds);

  const TaskID &GetCurrentTaskId();

  JobID GetCurrentJobID();

  const ActorID &GetCurrentActorID();

  virtual const WorkerContext &GetWorkerContext() = 0;

  static std::shared_ptr<AbstractStraiRuntime> GetInstance();
  static std::shared_ptr<AbstractStraiRuntime> DoInit();

  static void DoShutdown();

  const std::unique_ptr<strai::gcs::GlobalStateAccessor> &GetGlobalStateAccessor();

  bool WasCurrentActorRestarted();

  virtual std::vector<PlacementGroup> GetAllPlacementGroups();
  virtual PlacementGroup GetPlacementGroupById(const std::string &id);
  virtual PlacementGroup GetPlacementGroup(const std::string &name);

  std::string GetNamespace();
  std::string SerializeActorHandle(const std::string &actor_id);
  std::string DeserializeAndRegisterActorHandle(
      const std::string &serialized_actor_handle);

 protected:
  std::unique_ptr<TaskSubmitter> task_submitter_;
  std::unique_ptr<TaskExecutor> task_executor_;
  std::unique_ptr<ObjectStore> object_store_;
  std::unique_ptr<strai::gcs::GlobalStateAccessor> global_state_accessor_;

 private:
  static std::shared_ptr<AbstractStraiRuntime> abstract_strai_runtime_;
  void Execute(const TaskSpecification &task_spec);
  PlacementGroup GeneratePlacementGroup(const std::string &str);
};
}  // namespace internal
}  // namespace strai
