/**
 * RSPL C++ transpiler — CLI entry point.
 */

#include "pipeline.h"
#include "preproc.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

struct CliArgs {
  std::string inputFile;
  std::string outputFile;
  bool astDump = false;
  bool optimize = true;
  bool reorder = false;
  bool magma = false;
  int optimizeTime = 30'000;
  int optWorkers = 0; // 0 = auto (hw threads - 1)
  bool rspqWrapper = true;
  bool help = false;
  std::vector<std::string> defines; // "KEY=VALUE" pairs
};

void printHelp() {
  std::cout << R"(Usage: rspl <input.rspl> [options]

Options:
  -o <file>        Output .S file (default: input base + .S)
  -D KEY=VALUE     Define a preprocessor constant
  --opt-time=N     Optimizer time budget in seconds (default: 30)
  --opt-workers=N  Number of reorder worker threads (default: auto)
  --no-optimize    Disable optimization
  --reorder        Enable instruction reordering
  --no-rspq        Disable RSPQ wrapper
  --magma          Compile as a magma shader
  --ast-dump       Print parsed AST and exit
  -h, --help       Show this help
)";
}

CliArgs parseArgs(int argc, char **argv) {
  CliArgs args;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "-h" || arg == "--help") { args.help = true; }
    else if (arg == "--ast-dump") { args.astDump = true; }
    else if (arg == "-o" && i + 1 < argc) { args.outputFile = argv[++i]; }
    else if (arg == "--no-optimize") { args.optimize = false; }
    else if (arg == "--reorder") { args.reorder = true; }
    else if (arg == "--no-rspq") { args.rspqWrapper = false; }
    else if (arg == "--magma") { args.magma = true; }
    else if (arg == "-D" && i + 1 < argc) { args.defines.push_back(argv[++i]); }
    else if (arg.starts_with("-D")) { args.defines.push_back(arg.substr(2)); }
    else if (arg.starts_with("--opt-time=")) { args.optimizeTime = std::stoi(arg.substr(11)) * 1000; }
    else if (arg.starts_with("--opt-workers=")) { args.optWorkers = std::stoi(arg.substr(14)); }
    else if (!arg.starts_with("-")) { args.inputFile = arg; }
  }
  return args;
}

std::string readFile(const std::string &path) {
  std::ifstream f(path);
  if (!f) { std::cerr << "Error: cannot open file: " << path << "\n"; std::exit(1); }
  std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}

void writeFile(const std::string &path, const std::string &content) {
  std::ofstream f(path);
  if (!f) { std::cerr << "Error: cannot write file: " << path << "\n"; std::exit(1); }
  f << content;
}

std::string execJsParser(const std::string &rsplPath, bool skipPreproc) {
  const char *scriptPath = std::getenv("RSPL_PARSE_JS");
  std::string cmd;
  if (scriptPath) {
    cmd = std::string("node ") + scriptPath;
  } else {
    cmd = "node scripts/parse.js";
  }
  cmd += skipPreproc ? " --preprocessed " : " ";
  cmd += rsplPath;

  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe) { std::cerr << "Error: cannot start JS parser\n"; std::exit(1); }
  std::string result;
  char buf[4096];
  while (fgets(buf, sizeof(buf), pipe)) result += buf;
  int rc = pclose(pipe);
  if (rc != 0) { std::cerr << "Error: JS parser exited with code " << rc << "\n"; std::exit(1); }
  return result;
}

} // namespace

int main(int argc, char **argv) {
  CliArgs args = parseArgs(argc, argv);

  if (args.help) { printHelp(); return 0; }

  // Read source
  std::string source;
  if (!args.inputFile.empty()) {
    source = readFile(args.inputFile);
  } else {
    std::ostringstream ss; ss << std::cin.rdbuf(); source = ss.str();
  }
  if (source.empty()) { std::cerr << "Error: no input provided\n"; return 1; }

  // Parse defines
  std::unordered_map<std::string, rspl::DefineEntry> defines;
  for (const auto &d : args.defines) {
    auto eq = d.find('=');
    std::string key = d.substr(0, eq);
    std::string val = (eq != std::string::npos) ? d.substr(eq + 1) : "1";
    defines[key] = {key, val};
  }

  // Run C++ preprocessor (strip comments + defines + includes)
  std::string sourceDir = ".";
  if (!args.inputFile.empty()) {
    auto slash = args.inputFile.find_last_of('/');
    if (slash != std::string::npos)
      sourceDir = args.inputFile.substr(0, slash);
  }
  std::string preprocessed = rspl::preprocFull(source, defines, sourceDir);

  // Write preprocessed source to temp file for JS parser
  std::string tmpPath = "/tmp/rspl_preprocessed.rspl";
  writeFile(tmpPath, preprocessed);

  // Route through JS parser with --preprocessed flag
  std::string astJson = execJsParser(tmpPath, true);

  if (args.astDump) { std::cout << astJson; return 0; }

  rspl::TranspileConfig cfg;
  cfg.rspqWrapper = args.rspqWrapper;
  cfg.optimize = args.optimize;
  cfg.reorder = args.reorder;
  cfg.magma = args.magma;
  cfg.optimizeTime = args.optimizeTime;
  cfg.optWorkers = args.optWorkers;
  cfg.sourceDir = sourceDir;

  auto result = rspl::runPipeline(astJson, cfg);

  // Determine output path: explicit -o flag, or derive from input
  std::string outPath = args.outputFile;
  if (outPath.empty() && !args.inputFile.empty()) {
    outPath = args.inputFile;
    // Replace .rspl extension with .S
    if (outPath.ends_with(".rspl"))
      outPath = outPath.substr(0, outPath.size() - 5) + ".S";
    else
      outPath += ".S";
  }

  if (!outPath.empty()) {
    writeFile(outPath, result.asm_);
  }

  std::cerr << "// DMEM: " << result.sizeDMEM
            << " bytes, IMEM: " << result.sizeIMEM << " bytes" << std::endl;

  if (!result.warn.empty())
    std::cerr << result.warn << std::flush;

  return 0;
}
