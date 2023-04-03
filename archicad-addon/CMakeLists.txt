cmake_minimum_required (VERSION 3.16)

function (SetCompilerOptions target)
	target_compile_features (${target} PUBLIC cxx_std_14)
	target_compile_options (${target} PUBLIC "$<$<CONFIG:Debug>:-DDEBUG>")
	if (WIN32)
		target_compile_options (${target} PUBLIC /W4 /WX
			/Zc:wchar_t-
			/wd4499
		)
	else ()
		target_compile_options (${target} PUBLIC -Wall -Wextra -Werror
			-fvisibility=hidden
			-Wno-multichar
			-Wno-ctor-dtor-privacy
			-Wno-invalid-offsetof
			-Wno-ignored-qualifiers
			-Wno-reorder
			-Wno-overloaded-virtual
			-Wno-unused-parameter
			-Wno-deprecated
			-Wno-unknown-pragmas
			-Wno-missing-braces
			-Wno-unused-private-field
			-Wno-non-c-typedef-for-linkage)
	endif ()
endfunction ()

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_SUPPRESS_REGENERATION 1)
set (CMAKE_CONFIGURATION_TYPES Debug;Release;RelWithDebInfo)
set (AC_API_DEVKIT_DIR $ENV{AC_API_DEVKIT_DIR} CACHE PATH "API DevKit directory.")
set (AC_ADDON_NAME "TapirAddOn" CACHE STRING "Add-On name.")
set (AC_ADDON_LANGUAGE "INT" CACHE STRING "Add-On language code.")

set (ACAPINC_FILE_LOCATION ${AC_API_DEVKIT_DIR}/Support/Inc/ACAPinc.h)
if (EXISTS ${ACAPINC_FILE_LOCATION})
	file (READ ${ACAPINC_FILE_LOCATION} ACAPIncContent)
	string (REGEX MATCHALL "#define[ \t]+ServerMainVers_([0-9][0-9])" VersionList ${ACAPIncContent})
	set (ARCHICAD_VERSION ${CMAKE_MATCH_1})
	message (STATUS "Archicad Version: ${ARCHICAD_VERSION}")
else ()
	message (FATAL_ERROR "Failed to detect Archicad version, please check the value of the AC_API_DEVKIT_DIR variable.")
endif ()

if (WIN32)
	add_definitions (-DUNICODE -D_UNICODE)
else ()
	add_definitions (-Dmacintosh=1)
endif ()
add_definitions (-DACExtension)

if (WIN32)
	set (OutputPostfix Win)
else ()
	set (OutputPostfix Mac)
endif ()

set (OutputName ${AC_ADDON_NAME}_AC${ARCHICAD_VERSION}_${OutputPostfix})
project (${OutputName})

set (AddOnSourcesFolder Sources)

set (ResourceObjectsDir ${CMAKE_BINARY_DIR}/ResourceObjects)
set (ResourceStampFile "${ResourceObjectsDir}/AddOnResources.stamp")

