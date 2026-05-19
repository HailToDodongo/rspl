#include "preproc.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace rspl {

std::string stripComments(const std::string &source) {
  std::istringstream iss(source);
  std::string line;
  std::string result;

  while (std::getline(iss, line)) {
    // Remove // comments
    auto pos = line.find("//");
    if (pos != std::string::npos) {
      line = line.substr(0, pos);
    }
    result += line + "\n";
  }

  // Remove /* */ block comments, preserving newline count
  std::regex blockRe("/\\*[\\s\\S]*?\\*/");
  std::string tmp;
  auto it = std::sregex_iterator(result.begin(), result.end(), blockRe);
  auto end = std::sregex_iterator();
  size_t prevPos = 0;
  for (auto i = it; i != end; ++i) {
    tmp += result.substr(prevPos, i->position() - prevPos);
    std::string match = i->str();
    for (char c : match)
      if (c == '\n') tmp += '\n';
    prevPos = i->position() + i->length();
  }
  tmp += result.substr(prevPos);
  return tmp;
}

std::string preprocess(const std::string &src,
                       std::unordered_map<std::string, DefineEntry> &defines,
                       const std::string &sourceDir) {
  std::istringstream iss(src);
  std::string line;
  std::string result;
  bool insideIfdef = false;
  bool ignoreLine = false;
  int lineNum = 0;

  auto replaceDefines = [&](std::string l) -> std::string {
    for (const auto &[name, entry] : defines) {
      std::string patStr =
          "(\\$\\{" + name + "\\})|(\\b" + name + "\\b)";
      l = std::regex_replace(l, std::regex(patStr), entry.value);
    }
    return l;
  };

  while (std::getline(iss, line)) {
    ++lineNum;
    std::string trimmed = line;
    size_t firstNonSpace = trimmed.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos) {
      trimmed = trimmed.substr(firstNonSpace);
    } else {
      trimmed.clear();
    }
    std::string newLine;

    if (!ignoreLine && trimmed.starts_with("#define")) {
      std::regex defRe("#define\\s+([a-zA-Z0-9_]+)\\s+(.*)");
      std::smatch m;
      if (!std::regex_match(trimmed, m, defRe)) {
        throw std::runtime_error(
            "Line " + std::to_string(lineNum) +
            ": Invalid #define statement!");
      }
      std::string name = m[1].str();
      std::string value = replaceDefines(m[2].str());
      defines[name] = {name, value};
    } else if (!ignoreLine && trimmed.starts_with("#undef")) {
      std::regex undefRe("#undef\\s+([a-zA-Z0-9_]+)");
      std::smatch m;
      if (!std::regex_match(trimmed, m, undefRe)) {
        throw std::runtime_error(
            "Line " + std::to_string(lineNum) +
            ": Invalid #undef statement!");
      }
      defines.erase(m[1].str());
    } else if (trimmed.starts_with("#ifdef") ||
               trimmed.starts_with("#ifndef")) {
      if (insideIfdef) {
        throw std::runtime_error(
            "Line " + std::to_string(lineNum) +
            ": Nested #ifdef not allowed!");
      }
      insideIfdef = true;
      std::regex ifdefRe("#ifn?def\\s+([a-zA-Z0-9_]+)");
      std::smatch m;
      if (!std::regex_match(trimmed, m, ifdefRe)) {
        throw std::runtime_error(
            "Line " + std::to_string(lineNum) +
            ": Invalid #ifdef statement!");
      }
      bool isIfdef = trimmed.starts_with("#ifdef");
      std::string name = m[1].str();
      ignoreLine = isIfdef ? !defines.count(name) : defines.count(name);
    } else if (trimmed.starts_with("#else")) {
      ignoreLine = insideIfdef && !ignoreLine;
    } else if (trimmed.starts_with("#endif")) {
      insideIfdef = false;
      ignoreLine = false;
    } else if (!ignoreLine && trimmed.starts_with("#include")) {
      std::regex incRe("#include\\s+\"(.*)\"");
      std::smatch m;
      if (!std::regex_match(trimmed, m, incRe)) {
        throw std::runtime_error(
            "Line " + std::to_string(lineNum) +
            ": Invalid #include!");
      }
      std::string path = m[1].str();
      std::string fullPath = sourceDir + "/" + path;
      std::ifstream incFile(fullPath);
      if (!incFile) {
        throw std::runtime_error(
            "Line " + std::to_string(lineNum) +
            ": Cannot open include: " + fullPath);
      }
      std::string incSrc;
      {
        std::ostringstream ss;
        ss << incFile.rdbuf();
        incSrc = ss.str();
      }
      result += preprocess(stripComments(incSrc), defines, sourceDir);
    } else if (!ignoreLine) {
      newLine = replaceDefines(line);
    }

    result += newLine + "\n";
  }

  return result;
}

std::string preprocFull(const std::string &src,
                        std::unordered_map<std::string, DefineEntry> &defines,
                        const std::string &sourceDir) {
  return preprocess(stripComments(src), defines, sourceDir);
}

} // namespace rspl
