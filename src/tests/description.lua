return {
  kind = 'ConsoleApp',
  cppdialect = "C++17",
  buildoptions = {"/permissive", "/arch:AVX2"}, -- catch2 can't handle permissive- on Windows :|
}
