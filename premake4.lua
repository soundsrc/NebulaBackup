solution "NebulaBackup"
	configurations { "Debug", "Release" }
	location(_WORKING_DIR)

	configuration "Debug"
		flags { "Symbols" }
		buildoptions { "-std=c++11", "-stdlib=libc++" }

	configuration "Release"
		flags { "OptimizeSpeed" }
		buildoptions { "-std=c++11", "-stdlib=libc++" }

	project "Nebula"
		language "C++"
		kind "SharedLib"
		includedirs {
			"."
		}
		files { 
			"libnebula/**.h",
			"libnebula/**.cpp" 
		}

	project "NebulaBackup-cli"
		kind "ConsoleApp"
		language "C++"
		includedirs {
			"."
		}
		files {
			"cli/*.h",
			"cli/*.cpp"
		}
