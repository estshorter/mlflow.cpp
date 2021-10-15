cmake_minimum_required(VERSION 3.10)

project(mlflow_cpp)

option(DOWNLOAD_DEPENDENCIES "download dependencies" OFF)

if(DOWNLOAD_DEPENDENCIES)
	include(ExternalProject)
	ExternalProject_Add (json
	  URL https://github.com/nlohmann/json/releases/download/v3.10.3/include.zip
	  PREFIX ${CMAKE_CURRENT_BINARY_DIR}/json
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ""
      INSTALL_COMMAND ""
      LOG_DOWNLOAD ON
      LOG_INSTALL ON
	)
	
	include_directories(./include/cpp-httplib ${CMAKE_CURRENT_BINARY_DIR}/json/src/json/include ./)
endif()

add_subdirectory(examples)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "examples")
