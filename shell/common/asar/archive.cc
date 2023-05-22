// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/asar/archive.h"

#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/pickle.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "shell/common/asar/scoped_temporary_file.h"

#if defined(OS_WIN)
#include <io.h>
#endif

namespace asar {

namespace {

#if defined(OS_WIN)
const char kSeparators[] = "\\/";
#else
const char kSeparators[] = "/";
#endif

bool GetNodeFromPath(std::string path,
                     const base::DictionaryValue* root,
                     const base::DictionaryValue** out);

// Gets the "files" from "dir".
bool GetFilesNode(const base::DictionaryValue* root,
                  const base::DictionaryValue* dir,
                  const base::DictionaryValue** out) {
  // Test for symbol linked directory.
  std::string link;
  if (dir->GetStringWithoutPathExpansion("link", &link)) {
    const base::DictionaryValue* linked_node = nullptr;
    if (!GetNodeFromPath(link, root, &linked_node))
      return false;
    dir = linked_node;
  }

  return dir->GetDictionaryWithoutPathExpansion("files", out);
}

// Gets sub-file "name" from "dir".
bool GetChildNode(const base::DictionaryValue* root,
                  const std::string& name,
                  const base::DictionaryValue* dir,
                  const base::DictionaryValue** out) {
  if (name == "") {
    *out = root;
    return true;
  }

  const base::DictionaryValue* files = nullptr;
  return GetFilesNode(root, dir, &files) &&
         files->GetDictionaryWithoutPathExpansion(name, out);
}

// Gets the node of "path" from "root".
bool GetNodeFromPath(std::string path,
                     const base::DictionaryValue* root,
                     const base::DictionaryValue** out) {
  if (path == "") {
    *out = root;
    return true;
  }

  const base::DictionaryValue* dir = root;
  for (size_t delimiter_position = path.find_first_of(kSeparators);
       delimiter_position != std::string::npos;
       delimiter_position = path.find_first_of(kSeparators)) {
    const base::DictionaryValue* child = nullptr;
    if (!GetChildNode(root, path.substr(0, delimiter_position), dir, &child))
      return false;

    dir = child;
    path.erase(0, delimiter_position + 1);
  }

  return GetChildNode(root, path, dir, out);
}

bool FillFileInfoWithNode(Archive::FileInfo* info,
                          uint32_t header_size,
                          const base::DictionaryValue* node) {
  int size;
  if (!node->GetInteger("size", &size))
    return false;
  info->size = static_cast<uint32_t>(size);

  if (node->GetBoolean("unpacked", &info->unpacked) && info->unpacked)
    return true;

  std::string offset;
  if (!node->GetString("offset", &offset))
    return false;
  if (!base::StringToUint64(offset, &info->offset))
    return false;
  info->offset += header_size;

  node->GetBoolean("executable", &info->executable);

  return true;
}

}  // namespace

Archive::Archive(const base::FilePath& path)
    : initialized_(false), path_(path), file_(base::File::FILE_OK) {
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  file_.Initialize(path_, base::File::FLAG_OPEN | base::File::FLAG_READ);
#if defined(OS_WIN)
  fd_ = _open_osfhandle(reinterpret_cast<intptr_t>(file_.GetPlatformFile()), 0);
#elif defined(OS_POSIX)
  fd_ = file_.GetPlatformFile();
#endif
}

Archive::~Archive() {
#if defined(OS_WIN)
  if (fd_ != -1) {
    _close(fd_);
    // Don't close the handle since we already closed the fd.
    file_.TakePlatformFile();
  }
#endif
  base::ThreadRestrictions::ScopedAllowIO allow_io;
  file_.Close();
}

bool Archive::Init() {
  // Should only be initialized once
  CHECK(!initialized_);
  initialized_ = true;

  if (!file_.IsValid()) {
    if (file_.error_details() != base::File::FILE_ERROR_NOT_FOUND) {
      LOG(WARNING) << "Opening " << path_.value() << ": "
                   << base::File::ErrorToString(file_.error_details());
    }
    return false;
  }

  std::vector<char> buf;
  int len;

  buf.resize(8);
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    len = file_.ReadAtCurrentPos(buf.data(), buf.size());
  }
  if (len != static_cast<int>(buf.size())) {
    PLOG(ERROR) << "Failed to read header size from " << path_.value();
    return false;
  }

