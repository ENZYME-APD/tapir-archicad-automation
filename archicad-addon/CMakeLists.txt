cmake_minimum_required (VERSION 3.17)

include (Tools/CMakeCommon.cmake)

set_property (GLOBAL PROPERTY USE_FOLDERS ON)

set (CMAKE_CONFIGURATION_TYPES Debug;Release;RelWithDebInfo)
set (AC_API_DEVKIT_DIR ${AC_API_DEVKIT_DIR} CACHE PATH "API DevKit directory.")
set (AC_VERSION ${API_AC_VERSION} CACHE STRING "Archicad version.")

SetGlobalCompilerDefinitions (${AC_VERSION})

if (WIN32)
	set (NamePostFix Win)
else ()
	set (NamePostFix Mac)
endif ()
set (AddOnName "TapirAddOn_AC${AC_VERSION}_${NamePostFix}")

project (${AddOnName})

set (AddOnSourcesFolder Sources)
set (AddOnResourcesFolder Sources)

# The language of the user interface. As in Graphisoft's DevKit, one build has one language: its
# texts are compiled from Sources/R<language>/AddOn.grc (INT, international English, is the
# complete one the releases are built with). See Sources/LOCALIZATION.md.
set (AC_ADDON_LANGUAGE "INT" CACHE STRING "Add-On language code, the R<code> resource folder in Sources.")
if (NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/${AddOnResourcesFolder}/R${AC_ADDON_LANGUAGE}/AddOn.grc")
	message (FATAL_ERROR "AC_ADDON_LANGUAGE ${AC_ADDON_LANGUAGE}: ${AddOnResourcesFolder}/R${AC_ADDON_LANGUAGE}/AddOn.grc does not exist.")
endif ()

GenerateAddOnProject (AddOn ${AC_VERSION} ${AC_API_DEVKIT_DIR} ${AddOnName} ${AddOnSourcesFolder} ${AddOnResourcesFolder} ${AC_ADDON_LANGUAGE})
