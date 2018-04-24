set(libui_INCLUDE_DIRS)
set(libui_LIBRARIES)

find_path(libui_INCLUDE_DIR ui.h)
find_library(libui_LIBRARY libui)

if(libui_INCLUDE_DIR AND libui_LIBRARY)
    set(libui_FOUND TRUE)
    list(APPEND libui_INCLUDE_DIRS "${libui_INCLUDE_DIR}")
    list(APPEND libui_LIBRARIES "${libui_LIBRARY}")
endif()

list(REMOVE_DUPLICATES libui_INCLUDE_DIRS)
list(REMOVE_DUPLICATES libui_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libui
  FOUND_VAR libui_FOUND
  REQUIRED_VARS libui_INCLUDE_DIRS libui_LIBRARIES)
