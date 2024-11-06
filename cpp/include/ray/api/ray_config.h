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
#include <strai/api/strai_exception.h>
#include <strai/api/runtime_env.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "boost/optional.hpp"

namespace strai {

enum class ActorLifetime {
  NON_DETACHED,
  DETACHED,
};

class StraiConfig {
 public:
  // The address of the Strai cluster to connect to.
  // If not provided, it will be initialized from environment variable "STRAI_ADDRESS" by
  // default.
  std::string address = "";

  // Whether or not to run this application in a local mode. This is used for debugging.
  bool local_mode = false;

  // An arstrai of directories or dynamic library files that specify the search path for
  // user code. This parameter is not used when the application runs in local mode.
  // Only searching the top level under a directory.
  std::vector<std::string> code_search_path;

  // The command line args to be appended as parameters of the `strai start` command. It
  // takes effect only if Strai head is started by a driver. Run `strai start --help` for
  // details.
  std::vector<std::string> head_args = {};

  // The default actor lifetime type, `DETACHED` or `NON_DETACHED`.
  ActorLifetime default_actor_lifetime = ActorLifetime::NON_DETACHED;

  // The job level runtime environments.
  boost::optional<RuntimeEnv> runtime_env;

  /* The following are unstable parameters and their use is discouraged. */

  // Prevents external clients without the password from connecting to Redis if provided.
  boost::optional<std::string> redis_password_;

  // A specific flag for internal `default_worker`. Please don't use it in user code.
  bool is_worker_ = false;

  // A namespace is a logical grouping of jobs and named actors.
  std::string strai_namespace = "";
};

}  // namespace strai
