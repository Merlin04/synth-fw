/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include <yoga/Yoga.h>

#include "log.h"
#include "YGConfig.h"
#include "YGNode.h"

namespace facebook {
namespace yoga {
namespace detail {

namespace {

void vlog(
    YGConfig* config,
    YGNode* node,
    YGLogLevel level,
    void* context,
    const char* format,
    va_list args) {
  #ifdef DEBUG
  YGConfig* logConfig = config != nullptr ? config : YGConfigGetDefault();
  logConfig->log(logConfig, node, level, context, format, args);
  #endif
}
} // namespace

YOGA_EXPORT void Log::log(
    YGNode* node,
    YGLogLevel level,
    void* context,
    const char* format,
    ...) noexcept {
  #ifdef DEBUG
  va_list args;
  va_start(args, format);
  vlog(
      node == nullptr ? nullptr : node->getConfig(),
      node,
      level,
      context,
      format,
      args);
  va_end(args);
  #endif
}

void Log::log(
    YGConfig* config,
    YGLogLevel level,
    void* context,
    const char* format,
    ...) noexcept {
  #ifdef DEBUG
  va_list args;
  va_start(args, format);
  vlog(config, nullptr, level, context, format, args);
  va_end(args);
  #endif
}

} // namespace detail
} // namespace yoga
} // namespace facebook
