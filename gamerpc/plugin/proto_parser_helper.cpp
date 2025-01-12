// Copyright 2023 The gRPC Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "proto_parser_helper.h"

namespace grpc_generator {

std::string EscapeVariableDelimiters(const std::string& original) {
  std::string mut_str = original;
  size_t index = 0;
  while ((index = mut_str.find('$', index)) != std::string::npos) {
    mut_str.replace(index, 1, "$$");
    index += 2;
  }
  return mut_str;
}

}  // namespace grpc_generator
