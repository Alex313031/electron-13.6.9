// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <set>
#include <string>
#include <utility>

#include "base/files/file_util.h"
#include "base/optional.h"
#include "base/task/thread_pool.h"
#include "base/threading/thread_restrictions.h"
#include "content/public/browser/tracing_controller.h"
#include "shell/common/gin_converters/callback_converter.h"
#include "shell/common/gin_converters/file_path_converter.h"
#include "shell/common/gin_converters/value_converter.h"
#include "shell/common/gin_helper/dictionary.h"
#include "shell/common/gin_helper/promise.h"
#include "shell/common/node_includes.h"

using content::TracingController;

namespace gin {

template <>
struct Converter<base::trace_event::TraceConfig> {
  static bool FromV8(v8::Isolate* isolate,
                     v8::Local<v8::Value> val,
                     base::trace_event::TraceConfig* out) {
    // (alexeykuzmin): A combination of "categoryFilter" and "traceOptions"
    // has to be checked first because none of the fields
    // in the `memory_dump_config` dict below are mandatory
    // and we cannot check the config format.
    gin_helper::Dictionary options;
    if (ConvertFromV8(isolate, val, &options)) {
      std::string category_filter, trace_options;
      if (options.Get("categoryFilter", &category_filter) &&
          options.Get("traceOptions", &trace_options)) {
        *out = base::trace_event::TraceConfig(category_filter, trace_options);
        return true;
      }
    }

    base::DictionaryValue memory_dump_config;
    if (ConvertFromV8(isolate, val, &memory_dump_config)) {
      *out = base::trace_event::TraceConfig(memory_dump_config);
      return true;
    }

    return false;
  }
};

}  // namespace gin

namespace {

using CompletionCallback = base::OnceCallback<void(const base::FilePath&)>;

base::Optional<base::FilePath> CreateTemporaryFileOnIO() {
  base::FilePath temp_file_path;
  if (!base::CreateTemporaryFile(&temp_file_path))
    return base::nullopt;
  return base::make_optional(std::move(temp_file_path));
}

void StopTracing(gin_helper::Promise<base::FilePath> promise,
                 base::Optional<base::FilePath> file_path) {
  auto resolve_or_reject = base::AdaptCallbackForRepeating(base::BindOnce(
      [](gin_helper::Promise<base::FilePath> promise,
         const base::FilePath& path, base::Optional<std::string> error) {
        if (error) {
          promise.RejectWithErrorMessage(error.value());
        } else {
          promise.Resolve(path);
        }
      },
      std::move(promise), *file_path));
  if (file_path) {
    auto endpoint = TracingController::CreateFileEndpoint(
        *file_path, base::BindRepeating(resolve_or_reject, base::nullopt));
    if (!TracingController::GetInstance()->StopTracing(endpoint)) {
      resolve_or_reject.Run(base::make_optional(
          "Failed to stop tracing (was a trace in progress?)"));
    }
  } else {
    resolve_or_reject.Run(
        base::make_optional("Failed to create temporary file for trace data"));
  }
}

v8::Local<v8::Promise> StopRecording(gin_helper::Arguments* args) {
  gin_helper::Promise<base::FilePath> promise(args->isolate());
  v8::Local<v8::Promise> handle = promise.GetHandle();

  base::FilePath path;
  if (args->GetNext(&path) && !path.empty()) {
    StopTracing(std::move(promise), base::make_optional(path));
  } else {
    // use a temporary file.
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::BindOnce(CreateTemporaryFileOnIO),
        base::BindOnce(StopTracing, std::move(promise)));
  }

  return handle;
}

v8::Local<v8::Promise> GetCategories(v8::Isolate* isolate) {
  gin_helper::Promise<const std::set<std::string>&> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // Note: This method always succeeds.
  TracingController::GetInstance()->GetCategories(base::BindOnce(
      gin_helper::Promise<const std::set<std::string>&>::ResolvePromise,
      std::move(promise)));

  return handle;
}

v8::Local<v8::Promise> StartTracing(
    v8::Isolate* isolate,
    const base::trace_event::TraceConfig& trace_config) {
  gin_helper::Promise<void> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  if (!TracingController::GetInstance()->StartTracing(
          trace_config,
          base::BindOnce(gin_helper::Promise<void>::ResolvePromise,
                         std::move(promise)))) {
    // If StartTracing returns false, that means it didn't invoke its callback.
    // Return an already-resolved promise and abandon the previous promise (it
    // was std::move()d into the StartTracing callback and has been deleted by
    // this point).
    return gin_helper::Promise<void>::ResolvedPromise(isolate);
  }
  return handle;
}

void OnTraceBufferUsageAvailable(
    gin_helper::Promise<gin_helper::Dictionary> promise,
    float percent_full,
    size_t approximate_count) {
  gin_helper::Dictionary dict = gin::Dictionary::CreateEmpty(promise.isolate());
  dict.Set("percentage", percent_full);
  dict.Set("value", approximate_count);

  promise.Resolve(dict);
}

v8::Local<v8::Promise> GetTraceBufferUsage(v8::Isolate* isolate) {
  gin_helper::Promise<gin_helper::Dictionary> promise(isolate);
  v8::Local<v8::Promise> handle = promise.GetHandle();

  // Note: This method always succeeds.
  TracingController::GetInstance()->GetTraceBufferUsage(
      base::BindOnce(&OnTraceBufferUsageAvailable, std::move(promise)));
  return handle;
}

void Initialize(v8::Local<v8::Object> exports,
                v8::Local<v8::Value> unused,
                v8::Local<v8::Context> context,
                void* priv) {
  gin_helper::Dictionary dict(context->GetIsolate(), exports);
  dict.SetMethod("getCategories", &GetCategories);
  dict.SetMethod("startRecording", &StartTracing);
  dict.SetMethod("stopRecording", &StopRecording);
  dict.SetMethod("getTraceBufferUsage", &GetTraceBufferUsage);
}

}  // namespace

NODE_LINKED_MODULE_CONTEXT_AWARE(electron_browser_content_tracing, Initialize)
