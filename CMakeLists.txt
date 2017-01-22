cmake_minimum_required (VERSION 3.0.2)
include(ExternalProject)

project(NebulaBackup)

set (CMAKE_CXX_STANDARD 11)
set (OPENSSL_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libressl")

set (NEBULA_BACKUP_INCLUDES
	"${CMAKE_CURRENT_SOURCE_DIR}"
	"${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libssh"
	"${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libssh/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libressl/include"
	"${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/boost"
	"${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/lzma/C"
	"${CMAKE_BINARY_DIR}/include"
)

link_directories("${CMAKE_BINARY_DIR}/lib")

set (NEBULA_BACKUP_LIBS
	"Nebula"
	"boostfilesystem"
	 "lzma"
	 "ssl"
	 "crypto"
	 "ssh_shared"
	 "aws-cpp-sdk-s3"
)

if(APPLE)
	list(APPEND NEBULA_BACKUP_LIBS
		"resolv"
	)
endif(APPLE)


add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libressl")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libssh")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/googletest")
ExternalProject_Add(aws-sdk-cpp
	SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/aws-sdk-cpp"
	CMAKE_ARGS -DENABLE_TESTING=OFF -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}
	INSTALL_DIR "${CMAKE_BINARY_DIR}"
)

add_library(boostfilesystem
	"thirdparty/boost/libs/filesystem/src/codecvt_error_category.cpp"
	"thirdparty/boost/libs/filesystem/src/codecvt_error_category.cpp"
	"thirdparty/boost/libs/filesystem/src/unique_path.cpp"
	"thirdparty/boost/libs/filesystem/src/operations.cpp"
	"thirdparty/boost/libs/filesystem/src/utf8_codecvt_facet.cpp"
	"thirdparty/boost/libs/filesystem/src/path.cpp"
	"thirdparty/boost/libs/filesystem/src/windows_file_codecvt.cpp"
	"thirdparty/boost/libs/filesystem/src/path_traits.cpp"
	"thirdparty/boost/libs/filesystem/src/windows_file_codecvt.hpp"
	"thirdparty/boost/libs/filesystem/src/portability.cpp"
	"thirdparty/boost/libs/system/src/error_code.cpp"
)
target_include_directories(boostfilesystem PUBLIC
	"thirdparty/boost"
)

add_library(lzma
	"thirdparty/lzma/C/7z.h"
	"thirdparty/lzma/C/7zAlloc.c"
	"thirdparty/lzma/C/7zAlloc.h"
	"thirdparty/lzma/C/7zArcIn.c"
	"thirdparty/lzma/C/7zBuf.c"
	"thirdparty/lzma/C/7zBuf.h"
	"thirdparty/lzma/C/7zBuf2.c"
	"thirdparty/lzma/C/7zCrc.c"
	"thirdparty/lzma/C/7zCrc.h"
	"thirdparty/lzma/C/7zCrcOpt.c"
	"thirdparty/lzma/C/7zDec.c"
	"thirdparty/lzma/C/7zFile.c"
	"thirdparty/lzma/C/7zFile.h"
	"thirdparty/lzma/C/7zStream.c"
	"thirdparty/lzma/C/7zTypes.h"
	"thirdparty/lzma/C/7zVersion.h"
	"thirdparty/lzma/C/7zVersion.rc"
	"thirdparty/lzma/C/Aes.c"
	"thirdparty/lzma/C/Aes.h"
	"thirdparty/lzma/C/AesOpt.c"
	"thirdparty/lzma/C/Alloc.c"
	"thirdparty/lzma/C/Alloc.h"
	"thirdparty/lzma/C/Bcj2.c"
	"thirdparty/lzma/C/Bcj2.h"
	"thirdparty/lzma/C/Bcj2Enc.c"
	"thirdparty/lzma/C/Bra.c"
	"thirdparty/lzma/C/Bra.h"
	"thirdparty/lzma/C/Bra86.c"
	"thirdparty/lzma/C/BraIA64.c"
	"thirdparty/lzma/C/Compiler.h"
	"thirdparty/lzma/C/CpuArch.c"
	"thirdparty/lzma/C/CpuArch.h"
	"thirdparty/lzma/C/Delta.c"
	"thirdparty/lzma/C/Delta.h"
	"thirdparty/lzma/C/DllSecur.c"
	"thirdparty/lzma/C/DllSecur.h"
	"thirdparty/lzma/C/LzFind.c"
	"thirdparty/lzma/C/LzFind.h"
	"thirdparty/lzma/C/LzHash.h"
	"thirdparty/lzma/C/Lzma2Dec.c"
	"thirdparty/lzma/C/Lzma2Dec.h"
	"thirdparty/lzma/C/Lzma2Enc.c"
	"thirdparty/lzma/C/Lzma2Enc.h"
	"thirdparty/lzma/C/Lzma86.h"
	"thirdparty/lzma/C/Lzma86Dec.c"
	"thirdparty/lzma/C/Lzma86Enc.c"
	"thirdparty/lzma/C/LzmaDec.c"
	"thirdparty/lzma/C/LzmaDec.h"
	"thirdparty/lzma/C/LzmaEnc.c"
	"thirdparty/lzma/C/LzmaEnc.h"
	"thirdparty/lzma/C/LzmaLib.c"
	"thirdparty/lzma/C/LzmaLib.h"
	"thirdparty/lzma/C/Ppmd.h"
	"thirdparty/lzma/C/Ppmd7.c"
	"thirdparty/lzma/C/Ppmd7.h"
	"thirdparty/lzma/C/Ppmd7Dec.c"
	"thirdparty/lzma/C/Ppmd7Enc.c"
	"thirdparty/lzma/C/Precomp.h"
	"thirdparty/lzma/C/RotateDefs.h"
	"thirdparty/lzma/C/Sha256.c"
	"thirdparty/lzma/C/Sha256.h"
	"thirdparty/lzma/C/Sort.c"
	"thirdparty/lzma/C/Sort.h"
	"thirdparty/lzma/C/Util"
	"thirdparty/lzma/C/Xz.c"
	"thirdparty/lzma/C/Xz.h"
	"thirdparty/lzma/C/XzCrc64.c"
	"thirdparty/lzma/C/XzCrc64.h"
	"thirdparty/lzma/C/XzCrc64Opt.c"
	"thirdparty/lzma/C/XzDec.c"
	"thirdparty/lzma/C/XzEnc.c"
	"thirdparty/lzma/C/XzEnc.h"
	"thirdparty/lzma/C/XzIn.c"
)
target_compile_definitions(lzma PUBLIC
	"_7ZIP_ST=1"
)
target_include_directories(lzma PUBLIC
	"thirdparty/lzma/C"
)

