cmake_minimum_required(VERSION 3.14)

project(mlflow_cpp)

option(DOWNLOAD_DEPENDENCIES "download dependencies" OFF)

if(DOWNLOAD_DEPENDENCIES)
	include(FetchContent)
	FetchContent_Declare(
		json
		URL https://github.com/nlohmann/json/releases/download/v3.10.3/include.zip
		URL_HASH SHA256=4ae5744bc1edd216c79f619fd49915c0e490e41b05434c2d2b89e078299f04ed 
	  )
	FetchContent_MakeAvailable(json)
	include_directories(./include/cpp-httplib ./ ${json_SOURCE_DIR}/include)
endif()

add_subdirectory(examples)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "examples")
