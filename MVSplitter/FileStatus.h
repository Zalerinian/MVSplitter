#pragma once
#include <string>

struct FileStatus {
  std::string name;
  bool complete = false;

  bool operator==(const FileStatus& other) {
    return name == other.name;
  }

  bool operator==(const std::string& other) {
    return name == other;
  }
};