#include "HTMLWriter.h"
#include <iostream>

std::stringstream HTMLWriter::stream;
std::ofstream HTMLWriter::writer;

bool HTMLWriter::WriteHead(const char* path, const char* title) {
  writer.open(path, std::ios_base::out);
  if (!writer.is_open()) {
    char buffer[1024];
    strerror_s(buffer, errno);
    stream << "[ERROR] Could not open " << path << " for writing: " << buffer;
    std::cout << stream.str().c_str() << std::endl;
    return false;
  }
  stream << "<!DOCTYPE html>\n";
  stream << "<html>\n";
  stream << "<head>\n";
  stream << "  <meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n";
  stream << "  <meta charset=\"UTF-8\">\n";
  stream << "  <meta name=\"apple-mobile-web-app-capable\" content=\"yes\">\n";
  stream << "  <meta name=\"apple-mobile-web-app-status-bar-style\" content=\"black-translucent\">\n";
  stream << "  <meta name=\"viewport\" content=\"user-scalable=no\">\n";
  stream << "  <link rel=\"icon\" href=\"./icon.png\" type=\"image/png\">\n";
  stream << "  <link rel=\"apple-touch-icon\" href=\"./icon.png\">\n";
  stream << "  <link rel=\"stylesheet\" type=\"text/css\" href=\"./fonts/gamefont.css\">\n";
  stream << "  <title>" << title << "</title>\n";
  stream << "</head>\n";
  stream << "<body style=\"-webkit-user-select: none; background-color: black;\" cz-shortcut-listen=\"true\">\n";
  stream << "  <!-- External Libraries -->\n";
  stream << "  <script type=\"text/javascript\" src=\"./js/libs/pixi.js\"></script>\n";
  stream << "  <script type=\"text/javascript\" src=\"./js/libs/pixi-tilemap.js\"></script>\n";
  stream << "  <script type=\"text/javascript\" src=\"./js/libs/pixi-picture.js\"></script>\n";
  stream << "  <script type=\"text/javascript\" src=\"./js/libs/fpsmeter.js\"></script>\n";
  stream << "  <script type=\"text/javascript\" src=\"./js/libs/lz-string.js\"></script>\n\n";
  stream << "  <script type=\"text/javascript\" src=\"./js/libs/iphone-inline-video.browser.js\"></script>\n\n";
  return true;
}

void HTMLWriter::WriteSection(const char* comment, std::vector<std::string> sections) {
  stream << "  <!-- " << comment << " -->\n";
  for (unsigned int i = 0; i < sections.size(); i++) {
    stream << "  <script type=\"text/javascript\" src=\"" << sections[i] << ".js\"></script>\n";
  }
  stream << "\n";
}

void HTMLWriter::WriteEnd() {
  stream << "  <!-- Plugins! -->\n";
  stream << "  <script type=\"text/javascript\" src=\"./js/plugins.js\"></script>\n";
  stream << "  <script type=\"text/javascript\" src=\"./js/main.js\"></script>\n";
  stream << "</body>\n";
  stream << "</html>\n";
  auto string = stream.str();
  writer.write(string.c_str(), string.size());
  writer.close();
}