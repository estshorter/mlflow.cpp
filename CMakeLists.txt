cmake_minimum_required(VERSION 3.14)

project(mlflow_cpp)

option(DOWNLOAD_DEPENDENCIES "download dependencies" OFF)

if(DOWNLOAD_DEPENDENCIES)
	include(FetchContent)
	FetchContent_Declare(
		json
		URL https://github.com/nlohmann/json/releases/download/v3.10.4/include.zip
		URL_HASH SHA256=62c585468054e2d8e7c2759c0d990fd339d13be988577699366fe195162d16cb 
	  )
	FetchContent_MakeAvailable(json)
	include_directories(./include/cpp-httplib ./ ${json_SOURCE_DIR}/include)
endif()

add_subdirectory(examples)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "examples")
