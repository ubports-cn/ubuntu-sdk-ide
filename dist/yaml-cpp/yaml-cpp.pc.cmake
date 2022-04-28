prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
includedir=${prefix}/@INCLUDE_INSTALL_ROOT_DIR@

Name: Yaml-cpp
Description: A YAML parser and emitter for C++
Version: @YAML_CPP_VERSION@
Requires:
Libs: -lyaml-cpp
Cflags: -I${includedir}
