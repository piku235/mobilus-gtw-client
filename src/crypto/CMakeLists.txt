find_package(OpenSSL REQUIRED)

add_library(crypto OBJECT
  Aes256Encryptor.cpp
  hash.cpp
  utils.cpp
)

target_link_libraries(crypto PRIVATE OpenSSL::Crypto)
