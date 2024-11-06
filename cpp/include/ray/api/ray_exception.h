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

#include <exception>
#include <string>

namespace strai {
namespace internal {

class StraiException : public std::exception {
 public:
  StraiException(const std::string &msg) : msg_(msg){};

  const char *what() const noexcept override { return msg_.c_str(); };

  std::string msg_;
};

class StraiActorException : public StraiException {
 public:
  StraiActorException(const std::string &msg) : StraiException(msg){};
};

class StraiTaskException : public StraiException {
 public:
  StraiTaskException(const std::string &msg) : StraiException(msg){};
};

class StraiWorkerException : public StraiException {
 public:
  StraiWorkerException(const std::string &msg) : StraiException(msg){};
};

class UnreconstructableException : public StraiException {
 public:
  UnreconstructableException(const std::string &msg) : StraiException(msg){};
};

class StraiFunctionNotFound : public StraiException {
 public:
  StraiFunctionNotFound(const std::string &msg) : StraiException(msg){};
};

class StraiRuntimeEnvException : public StraiException {
 public:
  StraiRuntimeEnvException(const std::string &msg) : StraiException(msg){};
};

class StraiTimeoutException : public StraiException {
 public:
  StraiTimeoutException(const std::string &msg) : StraiException(msg){};
};

}  // namespace internal
}  // namespace strai