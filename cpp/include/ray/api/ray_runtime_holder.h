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

#pragma once

#include <strai/api/strai_runtime.h>

namespace strai {
namespace internal {

struct StraiRuntimeHolder {
  static StraiRuntimeHolder &Instance() {
    static StraiRuntimeHolder instance;
    return instance;
  }

  void Init(std::shared_ptr<StraiRuntime> runtime) { runtime_ = runtime; }

  std::shared_ptr<StraiRuntime> Runtime() { return runtime_; }

 private:
  StraiRuntimeHolder() = default;
  ~StraiRuntimeHolder() = default;
  StraiRuntimeHolder(StraiRuntimeHolder const &) = delete;
  StraiRuntimeHolder(StraiRuntimeHolder &&) = delete;
  StraiRuntimeHolder &operator=(StraiRuntimeHolder const &) = delete;
  StraiRuntimeHolder &operator=(StraiRuntimeHolder &&) = delete;

  std::shared_ptr<StraiRuntime> runtime_;
};

inline static std::shared_ptr<StraiRuntime> GetStraiRuntime() {
  return StraiRuntimeHolder::Instance().Runtime();
}

}  // namespace internal
}  // namespace strai