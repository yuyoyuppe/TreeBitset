return {
  kind = 'ConsoleApp',
  cppdialect = "C++17",
  -- buildoptions = { "-mbmi -mlzcnt"}, -- for linux
  buildoptions = {"/permissive", "/arch:AVX2"}, -- catch2 can't handle permissive- on Windows :|
}