if (WIN32)
	file (GLOB AddOnResourceFiles
		${AddOnSourcesFolder}/R${AC_ADDON_LANGUAGE}/*.grc
		${AddOnSourcesFolder}/RFIX/*.grc
		${AddOnSourcesFolder}/RFIX/*.grc
		${AddOnSourcesFolder}/RFIX.win/*.rc2
	)
else ()
	file (GLOB AddOnResourceFiles
		${AddOnSourcesFolder}/R${AC_ADDON_LANGUAGE}/*.grc
		${AddOnSourcesFolder}/RFIX/*.grc
		${AddOnSourcesFolder}/RFIX.mac/*.plist
	)
endif ()

file (GLOB AddOnHeaderFiles
	${AddOnSourcesFolder}/*.h
	${AddOnSourcesFolder}/*.hpp
)
file (GLOB AddOnSourceFiles
	${AddOnSourcesFolder}/*.c
	${AddOnSourcesFolder}/*.cpp
)
set (
	AddOnFiles
	${AddOnHeaderFiles}
	${AddOnSourceFiles}
	${AddOnResourceFiles}
	${ResourceStampFile}
)

source_group ("Sources" FILES ${AddOnHeaderFiles} ${AddOnSourceFiles})
source_group ("Resources" FILES ${AddOnResourceFiles})

if (WIN32)
	add_library (AddOn SHARED ${AddOnFiles})
else ()
	add_library (AddOn MODULE ${AddOnFiles})
endif ()


get_filename_component (AddOnSourcesFolderAbsolute "${CMAKE_CURRENT_LIST_DIR}/${AddOnSourcesFolder}" ABSOLUTE)
if (WIN32)
	add_custom_command (
		OUTPUT ${ResourceStampFile}
		DEPENDS ${AddOnResourceFiles}
		COMMENT "Compiling resources..."
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ResourceObjectsDir}"
		COMMAND python "${AddOnSourcesFolderAbsolute}/Tools/CompileResources.py" "${AC_ADDON_LANGUAGE}" "${AC_API_DEVKIT_DIR}" "${AddOnSourcesFolderAbsolute}" "${AddOnSourcesFolderAbsolute}" "${ResourceObjectsDir}" "${ResourceObjectsDir}/${OutputName}.res"
		COMMAND ${CMAKE_COMMAND} -E touch ${ResourceStampFile}
	)
else ()
	add_custom_command (
		OUTPUT ${ResourceStampFile}
		DEPENDS ${AddOnResourceFiles}
		COMMENT "Compiling resources..."
		COMMAND ${CMAKE_COMMAND} -E make_directory "${ResourceObjectsDir}"
		COMMAND python3 "${AddOnSourcesFolderAbsolute}/Tools/CompileResources.py" "${AC_ADDON_LANGUAGE}" "${AC_API_DEVKIT_DIR}" "${AddOnSourcesFolderAbsolute}" "${AddOnSourcesFolderAbsolute}" "${ResourceObjectsDir}" "${CMAKE_BINARY_DIR}/$<CONFIG>/${OutputName}.bundle/Contents/Resources"
		COMMAND ${CMAKE_COMMAND} -E copy "${AC_API_DEVKIT_DIR}/Support/Inc/PkgInfo" "${CMAKE_BINARY_DIR}/$<CONFIG>/${OutputName}.bundle/Contents/PkgInfo"
		COMMAND ${CMAKE_COMMAND} -E touch ${ResourceStampFile}
	)
endif ()

set_target_properties (AddOn PROPERTIES OUTPUT_NAME ${OutputName})
if (WIN32)
	set_target_properties (AddOn PROPERTIES SUFFIX ".apx")
else ()
	set_target_properties (AddOn PROPERTIES BUNDLE TRUE)
	set_target_properties (AddOn PROPERTIES MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/${AddOnSourcesFolder}/RFIX.mac/Info.plist")
	set_target_properties (AddOn PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif ()

if (WIN32)
	target_link_options (AddOn PUBLIC "${ResourceObjectsDir}/${OutputName}.res")
	target_link_options (AddOn PUBLIC /export:GetExportedFuncAddrs,@1 /export:SetImportedFuncAddrs,@2)
endif ()

target_include_directories (AddOn PUBLIC
	${AddOnSourcesFolder}
	${AC_API_DEVKIT_DIR}/Support/Inc
)

if (WIN32)
	target_link_libraries (AddOn
		"$<$<CONFIG:Debug>:${AC_API_DEVKIT_DIR}/Support/Lib/Win/ACAP_STATD.lib>"
		"$<$<CONFIG:Release>:${AC_API_DEVKIT_DIR}/Support/Lib/Win/ACAP_STAT.lib>"
		"$<$<CONFIG:RelWithDebInfo>:${AC_API_DEVKIT_DIR}/Support/Lib/Win/ACAP_STAT.lib>"
	)
else ()
	find_library (CocoaFramework Cocoa)
	target_link_libraries (AddOn
		"${AC_API_DEVKIT_DIR}/Support/Lib/Mactel/libACAP_STAT.a"
		${CocoaFramework}
	)
endif ()

SetCompilerOptions (AddOn)
set_source_files_properties (${AddOnSourceFiles} PROPERTIES LANGUAGE CXX)

file (GLOB ModuleFolders ${AC_API_DEVKIT_DIR}/Support/Modules/*)
target_include_directories (AddOn PUBLIC ${ModuleFolders})
if (WIN32)
	file (GLOB LibFilesInFolder ${AC_API_DEVKIT_DIR}/Support/Modules/*/*/*.lib)
	target_link_libraries (AddOn ${LibFilesInFolder})
else ()
	file (GLOB FrameworkFilesInFolder ${AC_API_DEVKIT_DIR}/Support/Frameworks/*.framework)
	target_link_libraries (AddOn ${FrameworkFilesInFolder})
endif ()
