project(osg_graph_viz)
set(PROJECT_VERSION 1.0)
set(PROJECT_DESCRIPTION "")
cmake_minimum_required(VERSION 2.6)
include(FindPkgConfig)

find_package(lib_manager)
lib_defaults()
define_module_info()

add_definitions(-DOSG_GRAPH_VIZ_DEFAULT_RESOURCES_PATH=\"${CMAKE_INSTALL_PREFIX}/share/osg_graph_viz\")

find_package(OpenSceneGraph REQUIRED osgManipulator osgViewer osgFX osgShadow osgParticle osgTerrain osgDB osgGA osgWidget osgText osgUtil)
include_directories(${OPENSCENEGRAPH_INCLUDE_DIRS})
link_directories(${OPENSCENEGRAPH_LIBRARY_DIRS})

pkg_check_modules(PKGCONFIG REQUIRED
          osg_text
          configmaps
          osg_material_manager
          mars_utils
)

include_directories(${PKGCONFIG_INCLUDE_DIRS})
link_directories(${PKGCONFIG_LIBRARY_DIRS})
add_definitions(${PKGCONFIG_CFLAGS_OTHER})  #flags excluding the ones with -I

include_directories(src)

set(SOURCES 
  src/View.cpp
  src/Node.cpp
  src/Edge.cpp
  src/XRockNode.cpp
  src/RoundBodyNode.cpp
)

set(HEADERS
  src/View.hpp
  src/Node.hpp
  src/Edge.hpp
  src/XRockNode.hpp
  src/UpdateInterface.hpp
  src/RoundBodyNode.hpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME}
                      ${OPENSCENEGRAPH_LIBRARIES}
                      ${PKGCONFIG_LIBRARIES}
)

if(WIN32)
  set(LIB_INSTALL_DIR bin) # .dll are in PATH, like executables
else(WIN32)
  set(LIB_INSTALL_DIR lib)
endif(WIN32)


set(_INSTALL_DESTINATIONS
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	ARCHIVE DESTINATION lib
)


# Install the library into the lib folder
install(TARGETS ${PROJECT_NAME} ${_INSTALL_DESTINATIONS})

# Install headers into mars include directory
install(FILES ${HEADERS} DESTINATION include/${PROJECT_NAME})

# Prepare and install necessary files to support finding of the library 
# using pkg-config
configure_file(${PROJECT_NAME}.pc.in ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.pc DESTINATION lib/pkgconfig)
install(DIRECTORY images DESTINATION share/osg_graph_viz)
install(DIRECTORY fonts DESTINATION share/osg_graph_viz)
install(DIRECTORY shader DESTINATION share/osg_graph_viz)