add_library(Nebula SHARED 
	"libnebula/Base32.cpp"
	"libnebula/Base32.h"
	"libnebula/BufferedInputStream.cpp"
	"libnebula/BufferedInputStream.h"
	"libnebula/CompressionType.h"
	"libnebula/DataStore.h"
	"libnebula/DecryptedInputStream.cpp"
	"libnebula/DecryptedInputStream.h"
	"libnebula/EncryptedOutputStream.cpp"
	"libnebula/EncryptedOutputStream.h"
	"libnebula/Exception.h"
	"libnebula/FileInfo.cpp"
	"libnebula/FileInfo.h"
	"libnebula/FileStream.cpp"
	"libnebula/FileStream.h"
	"libnebula/InputStream.cpp"
	"libnebula/InputStream.h"
	"libnebula/LZMAInputStream.cpp"
	"libnebula/LZMAInputStream.h"
	"libnebula/LZMAUtils.cpp"
	"libnebula/LZMAUtils.h"
	"libnebula/MemoryInputStream.cpp"
	"libnebula/MemoryInputStream.h"
	"libnebula/MemoryOutputStream.cpp"
	"libnebula/MemoryOutputStream.h"
	"libnebula/MultiInputStream.cpp"
	"libnebula/MultiInputStream.h"
	"libnebula/OutputStream.cpp"
	"libnebula/OutputStream.h"
	"libnebula/ProgressFunction.h"
	"libnebula/Repository.cpp"
	"libnebula/Repository.h"
	"libnebula/RollingHash.cpp"
	"libnebula/RollingHash.h"
	"libnebula/Snapshot.cpp"
	"libnebula/Snapshot.h"
	"libnebula/StreamUtils.cpp"
	"libnebula/StreamUtils.h"
	"libnebula/TempFileStream.cpp"
	"libnebula/TempFileStream.h"
	"libnebula/ZeroedAllocator.h"
	"libnebula/ZeroedArray.h"
	"libnebula/ZeroedString.h"
	"libnebula/backends/FileDataStore.cpp"
	"libnebula/backends/FileDataStore.h"
	"libnebula/backends/SSHDataStore.cpp"
	"libnebula/backends/SSHDataStore.h"
)
target_include_directories(Nebula PUBLIC
	${NEBULA_BACKUP_INCLUDES}
)

target_link_libraries(Nebula PUBLIC
	"boostfilesystem"
	"lzma"
	"ssl"
	"crypto"
	"ssh_shared"
)

if (APPLE)
	target_link_libraries(Nebula PUBLIC
		"resolv"
	)
endif(APPLE)

add_executable(NebulaBackup
	"cli/Main.cpp"
)

target_include_directories(NebulaBackup PUBLIC
	${NEBULA_BACKUP_INCLUDES}
)

target_link_libraries(NebulaBackup PUBLIC
	${NEBULA_BACKUP_LIBS}
)

add_dependencies(NebulaBackup aws-sdk-cpp)

add_executable(NebulaBackupTests
	"tests/Base32Tests.cpp"
	"tests/DataStoreTests.cpp"
	"tests/RollingHashTest.cpp"
	"tests/RepositoryTests.cpp"
	"tests/StreamTests.cpp"
)
target_include_directories(NebulaBackupTests PUBLIC
	${NEBULA_BACKUP_INCLUDES}
	"thirdparty/googletest/googletest"
	"thirdparty/googletest/googletest/include"
)
target_link_libraries(NebulaBackupTests PUBLIC
	${NEBULA_BACKUP_LIBS}
	"gtest"
	"gtest_main"
)
