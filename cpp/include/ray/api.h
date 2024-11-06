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

#include <strai/api/actor_creator.h>
#include <strai/api/actor_handle.h>
#include <strai/api/actor_task_caller.h>
#include <strai/api/function_manager.h>
#include <strai/api/logging.h>
#include <strai/api/object_ref.h>
#include <strai/api/strai_config.h>
#include <strai/api/strai_remote.h>
#include <strai/api/strai_runtime.h>
#include <strai/api/strai_runtime_holder.h>
#include <strai/api/runtime_env.h>
#include <strai/api/task_caller.h>
#include <strai/api/wait_result.h>

#include <boost/callable_traits.hpp>
#include <memory>
#include <msgpack.hpp>
#include <mutex>

namespace strai {

/// Initialize Strai runtime with config.
void Init(strai::StraiConfig &config);

/// Initialize Strai runtime with config and command-line arguments.
/// If a parameter is explicitly set in command-line arguments, the parameter value will
/// be overwritten.
void Init(strai::StraiConfig &config, int argc, char **argv);

/// Initialize Strai runtime with default config.
void Init();

/// Check if strai::Init has been called yet.
bool IsInitialized();

/// Shutdown Strai runtime.
void Shutdown();

/// Store an object in the object store.
///
/// \param[in] obj The object which should be stored.
/// \return ObjectRef A reference to the object in the object store.
template <typename T>
strai::ObjectRef<T> Put(const T &obj);

/// Get a single object from the object store.
/// This method will be blocked until the object is ready.
///
/// \param[in] object The object reference which should be returned.
/// \return shared pointer of the result.
template <typename T>
std::shared_ptr<T> Get(const strai::ObjectRef<T> &object);

/// Get a list of objects from the object store.
/// This method will be blocked until all the objects are ready.
///
/// \param[in] objects The object arstrai which should be got.
/// \return shared pointer arstrai of the result.
template <typename T>
std::vector<std::shared_ptr<T>> Get(const std::vector<strai::ObjectRef<T>> &objects);

/// Get a single object from the object store.
/// This method will be blocked until the object is ready.
///
/// \param[in] object The object reference which should be returned.
/// \param[in] timeout_ms The maximum amount of time in miliseconds to wait before
/// returning.
/// \return shared pointer of the result.
template <typename T>
std::shared_ptr<T> Get(const strai::ObjectRef<T> &object, const int &timeout_ms);

/// Get a list of objects from the object store.
/// This method will be blocked until all the objects are ready.
///
/// \param[in] objects The object arstrai which should be got.
/// \param[in] timeout_ms The maximum amount of time in miliseconds to wait before
/// returning.
/// \return shared pointer arstrai of the result.
template <typename T>
std::vector<std::shared_ptr<T>> Get(const std::vector<strai::ObjectRef<T>> &objects,
                                    const int &timeout_ms);

/// Wait for a list of objects to be locally available,
/// until specified number of objects are ready, or specified timeout has passed.
///
/// \param[in] objects The object arstrai which should be waited.
/// \param[in] num_objects The minimum number of objects to wait.
/// \param[in] timeout_ms The maximum wait time in milliseconds.
/// \return Two arstrais, one containing locally available objects, one containing the
/// rest.
template <typename T>
WaitResult<T> Wait(const std::vector<strai::ObjectRef<T>> &objects,
                   int num_objects,
                   int timeout_ms);

/// Create a `TaskCaller` for calling remote function.
/// It is used for normal task, such as strai::Task(Plus1).Remote(1),
/// strai::Task(Plus).Remote(1, 2).
/// \param[in] func The function to be remote executed.
/// \return TaskCaller.
template <typename F>
strai::internal::TaskCaller<F> Task(F func);

template <typename R>
strai::internal::TaskCaller<PyFunction<R>> Task(PyFunction<R> func);

template <typename R>
strai::internal::TaskCaller<JavaFunction<R>> Task(JavaFunction<R> func);

/// Generic version of creating an actor
/// It is used for creating an actor, such as: ActorCreator<Counter> creator =
/// strai::Actor(Counter::FactoryCreate<int>).Remote(1);
template <typename F>
strai::internal::ActorCreator<F> Actor(F create_func);

strai::internal::ActorCreator<PyActorClass> Actor(PyActorClass func);

strai::internal::ActorCreator<JavaActorClass> Actor(JavaActorClass func);

/// Get a handle to a named actor in current namespace.
/// The actor must have been created with name specified.
///
/// \param[in] actor_name The name of the named actor.
/// \return An ActorHandle to the actor if the actor of specified name exists or an
/// empty optional object.
template <typename T>
boost::optional<ActorHandle<T>> GetActor(const std::string &actor_name);

/// Get a handle to a named actor in the given namespace.
/// The actor must have been created with name specified.
///
/// \param[in] actor_name The name of the named actor.
/// \param[in] namespace The namespace of the actor.
/// \return An ActorHandle to the actor if the actor of specified name exists in
/// specifiled namespace or an empty optional object.
template <typename T>
boost::optional<ActorHandle<T>> GetActor(const std::string &actor_name,
                                         const std::string &strai_namespace);

/// Intentionally exit the current actor.
/// It is used to disconnect an actor and exit the worker.
/// \Throws StraiException if the current process is a driver or the current worker is not
/// an actor.
void ExitActor();

template <typename T>
std::vector<std::shared_ptr<T>> Get(const std::vector<std::string> &ids);

template <typename T>
std::vector<std::shared_ptr<T>> Get(const std::vector<std::string> &ids,
                                    const int &timeout_ms);

/// Create a placement group on remote nodes.
///
/// \param[in] create_options Creation options of the placement group.
/// \return A PlacementGroup to the created placement group.
PlacementGroup CreatePlacementGroup(
    const strai::PlacementGroupCreationOptions &create_options);

/// Remove a placement group by id.
///
/// \param[in] placement_group_id Id of the placement group.
void RemovePlacementGroup(const std::string &placement_group_id);

std::vector<PlacementGroup> GetAllPlacementGroups();

/// Get a placement group by id.
PlacementGroup GetPlacementGroupById(const std::string &id);

/// Get a placement group by name.
PlacementGroup GetPlacementGroup(const std::string &name);

/// Returns true if the current actor was restarted, otherwise false.
bool WasCurrentActorRestarted();

/// Get the namespace of this job.
std::string GetNamespace();

// --------- inline implementation ------------

template <typename T>
inline std::vector<std::string> ObjectRefsToObjectIDs(
    const std::vector<strai::ObjectRef<T>> &object_refs) {
  std::vector<std::string> object_ids;
  for (auto it = object_refs.begin(); it != object_refs.end(); it++) {
    object_ids.push_back(it->ID());
  }
  return object_ids;
}

template <typename T>
inline strai::ObjectRef<T> Put(const T &obj) {
  auto buffer =
      std::make_shared<msgpack::sbuffer>(strai::internal::Serializer::Serialize(obj));
  auto id = strai::internal::GetStraiRuntime()->Put(buffer);
  auto ref = ObjectRef<T>(id);
  // The core worker will add an initial ref to the put ID to
  // keep it in scope. Now that we've created the frontend
  // ObjectRef, remove this initial ref.
  strai::internal::GetStraiRuntime()->RemoveLocalReference(id);
  return ref;
}

template <typename T>
inline std::shared_ptr<T> Get(const strai::ObjectRef<T> &object, const int &timeout_ms) {
  return GetFromRuntime(object, timeout_ms);
}

template <typename T>
inline std::vector<std::shared_ptr<T>> Get(const std::vector<std::string> &ids,
                                           const int &timeout_ms) {
  auto result = strai::internal::GetStraiRuntime()->Get(ids, timeout_ms);
  std::vector<std::shared_ptr<T>> return_objects;
  return_objects.reserve(result.size());
  for (auto it = result.begin(); it != result.end(); it++) {
    auto obj = strai::internal::Serializer::Deserialize<std::shared_ptr<T>>((*it)->data(),
                                                                          (*it)->size());
    return_objects.push_back(std::move(obj));
  }
  return return_objects;
}

template <typename T>
inline std::vector<std::shared_ptr<T>> Get(const std::vector<strai::ObjectRef<T>> &ids,
                                           const int &timeout_ms) {
  auto object_ids = ObjectRefsToObjectIDs<T>(ids);
  return Get<T>(object_ids, timeout_ms);
}

template <typename T>
inline std::shared_ptr<T> Get(const strai::ObjectRef<T> &object) {
  return Get<T>(object, -1);
}

template <typename T>
inline std::vector<std::shared_ptr<T>> Get(const std::vector<std::string> &ids) {
  return Get<T>(ids, -1);
}

template <typename T>
inline std::vector<std::shared_ptr<T>> Get(const std::vector<strai::ObjectRef<T>> &ids) {
  return Get<T>(ids, -1);
}

template <typename T>
inline WaitResult<T> Wait(const std::vector<strai::ObjectRef<T>> &objects,
                          int num_objects,
                          int timeout_ms) {
  auto object_ids = ObjectRefsToObjectIDs<T>(objects);
  auto results =
      strai::internal::GetStraiRuntime()->Wait(object_ids, num_objects, timeout_ms);
  std::list<strai::ObjectRef<T>> readys;
  std::list<strai::ObjectRef<T>> unreadys;
  for (size_t i = 0; i < results.size(); i++) {
    if (results[i] == true) {
      readys.emplace_back(objects[i]);
    } else {
      unreadys.emplace_back(objects[i]);
    }
  }
  return WaitResult<T>(std::move(readys), std::move(unreadys));
}

inline strai::internal::ActorCreator<PyActorClass> Actor(PyActorClass func) {
  strai::internal::RemoteFunctionHolder remote_func_holder(func.module_name,
                                                         func.function_name,
                                                         func.class_name,
                                                         strai::internal::LangType::PYTHON);
  return {strai::internal::GetStraiRuntime().get(), std::move(remote_func_holder)};
}

template <typename R>
inline strai::internal::TaskCaller<PyFunction<R>> Task(PyFunction<R> func) {
  strai::internal::RemoteFunctionHolder remote_func_holder(
      func.module_name, func.function_name, "", strai::internal::LangType::PYTHON);
  return {strai::internal::GetStraiRuntime().get(), std::move(remote_func_holder)};
}

template <typename R>
inline strai::internal::TaskCaller<JavaFunction<R>> Task(JavaFunction<R> func) {
  strai::internal::RemoteFunctionHolder remote_func_holder(
      "", func.function_name, func.class_name, strai::internal::LangType::JAVA);
  return {strai::internal::GetStraiRuntime().get(), std::move(remote_func_holder)};
}

inline strai::internal::ActorCreator<JavaActorClass> Actor(JavaActorClass func) {
  strai::internal::RemoteFunctionHolder remote_func_holder(func.module_name,
                                                         func.function_name,
                                                         func.class_name,
                                                         strai::internal::LangType::JAVA);
  return {strai::internal::GetStraiRuntime().get(), std::move(remote_func_holder)};
}

/// Normal task.
template <typename F>
inline strai::internal::TaskCaller<F> Task(F func) {
  static_assert(!strai::internal::is_python_v<F>, "Must be a cpp function.");
  static_assert(!std::is_member_function_pointer_v<F>,
                "Incompatible type: member function cannot be called with strai::Task.");
  auto func_name = internal::FunctionManager::Instance().GetFunctionName(func);
  strai::internal::RemoteFunctionHolder remote_func_holder(std::move(func_name));
  return strai::internal::TaskCaller<F>(strai::internal::GetStraiRuntime().get(),
                                      std::move(remote_func_holder));
}

/// Creating an actor.
template <typename F>
inline strai::internal::ActorCreator<F> Actor(F create_func) {
  auto func_name = internal::FunctionManager::Instance().GetFunctionName(create_func);
  // Cpp actor don't need class_name, But java/python calls cpp actor need class name
  // param.
  auto class_name = internal::FunctionManager::GetClassNameByFuncName(func_name);
  strai::internal::RemoteFunctionHolder remote_func_holder(
      "", std::move(func_name), std::move(class_name), internal::LangType::CPP);
  return strai::internal::ActorCreator<F>(strai::internal::GetStraiRuntime().get(),
                                        std::move(remote_func_holder));
}

// Get the cpp actor handle by name.
template <typename T>
boost::optional<ActorHandle<T>> GetActor(const std::string &actor_name) {
  return GetActor<T>(actor_name, "");
}

template <typename T>
boost::optional<ActorHandle<T>> GetActor(const std::string &actor_name,
                                         const std::string &strai_namespace) {
  if (actor_name.empty()) {
    return {};
  }

  auto actor_id = strai::internal::GetStraiRuntime()->GetActorId(actor_name, strai_namespace);
  if (actor_id.empty()) {
    return {};
  }

  return ActorHandle<T>(actor_id);
}

// Get the cross-language actor handle by name.
inline boost::optional<ActorHandleXlang> GetActor(const std::string &actor_name,
                                                  const std::string &strai_namespce = "") {
  if (actor_name.empty()) {
    return {};
  }

  auto actor_id = strai::internal::GetStraiRuntime()->GetActorId(actor_name, strai_namespce);
  if (actor_id.empty()) {
    return {};
  }

  return ActorHandleXlang(actor_id);
}

inline void ExitActor() { strai::internal::GetStraiRuntime()->ExitActor(); }

inline PlacementGroup CreatePlacementGroup(
    const strai::PlacementGroupCreationOptions &create_options) {
  return strai::internal::GetStraiRuntime()->CreatePlacementGroup(create_options);
}

inline void RemovePlacementGroup(const std::string &placement_group_id) {
  return strai::internal::GetStraiRuntime()->RemovePlacementGroup(placement_group_id);
}

inline std::vector<PlacementGroup> GetAllPlacementGroups() {
  return strai::internal::GetStraiRuntime()->GetAllPlacementGroups();
}

inline PlacementGroup GetPlacementGroupById(const std::string &id) {
  return strai::internal::GetStraiRuntime()->GetPlacementGroupById(id);
}

inline PlacementGroup GetPlacementGroup(const std::string &name) {
  return strai::internal::GetStraiRuntime()->GetPlacementGroup(name);
}

inline bool WasCurrentActorRestarted() {
  return strai::internal::GetStraiRuntime()->WasCurrentActorRestarted();
}

inline std::string GetNamespace() {
  return strai::internal::GetStraiRuntime()->GetNamespace();
}

}  // namespace strai
