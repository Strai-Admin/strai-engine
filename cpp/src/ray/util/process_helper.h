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
#include <string>

#include "../config_internal.h"
#include "strai/core_worker/core_worker.h"
#include "strai/gcs/gcs_client/global_state_accessor.h"
#include "util.h"

namespace strai {
namespace internal {

using strai::core::CoreWorkerOptions;

class ProcessHelper {
 public:
  void StraiStart(CoreWorkerOptions::TaskExecutionCallback callback);
  void StraiStop();
  void StartStraiNode(const std::string node_id_address,
                    const int port,
                    const std::string redis_password,
                    const std::vector<std::string> &head_args = {});
  void StopStraiNode();

  static ProcessHelper &GetInstance() {
    static ProcessHelper processHelper;
    return processHelper;
  }

  std::unique_ptr<strai::gcs::GlobalStateAccessor> CreateGlobalStateAccessor(
      const std::string &gcs_ip, int gcs_port);

  ProcessHelper(ProcessHelper const &) = delete;
  void operator=(ProcessHelper const &) = delete;

 private:
  ProcessHelper(){};
};

}  // namespace internal
}  // namespace strai
