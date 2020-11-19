ps = require 'deps.premake_scaffold'
workspace "tree_bitset"
ps.generate({ paths = { 
  ClangFormatExecutable = "S:\\VS2019\\VC\\Tools\\Llvm\\bin\\clang-format.exe",
} })
