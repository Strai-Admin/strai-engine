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

#include "abstract_strai_runtime.h"

#include <strai/api.h>
#include <strai/api/strai_exception.h>
#include <strai/util/logging.h>

#include <cassert>

#include "../config_internal.h"
#include "../util/function_helper.h"
#include "local_mode_strai_runtime.h"
#include "native_strai_runtime.h"

namespace strai {

namespace internal {
msgpack::sbuffer PackError(std::string error_msg) {
  msgpack::sbuffer sbuffer;
  msgpack::packer<msgpack::sbuffer> packer(sbuffer);
  packer.pack(msgpack::type::nil_t());
  packer.pack(std::make_tuple((int)strai::rpc::ErrorType::TASK_EXECUTION_EXCEPTION,
                              std::move(error_msg)));

  return sbuffer;
}
}  // namespace internal
namespace internal {

using strai::core::CoreWorkerProcess;
using strai::core::WorkerType;

std::shared_ptr<AbstractStraiRuntime> AbstractStraiRuntime::abstract_strai_runtime_ = nullptr;

std::shared_ptr<AbstractStraiRuntime> AbstractStraiRuntime::DoInit() {
  std::shared_ptr<AbstractStraiRuntime> runtime;
  if (ConfigInternal::Instance().run_mode == RunMode::SINGLE_PROCESS) {
    runtime = std::shared_ptr<AbstractStraiRuntime>(new LocalModeStraiRuntime());
  } else {
    ProcessHelper::GetInstance().StraiStart(TaskExecutor::ExecuteTask);
    runtime = std::shared_ptr<AbstractStraiRuntime>(new NativeStraiRuntime());
    STRAI_LOG(INFO) << "Native strai runtime started.";
  }
  STRAI_CHECK(runtime);
  internal::StraiRuntimeHolder::Instance().Init(runtime);
  if (ConfigInternal::Instance().worker_type == WorkerType::WORKER) {
    // Load functions from code search path.
    FunctionHelper::GetInstance().LoadFunctionsFromPaths(
        ConfigInternal::Instance().code_search_path);
  }
  abstract_strai_runtime_ = runtime;
  return runtime;
}

std::shared_ptr<AbstractStraiRuntime> AbstractStraiRuntime::GetInstance() {
  return abstract_strai_runtime_;
}

void AbstractStraiRuntime::DoShutdown() {
  abstract_strai_runtime_ = nullptr;
  if (ConfigInternal::Instance().run_mode == RunMode::CLUSTER) {
    ProcessHelper::GetInstance().StraiStop();
  }
}

void AbstractStraiRuntime::Put(std::shared_ptr<msgpack::sbuffer> data,
                             ObjectID *object_id) {
  object_store_->Put(data, object_id);
}

void AbstractStraiRuntime::Put(std::shared_ptr<msgpack::sbuffer> data,
                             const ObjectID &object_id) {
  object_store_->Put(data, object_id);
}

std::string AbstractStraiRuntime::Put(std::shared_ptr<msgpack::sbuffer> data) {
  ObjectID object_id;
  object_store_->Put(data, &object_id);
  return object_id.Binary();
}

std::shared_ptr<msgpack::sbuffer> AbstractStraiRuntime::Get(const std::string &object_id) {
  return Get(object_id, -1);
}

inline static std::vector<ObjectID> StringIDsToObjectIDs(
    const std::vector<std::string> &ids) {
  std::vector<ObjectID> object_ids;
  for (std::string id : ids) {
    object_ids.push_back(ObjectID::FromBinary(id));
  }
  return object_ids;
}

std::vector<std::shared_ptr<msgpack::sbuffer>> AbstractStraiRuntime::Get(
    const std::vector<std::string> &ids) {
  return Get(ids, -1);
}

std::shared_ptr<msgpack::sbuffer> AbstractStraiRuntime::Get(const std::string &object_id,
                                                          const int &timeout_ms) {
  return object_store_->Get(ObjectID::FromBinary(object_id), timeout_ms);
}

std::vector<std::shared_ptr<msgpack::sbuffer>> AbstractStraiRuntime::Get(
    const std::vector<std::string> &ids, const int &timeout_ms) {
  return object_store_->Get(StringIDsToObjectIDs(ids), timeout_ms);
}

std::vector<bool> AbstractStraiRuntime::Wait(const std::vector<std::string> &ids,
                                           int num_objects,
                                           int timeout_ms) {
  return object_store_->Wait(StringIDsToObjectIDs(ids), num_objects, timeout_ms);
}

std::vector<std::unique_ptr<::strai::TaskArg>> TransformArgs(
    std::vector<strai::internal::TaskArg> &args, bool cross_lang) {
  std::vector<std::unique_ptr<::strai::TaskArg>> strai_args;
  for (auto &arg : args) {
    std::unique_ptr<::strai::TaskArg> strai_arg = nullptr;
    if (arg.buf) {
      auto &buffer = *arg.buf;
      auto memory_buffer = std::make_shared<strai::LocalMemoryBuffer>(
          reinterpret_cast<uint8_t *>(buffer.data()), buffer.size(), true);
      std::shared_ptr<Buffer> metadata = nullptr;
      if (cross_lang) {
        auto meta_str = arg.meta_str;
        metadata = std::make_shared<strai::LocalMemoryBuffer>(
            reinterpret_cast<uint8_t *>(const_cast<char *>(meta_str.data())),
            meta_str.size(),
            true);
      }
      strai_arg = absl::make_unique<strai::TaskArgByValue>(std::make_shared<strai::StraiObject>(
          memory_buffer, metadata, std::vector<rpc::ObjectReference>()));
    } else {
      STRAI_CHECK(arg.id);
      auto id = ObjectID::FromBinary(*arg.id);
      auto owner_address = strai::rpc::Address{};
      if (ConfigInternal::Instance().run_mode == RunMode::CLUSTER) {
        auto &core_worker = CoreWorkerProcess::GetCoreWorker();
        owner_address = core_worker.GetOwnerAddressOrDie(id);
      }
      strai_arg = absl::make_unique<strai::TaskArgByReference>(id,
                                                           owner_address,
                                                           /*call_site=*/"");
    }
    strai_args.push_back(std::move(strai_arg));
  }

  return strai_args;
}

InvocationSpec BuildInvocationSpec1(TaskType task_type,
                                    const RemoteFunctionHolder &remote_function_holder,
                                    std::vector<strai::internal::TaskArg> &args,
                                    const ActorID &actor) {
  InvocationSpec invocation_spec;
  invocation_spec.task_type = task_type;
  invocation_spec.remote_function_holder = remote_function_holder;
  invocation_spec.actor_id = actor;
  invocation_spec.args =
      TransformArgs(args, remote_function_holder.lang_type != LangType::CPP);
  return invocation_spec;
}

std::string AbstractStraiRuntime::Call(const RemoteFunctionHolder &remote_function_holder,
                                     std::vector<strai::internal::TaskArg> &args,
                                     const CallOptions &task_options) {
  auto invocation_spec = BuildInvocationSpec1(
      TaskType::NORMAL_TASK, remote_function_holder, args, ActorID::Nil());
  return task_submitter_->SubmitTask(invocation_spec, task_options).Binary();
}

std::string AbstractStraiRuntime::CreateActor(
    const RemoteFunctionHolder &remote_function_holder,
    std::vector<strai::internal::TaskArg> &args,
    const ActorCreationOptions &create_options) {
  auto invocation_spec = BuildInvocationSpec1(
      TaskType::ACTOR_CREATION_TASK, remote_function_holder, args, ActorID::Nil());
  return task_submitter_->CreateActor(invocation_spec, create_options).Binary();
}

std::string AbstractStraiRuntime::CallActor(
    const RemoteFunctionHolder &remote_function_holder,
    const std::string &actor,
    std::vector<strai::internal::TaskArg> &args,
    const CallOptions &call_options) {
  InvocationSpec invocation_spec{};
  if (remote_function_holder.lang_type == LangType::PYTHON) {
    const auto native_actor_handle = CoreWorkerProcess::GetCoreWorker().GetActorHandle(
        strai::ActorID::FromBinary(actor));
    auto function_descriptor = native_actor_handle->ActorCreationTaskFunctionDescriptor();
    auto typed_descriptor = function_descriptor->As<PythonFunctionDescriptor>();
    RemoteFunctionHolder func_holder = remote_function_holder;
    func_holder.module_name = typed_descriptor->ModuleName();
    func_holder.class_name = typed_descriptor->ClassName();
    invocation_spec = BuildInvocationSpec1(
        TaskType::ACTOR_TASK, func_holder, args, ActorID::FromBinary(actor));
  } else if (remote_function_holder.lang_type == LangType::JAVA) {
    const auto native_actor_handle = CoreWorkerProcess::GetCoreWorker().GetActorHandle(
        strai::ActorID::FromBinary(actor));
    auto function_descriptor = native_actor_handle->ActorCreationTaskFunctionDescriptor();
    auto typed_descriptor = function_descriptor->As<JavaFunctionDescriptor>();
    RemoteFunctionHolder func_holder = remote_function_holder;
    func_holder.class_name = typed_descriptor->ClassName();
    invocation_spec = BuildInvocationSpec1(
        TaskType::ACTOR_TASK, func_holder, args, ActorID::FromBinary(actor));
  } else {
    invocation_spec = BuildInvocationSpec1(
        TaskType::ACTOR_TASK, remote_function_holder, args, ActorID::FromBinary(actor));
  }

  return task_submitter_->SubmitActorTask(invocation_spec, call_options).Binary();
}

const TaskID &AbstractStraiRuntime::GetCurrentTaskId() {
  return GetWorkerContext().GetCurrentTaskID();
}

JobID AbstractStraiRuntime::GetCurrentJobID() {
  return GetWorkerContext().GetCurrentJobID();
}

const ActorID &AbstractStraiRuntime::GetCurrentActorID() {
  return GetWorkerContext().GetCurrentActorID();
}

void AbstractStraiRuntime::AddLocalReference(const std::string &id) {
  if (CoreWorkerProcess::IsInitialized()) {
    auto &core_worker = CoreWorkerProcess::GetCoreWorker();
    core_worker.AddLocalReference(ObjectID::FromBinary(id));
  }
}

void AbstractStraiRuntime::RemoveLocalReference(const std::string &id) {
  if (CoreWorkerProcess::IsInitialized()) {
    auto &core_worker = CoreWorkerProcess::GetCoreWorker();
    core_worker.RemoveLocalReference(ObjectID::FromBinary(id));
  }
}

std::string AbstractStraiRuntime::GetActorId(const std::string &actor_name,
                                           const std::string &strai_namespace) {
  auto actor_id = task_submitter_->GetActor(actor_name, strai_namespace);
  if (actor_id.IsNil()) {
    return "";
  }

  return actor_id.Binary();
}

void AbstractStraiRuntime::KillActor(const std::string &str_actor_id, bool no_restart) {
  auto &core_worker = CoreWorkerProcess::GetCoreWorker();
  strai::ActorID actor_id = strai::ActorID::FromBinary(str_actor_id);
  Status status = core_worker.KillActor(actor_id, true, no_restart);
  if (!status.ok()) {
    throw StraiException(status.message());
  }
}

void AbstractStraiRuntime::ExitActor() {
  auto &core_worker = CoreWorkerProcess::GetCoreWorker();
  if (ConfigInternal::Instance().worker_type != WorkerType::WORKER ||
      core_worker.GetActorId().IsNil()) {
    throw std::logic_error("This shouldn't be called on a non-actor worker.");
  }
  throw StraiIntentionalSystemExitException("SystemExit");
}

const std::unique_ptr<strai::gcs::GlobalStateAccessor>
    &AbstractStraiRuntime::GetGlobalStateAccessor() {
  return global_state_accessor_;
}

bool AbstractStraiRuntime::WasCurrentActorRestarted() {
  if (ConfigInternal::Instance().run_mode == RunMode::SINGLE_PROCESS) {
    return false;
  }

  const auto &actor_id = GetCurrentActorID();
  auto byte_ptr = global_state_accessor_->GetActorInfo(actor_id);
  if (byte_ptr == nullptr) {
    return false;
  }

  rpc::ActorTableData actor_table_data;
  bool r = actor_table_data.ParseFromString(*byte_ptr);
  if (!r) {
    throw StraiException("Received invalid protobuf data from GCS.");
  }

  return actor_table_data.num_restarts() != 0;
}

strai::PlacementGroup AbstractStraiRuntime::CreatePlacementGroup(
    const strai::PlacementGroupCreationOptions &create_options) {
  return task_submitter_->CreatePlacementGroup(create_options);
}

void AbstractStraiRuntime::RemovePlacementGroup(const std::string &group_id) {
  return task_submitter_->RemovePlacementGroup(group_id);
}

bool AbstractStraiRuntime::WaitPlacementGroupReady(const std::string &group_id,
                                                 int64_t timeout_seconds) {
  return task_submitter_->WaitPlacementGroupReady(group_id, timeout_seconds);
}

PlacementGroup AbstractStraiRuntime::GeneratePlacementGroup(const std::string &str) {
  rpc::PlacementGroupTableData pg_table_data;
  bool r = pg_table_data.ParseFromString(str);
  if (!r) {
    throw StraiException("Received invalid protobuf data from GCS.");
  }

  PlacementGroupCreationOptions options;
  options.name = pg_table_data.name();
  auto &bundles = options.bundles;
  for (auto &bundle : bundles) {
    options.bundles.emplace_back(bundle);
  }
  options.strategy = PlacementStrategy(pg_table_data.strategy());
  PlacementGroup group(pg_table_data.placement_group_id(),
                       std::move(options),
                       PlacementGroupState(pg_table_data.state()));
  return group;
}

std::vector<PlacementGroup> AbstractStraiRuntime::GetAllPlacementGroups() {
  std::vector<std::string> list = global_state_accessor_->GetAllPlacementGroupInfo();
  std::vector<PlacementGroup> groups;
  for (auto &str : list) {
    PlacementGroup group = GeneratePlacementGroup(str);
    groups.push_back(std::move(group));
  }

  return groups;
}

PlacementGroup AbstractStraiRuntime::GetPlacementGroupById(const std::string &id) {
  PlacementGroupID pg_id = PlacementGroupID::FromBinary(id);
  auto str_ptr = global_state_accessor_->GetPlacementGroupInfo(pg_id);
  if (str_ptr == nullptr) {
    return {};
  }
  PlacementGroup group = GeneratePlacementGroup(*str_ptr);
  return group;
}

PlacementGroup AbstractStraiRuntime::GetPlacementGroup(const std::string &name) {
  // TODO(WangTaoTheTonic): Add namespace support for placement group.
  auto str_ptr = global_state_accessor_->GetPlacementGroupByName(
      name, CoreWorkerProcess::GetCoreWorker().GetJobConfig().strai_namespace());
  if (str_ptr == nullptr) {
    return {};
  }
  PlacementGroup group = GeneratePlacementGroup(*str_ptr);
  return group;
}

std::string AbstractStraiRuntime::GetNamespace() {
  auto &core_worker = CoreWorkerProcess::GetCoreWorker();
  return core_worker.GetJobConfig().strai_namespace();
}

std::string AbstractStraiRuntime::SerializeActorHandle(const std::string &actor_id) {
  auto &core_worker = CoreWorkerProcess::GetCoreWorker();
  std::string output;
  ObjectID actor_handle_id;
  auto status = core_worker.SerializeActorHandle(
      ActorID::FromBinary(actor_id), &output, &actor_handle_id);
  return output;
}

std::string AbstractStraiRuntime::DeserializeAndRegisterActorHandle(
    const std::string &serialized_actor_handle) {
  auto &core_worker = CoreWorkerProcess::GetCoreWorker();
  return core_worker
      .DeserializeAndRegisterActorHandle(serialized_actor_handle,
                                         ObjectID::Nil(),
                                         /*add_local_ref=*/true)
      .Binary();
}

}  // namespace internal
}  // namespace strai
