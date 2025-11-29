find_package(PkgConfig REQUIRED)
pkg_check_modules(OPENSSL REQUIRED IMPORTED_TARGET libcrypto)

add_library(crypto OBJECT
  EvpEncryptor.cpp
  hash.cpp
  utils.cpp
)

target_link_libraries(crypto 
  PRIVATE
    common_compiler_warnings
    PkgConfig::OPENSSL
)