  uint32_t size;
  if (!base::PickleIterator(base::Pickle(buf.data(), buf.size()))
           .ReadUInt32(&size)) {
    LOG(ERROR) << "Failed to parse header size from " << path_.value();
    return false;
  }

  buf.resize(size);
  {
    base::ThreadRestrictions::ScopedAllowIO allow_io;
    len = file_.ReadAtCurrentPos(buf.data(), buf.size());
  }
  if (len != static_cast<int>(buf.size())) {
    PLOG(ERROR) << "Failed to read header from " << path_.value();
    return false;
  }

  std::string header;
  if (!base::PickleIterator(base::Pickle(buf.data(), buf.size()))
           .ReadString(&header)) {
    LOG(ERROR) << "Failed to parse header from " << path_.value();
    return false;
  }

  base::Optional<base::Value> value = base::JSONReader::Read(header);
  if (!value || !value->is_dict()) {
    LOG(ERROR) << "Failed to parse header";
    return false;
  }

  header_size_ = 8 + size;
  header_ = base::DictionaryValue::From(
      base::Value::ToUniquePtrValue(std::move(*value)));
  return true;
}

bool Archive::GetFileInfo(const base::FilePath& path, FileInfo* info) const {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  std::string link;
  if (node->GetString("link", &link))
    return GetFileInfo(base::FilePath::FromUTF8Unsafe(link), info);

  return FillFileInfoWithNode(info, header_size_, node);
}

bool Archive::Stat(const base::FilePath& path, Stats* stats) const {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  if (node->FindKey("link")) {
    stats->is_file = false;
    stats->is_link = true;
    return true;
  }

  if (node->FindKey("files")) {
    stats->is_file = false;
    stats->is_directory = true;
    return true;
  }

  return FillFileInfoWithNode(stats, header_size_, node);
}

bool Archive::Readdir(const base::FilePath& path,
                      std::vector<base::FilePath>* files) const {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  const base::DictionaryValue* files_node;
  if (!GetFilesNode(header_.get(), node, &files_node))
    return false;

  base::DictionaryValue::Iterator iter(*files_node);
  while (!iter.IsAtEnd()) {
    files->push_back(base::FilePath::FromUTF8Unsafe(iter.key()));
    iter.Advance();
  }
  return true;
}

bool Archive::Realpath(const base::FilePath& path,
                       base::FilePath* realpath) const {
  if (!header_)
    return false;

  const base::DictionaryValue* node;
  if (!GetNodeFromPath(path.AsUTF8Unsafe(), header_.get(), &node))
    return false;

  std::string link;
  if (node->GetString("link", &link)) {
    *realpath = base::FilePath::FromUTF8Unsafe(link);
    return true;
  }

  *realpath = path;
  return true;
}

bool Archive::CopyFileOut(const base::FilePath& path, base::FilePath* out) {
  if (!header_)
    return false;

  base::AutoLock auto_lock(external_files_lock_);

  auto it = external_files_.find(path.value());
  if (it != external_files_.end()) {
    *out = it->second->path();
    return true;
  }

  FileInfo info;
  if (!GetFileInfo(path, &info))
    return false;

  if (info.unpacked) {
    *out = path_.AddExtension(FILE_PATH_LITERAL("unpacked")).Append(path);
    return true;
  }

  auto temp_file = std::make_unique<ScopedTemporaryFile>();
  base::FilePath::StringType ext = path.Extension();
  if (!temp_file->InitFromFile(&file_, ext, info.offset, info.size))
    return false;

#if defined(OS_POSIX)
  if (info.executable) {
    // chmod a+x temp_file;
    base::SetPosixFilePermissions(temp_file->path(), 0755);
  }
#endif

  *out = temp_file->path();
  external_files_[path.value()] = std::move(temp_file);
  return true;
}

int Archive::GetFD() const {
  return fd_;
}

}  // namespace asar
