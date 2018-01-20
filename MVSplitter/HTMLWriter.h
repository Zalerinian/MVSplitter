#pragma once
#include <vector>
#include <sstream>
#include <fstream>

struct HTMLWriter {
  static bool WriteHead(const char* path, const char* title);
  static void WriteSection(const char* comment, std::vector<std::string> scripts);
  static void WriteEnd();
  static std::stringstream stream;
  static std::ofstream writer;
};