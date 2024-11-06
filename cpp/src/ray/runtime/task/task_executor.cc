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

#include "task_executor.h"

#include <strai/api/common_types.h>

#include <memory>

#include "../../util/function_helper.h"
#include "../abstract_strai_runtime.h"
#include "strai/util/event.h"
#include "strai/util/event_label.h"

namespace strai {

namespace internal {
/// Execute remote functions by networking stream.
msgpack::sbuffer TaskExecutionHandler(const std::string &func_name,
                                      const ArgsBufferList &args_buffer,
                                      msgpack::sbuffer *actor_ptr) {
  if (func_name.empty()) {
    throw std::invalid_argument("Task function name is empty");
  }

  msgpack::sbuffer result;
  do {
    if (actor_ptr) {
      auto func_ptr = FunctionManager::Instance().GetMemberFunction(func_name);
      if (func_ptr == nullptr) {
        result = PackError("unknown actor task: " + func_name);
        break;
      }
      result = (*func_ptr)(actor_ptr, args_buffer);
    } else {
      auto func_ptr = FunctionManager::Instance().GetFunction(func_name);
      if (func_ptr == nullptr) {
        result = PackError("unknown function: " + func_name);
        break;
      }
      result = (*func_ptr)(args_buffer);
    }
  } while (0);

  return result;
}

auto &init_func_manager = FunctionManager::Instance();

FunctionManager &GetFunctionManager() { return init_func_manager; }

std::pair<const RemoteFunctionMap_t &, const RemoteMemberFunctionMap_t &>
GetRemoteFunctions() {
  return init_func_manager.GetRemoteFunctions();
}

void InitStraiRuntime(std::shared_ptr<StraiRuntime> runtime) {
  StraiRuntimeHolder::Instance().Init(runtime);
}
}  // namespace internal

namespace internal {

using strai::core::CoreWorkerProcess;

std::shared_ptr<msgpack::sbuffer> TaskExecutor::current_actor_ = nullptr;

/// TODO(qicosmos): Need to add more details of the error messages, such as object id,
/// task id etc.
std::pair<Status, std::shared_ptr<msgpack::sbuffer>> GetExecuteResult(
    const std::string &func_name,
    const ArgsBufferList &args_buffer,
    msgpack::sbuffer *actor_ptr) {
  try {
    EntryFuntion entry_function;
    if (actor_ptr == nullptr) {
      entry_function = FunctionHelper::GetInstance().GetExecutableFunctions(func_name);
    } else {
      entry_function =
          FunctionHelper::GetInstance().GetExecutableMemberFunctions(func_name);
    }
    STRAI_LOG(DEBUG) << "Get executable function " << func_name << " ok.";
    auto result = entry_function(func_name, args_buffer, actor_ptr);
    STRAI_LOG(DEBUG) << "Execute function " << func_name << " ok.";
    return std::make_pair(strai::Status::OK(),
                          std::make_shared<msgpack::sbuffer>(std::move(result)));
  } catch (StraiIntentionalSystemExitException &e) {
    STRAI_LOG(ERROR) << "Strai intentional system exit while executing function(" << func_name
                   << ").";
    return std::make_pair(strai::Status::IntentionalSystemExit(""), nullptr);
  } catch (const std::exception &e) {
#ifdef _WIN32
    auto exception_name = std::string(typeid(e).name());
#else
    auto exception_name =
        std::string(abi::__cxa_demangle(typeid(e).name(), nullptr, nullptr, nullptr));
#endif
    std::string err_msg = "An exception was thrown while executing function(" +
                          func_name + "): " + exception_name + ": " + e.what();
    STRAI_LOG(ERROR) << err_msg;
    return std::make_pair(strai::Status::Invalid(err_msg), nullptr);
  } catch (...) {
    STRAI_LOG(ERROR) << "An unknown exception was thrown while executing function("
                   << func_name << ").";
    return std::make_pair(strai::Status::UnknownError(std::string("unknown exception")),
                          nullptr);
  }
}

Status TaskExecutor::ExecuteTask(
    const rpc::Address &caller_address,
    strai::TaskType task_type,
    const std::string task_name,
    const StraiFunction &strai_function,
    const std::unordered_map<std::string, double> &required_resources,
    const std::vector<std::shared_ptr<strai::StraiObject>> &args_buffer,
    const std::vector<rpc::ObjectReference> &arg_refs,
    const std::string &debugger_breakpoint,
    const std::string &serialized_retry_exception_allowlist,
    std::vector<std::pair<ObjectID, std::shared_ptr<StraiObject>>> *returns,
    std::vector<std::pair<ObjectID, std::shared_ptr<StraiObject>>> *dynamic_returns,
    std::vector<std::pair<ObjectID, bool>> *streaming_generator_returns,
    std::shared_ptr<strai::LocalMemoryBuffer> &creation_task_exception_pb_bytes,
    bool *is_retryable_error,
    std::string *application_error,
    const std::vector<ConcurrencyGroup> &defined_concurrency_groups,
    const std::string name_of_concurrency_group_to_execute,
    bool is_reattempt,
    bool is_streaming_generator,
    bool retry_exception,
    int64_t generator_backpressure_num_objects) {
  STRAI_LOG(DEBUG) << "Execute task type: " << TaskType_Name(task_type)
                 << " name:" << task_name;
  STRAI_CHECK(strai_function.GetLanguage() == strai::Language::CPP);
  auto function_descriptor = strai_function.GetFunctionDescriptor();
  STRAI_CHECK(function_descriptor->Type() ==
            strai::FunctionDescriptorType::kCppFunctionDescriptor);
  auto typed_descriptor = function_descriptor->As<strai::CppFunctionDescriptor>();
  std::string func_name = typed_descriptor->FunctionName();
  bool cross_lang = !typed_descriptor->Caller().empty();
  // TODO(Clark): Support retrying application-level errors for C++.
  // TODO(Clark): Support exception allowlist for retrying application-level
  // errors for C++.
  *is_retryable_error = false;

  Status status{};
  std::shared_ptr<msgpack::sbuffer> data = nullptr;
  ArgsBufferList strai_args_buffer;
  for (size_t i = 0; i < args_buffer.size(); i++) {
    auto &arg = args_buffer.at(i);
    std::string meta_str = "";
    if (arg->GetMetadata() != nullptr) {
      meta_str = std::string((const char *)arg->GetMetadata()->Data(),
                             arg->GetMetadata()->Size());
    }
    msgpack::sbuffer sbuf;
    const char *arg_data = nullptr;
    size_t arg_data_size = 0;
    if (arg->GetData()) {
      arg_data = reinterpret_cast<const char *>(arg->GetData()->Data());
      arg_data_size = arg->GetData()->Size();
    }
    if (meta_str == METADATA_STR_RAW) {
      // TODO(LarryLian) In order to minimize the modification,
      // there is an extra serialization here, but the performance will be a little worse.
      // This code can be optimized later to improve performance
      const auto &raw_buffer = Serializer::Serialize(arg_data, arg_data_size);
      sbuf.write(raw_buffer.data(), raw_buffer.size());
    } else if (cross_lang) {
      STRAI_CHECK(arg_data != nullptr)
          << "Task " << task_name << " no." << i << " arg data is null.";
      sbuf.write(arg_data + XLANG_HEADER_LEN, arg_data_size - XLANG_HEADER_LEN);
    } else {
      sbuf.write(arg_data, arg_data_size);
    }

    strai_args_buffer.push_back(std::move(sbuf));
  }
  if (task_type == strai::TaskType::ACTOR_CREATION_TASK) {
    std::tie(status, data) = GetExecuteResult(func_name, strai_args_buffer, nullptr);
    current_actor_ = data;
  } else if (task_type == strai::TaskType::ACTOR_TASK) {
    if (cross_lang) {
      STRAI_CHECK(!typed_descriptor->ClassName().empty());
      func_name = std::string("&")
                      .append(typed_descriptor->ClassName())
                      .append("::")
                      .append(typed_descriptor->FunctionName());
    }
    STRAI_CHECK(current_actor_ != nullptr);
    std::tie(status, data) =
        GetExecuteResult(func_name, strai_args_buffer, current_actor_.get());
  } else {  // NORMAL_TASK
    std::tie(status, data) = GetExecuteResult(func_name, strai_args_buffer, nullptr);
  }

  std::shared_ptr<strai::LocalMemoryBuffer> meta_buffer = nullptr;
  if (!status.ok()) {
    if (status.IsIntentionalSystemExit()) {
      return status;
    } else {
      STRAI_EVENT(ERROR, EL_STRAI_CPP_TASK_FAILED)
              .WithField("task_type", TaskType_Name(task_type))
              .WithField("function_name", func_name)
          << "C++ task failed: " << status.ToString();
    }

    std::string meta_str = std::to_string(strai::rpc::ErrorType::TASK_EXECUTION_EXCEPTION);
    meta_buffer = std::make_shared<strai::LocalMemoryBuffer>(
        reinterpret_cast<uint8_t *>(&meta_str[0]), meta_str.size(), true);
    // Pass formatted exception string to CoreWorker
    *application_error = status.ToString();

    msgpack::sbuffer buf;
    if (cross_lang) {
      strai::rpc::StraiException strai_exception{};
      strai_exception.set_language(strai::rpc::Language::CPP);
      strai_exception.set_formatted_exception_string(status.ToString());
      auto msg = strai_exception.SerializeAsString();
      buf = Serializer::Serialize(msg.data(), msg.size());
    } else {
      std::string msg = status.ToString();
      buf.write(msg.data(), msg.size());
    }
    data = std::make_shared<msgpack::sbuffer>(std::move(buf));
  }

  if (task_type != strai::TaskType::ACTOR_CREATION_TASK) {
    size_t data_size = data->size();
    auto &result_id = (*returns)[0].first;
    auto result_ptr = &(*returns)[0].second;
    int64_t task_output_inlined_bytes = 0;

    if (cross_lang && meta_buffer == nullptr) {
      meta_buffer = std::make_shared<strai::LocalMemoryBuffer>(
          (uint8_t *)(&METADATA_STR_XLANG[0]), METADATA_STR_XLANG.size(), true);
    }

    size_t total = cross_lang ? (XLANG_HEADER_LEN + data_size) : data_size;
    STRAI_CHECK_OK(CoreWorkerProcess::GetCoreWorker().AllocateReturnObject(
        result_id,
        total,
        meta_buffer,
        std::vector<strai::ObjectID>(),
        caller_address,
        &task_output_inlined_bytes,
        result_ptr));

    auto result = *result_ptr;
    if (result != nullptr) {
      if (result->HasData()) {
        if (cross_lang) {
          auto len_buf = Serializer::Serialize(data_size);

          msgpack::sbuffer buffer(XLANG_HEADER_LEN + data_size);
          buffer.write(len_buf.data(), len_buf.size());
          for (size_t i = 0; i < XLANG_HEADER_LEN - len_buf.size(); ++i) {
            buffer.write("", 1);
          }
          buffer.write(data->data(), data_size);

          memcpy(result->GetData()->Data(), buffer.data(), buffer.size());
        } else {
          memcpy(result->GetData()->Data(), data->data(), data_size);
        }
      }
    }

    STRAI_CHECK_OK(CoreWorkerProcess::GetCoreWorker().SealReturnObject(
        result_id,
        result,
        /*generator_id=*/ObjectID::Nil(),
        caller_address));
  } else {
    if (!status.ok()) {
      return strai::Status::CreationTaskError("");
    }
  }
  return strai::Status::OK();
}

void TaskExecutor::Invoke(
    const TaskSpecification &task_spec,
    std::shared_ptr<msgpack::sbuffer> actor,
    AbstractStraiRuntime *runtime,
    std::unordered_map<ActorID, std::unique_ptr<ActorContext>> &actor_contexts,
    absl::Mutex &actor_contexts_mutex) {
  ArgsBufferList args_buffer;
  for (size_t i = 0; i < task_spec.NumArgs(); i++) {
    if (task_spec.ArgByRef(i)) {
      const auto &id = task_spec.ArgId(i).Binary();
      msgpack::sbuffer sbuf;
      sbuf.write(id.data(), id.size());
      args_buffer.push_back(std::move(sbuf));
    } else {
      msgpack::sbuffer sbuf;
      sbuf.write((const char *)task_spec.ArgData(i), task_spec.ArgDataSize(i));
      args_buffer.push_back(std::move(sbuf));
    }
  }

  auto function_descriptor = task_spec.FunctionDescriptor();
  auto typed_descriptor = function_descriptor->As<strai::CppFunctionDescriptor>();

  std::shared_ptr<msgpack::sbuffer> data;
  try {
    if (actor) {
      auto result = TaskExecutionHandler(
          typed_descriptor->FunctionName(), args_buffer, actor.get());
      data = std::make_shared<msgpack::sbuffer>(std::move(result));
      runtime->Put(std::move(data), task_spec.ReturnId(0));
    } else {
      auto result =
          TaskExecutionHandler(typed_descriptor->FunctionName(), args_buffer, nullptr);
      data = std::make_shared<msgpack::sbuffer>(std::move(result));
      if (task_spec.IsActorCreationTask()) {
        std::unique_ptr<ActorContext> actorContext(new ActorContext());
        actorContext->current_actor = data;
        absl::MutexLock lock(&actor_contexts_mutex);
        actor_contexts.emplace(task_spec.ActorCreationId(), std::move(actorContext));
      } else {
        runtime->Put(std::move(data), task_spec.ReturnId(0));
      }
    }
  } catch (std::exception &e) {
    auto result = PackError(e.what());
    auto data = std::make_shared<msgpack::sbuffer>(std::move(result));
    runtime->Put(std::move(data), task_spec.ReturnId(0));
  }
}

}  // namespace internal
}  // namespace strai
