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

#include <strai/api/actor_task_caller.h>
#include <strai/api/function_manager.h>
#include <strai/api/strai_runtime_holder.h>

namespace strai {

/// A handle to an actor which can be used to invoke a remote actor method, with the
/// `Call` method.
/// \param ActorType The type of the concrete actor class.
/// Note, the `Call` method is defined in actor_call.generated.h.
template <typename ActorType, bool IsXlang = false>
class ActorHandle {
 public:
  ActorHandle() = default;

  ActorHandle(const std::string &id) { id_ = id; }

  // Used to identify its type.
  static bool IsActorHandle() { return true; }

  /// Get a untyped ID of the actor
  const std::string &ID() const { return id_; }

  /// Include the `Call` methods for calling remote functions.
  template <typename F>
  strai::internal::ActorTaskCaller<F> Task(F actor_func) {
    static_assert(!IsXlang && !strai::internal::is_python_v<F>,
                  "Actor method is not a member function of actor class.");
    static_assert(std::is_member_function_pointer_v<F>,
                  "Actor method is not a member function of actor class.");
    using Self = boost::callable_traits::class_of_t<F>;
    static_assert(
        std::is_same<ActorType, Self>::value || std::is_base_of<Self, ActorType>::value,
        "Class types must be same.");
    auto func_name = internal::FunctionManager::Instance().GetFunctionName(actor_func);
    strai::internal::RemoteFunctionHolder remote_func_holder(func_name);
    return strai::internal::ActorTaskCaller<F>(
        internal::GetStraiRuntime().get(), id_, std::move(remote_func_holder));
  }

  template <typename R>
  strai::internal::ActorTaskCaller<PyActorMethod<R>> Task(PyActorMethod<R> func) {
    static_assert(IsXlang, "Actor function type does not match actor class");
    strai::internal::RemoteFunctionHolder remote_func_holder(
        "", func.function_name, "", strai::internal::LangType::PYTHON);
    return {strai::internal::GetStraiRuntime().get(), id_, std::move(remote_func_holder)};
  }

  template <typename R>
  strai::internal::ActorTaskCaller<JavaActorMethod<R>> Task(JavaActorMethod<R> func) {
    static_assert(IsXlang, "Actor function type does not match actor class");
    strai::internal::RemoteFunctionHolder remote_func_holder(
        "", func.function_name, "", strai::internal::LangType::JAVA);
    return {strai::internal::GetStraiRuntime().get(), id_, std::move(remote_func_holder)};
  }

  void Kill() { Kill(true); }
  void Kill(bool no_restart) {
    strai::internal::GetStraiRuntime()->KillActor(id_, no_restart);
  }

  static ActorHandle FromBytes(const std::string &serialized_actor_handle) {
    std::string id = strai::internal::GetStraiRuntime()->DeserializeAndRegisterActorHandle(
        serialized_actor_handle);
    return ActorHandle(id);
  }

  /// Make ActorHandle serializable
  MSGPACK_DEFINE(id_);

 private:
  std::string id_;
};

typedef ActorHandle<void, true> ActorHandleXlang;
}  // namespace strai
