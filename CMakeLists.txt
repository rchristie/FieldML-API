
 # ***** BEGIN LICENSE BLOCK *****
 # Version: MPL 1.1/GPL 2.0/LGPL 2.1
 #
 # The contents of this file are subject to the Mozilla Public License Version
 # 1.1 (the "License"); you may not use this file except in compliance with
 # the License. You may obtain a copy of the License at
 # http://www.mozilla.org/MPL/
 #
 # Software distributed under the License is distributed on an "AS IS" basis,
 # WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 # for the specific language governing rights and limitations under the
 # License.
 #
 # The Original Code is FieldML
 #
 # The Initial Developer of the Original Code is
 # Auckland Uniservices Ltd, Auckland, New Zealand.
 # Portions created by the Initial Developer are Copyright (C) 2011
 # the Initial Developer. All Rights Reserved.
 #
 # Contributor(s): 
 #
 # Alternatively, the contents of this file may be used under the terms of
 # either the GNU General Public License Version 2 or later (the "GPL"), or
 # the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 # in which case the provisions of the GPL or the LGPL are applicable instead
 # of those above. If you wish to allow use of your version of this file only
 # under the terms of either the GPL or the LGPL, and not to allow others to
 # use your version of this file under the terms of the MPL, indicate your
 # decision by deleting the provisions above and replace them with the notice
 # and other provisions required by the GPL or the LGPL. If you do not delete
 # the provisions above, a recipient may use your version of this file under
 # the terms of any one of the MPL, the GPL or the LGPL.
 #
 # ***** END LICENSE BLOCK ***** */
 

# Set the minimum version of cmake required to 2.6
CMAKE_MINIMUM_REQUIRED( VERSION 2.6 )

# Set this nampspace to keep these options seperate from other project options
SET( FIELDML_NAMESPACE_NAME FIELDML )
PROJECT(fieldml)

MACRO( OPTION_WITH_DEFAULT OPTION_NAME OPTION_STRING OPTION_DEFAULT )
	IF( NOT DEFINED ${OPTION_NAME} )
		SET( ${OPTION_NAME} ${OPTION_DEFAULT} )
	ENDIF( NOT DEFINED ${OPTION_NAME} )

	OPTION( ${OPTION_NAME} "${OPTION_STRING}" ${${OPTION_NAME}} )
ENDMACRO( OPTION_WITH_DEFAULT OPTION_NAME OPTION_STRING OPTION_DEFAULT )

MACRO( CACHE_VAR_WITH_DEFAULT OPTION_NAME OPTION_DEFAULT OPTION_TYPE OPTION_STRING )
	IF( NOT DEFINED ${OPTION_NAME} )
		SET( ${OPTION_NAME} ${OPTION_DEFAULT} )
	ENDIF( NOT DEFINED ${OPTION_NAME} )

	SET( ${OPTION_NAME} "${OPTION_DEFAULT}" CACHE ${OPTION_TYPE} "${OPTION_STRING}" )
ENDMACRO( CACHE_VAR_WITH_DEFAULT OPTION_NAME OPTION_DEFAULT OPTION_TYPE OPTION_STRING )

# Set up library options
# Generic library options
SET( LIBRARY_TARGET_NAME fieldml )

STRING( TOUPPER ${LIBRARY_TARGET_NAME} UPPERCASE_LIBRARY_TARGET_NAME )
SET( FIELDML_API_MAJOR_VERSION "0" )
SET( FIELDML_API_MINOR_VERSION "5" )
SET( FIELDML_API_PATCH_VERSION "0" )

CACHE_VAR_WITH_DEFAULT( ${UPPERCASE_LIBRARY_TARGET_NAME}_BUILD_TYPE "Release" STRING 
	"Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel" )
SET( CMAKE_BUILD_TYPE ${${UPPERCASE_LIBRARY_TARGET_NAME}_BUILD_TYPE} CACHE
	INTERNAL "Internalise this variable and manipulate with
	${UPPERCASE_LIBRARY_TARGET_NAME}_BUILD_TYPE" FORCE )
CACHE_VAR_WITH_DEFAULT( ${UPPERCASE_LIBRARY_TARGET_NAME}_INSTALL_PREFIX
	"${CMAKE_INSTALL_PREFIX}" PATH 
	"Install path prefix, prepended onto install directories." )
SET( CMAKE_INSTALL_PREFIX ${${UPPERCASE_LIBRARY_TARGET_NAME}_INSTALL_PREFIX}
	CACHE INTERNAL "Internalise this variable and manipulate with
	${UPPERCASE_LIBRARY_TARGET_NAME}_INSTALL_PREFIX" FORCE )
OPTION_WITH_DEFAULT( ${UPPERCASE_LIBRARY_TARGET_NAME}_BUILD_STATIC_LIB "Build static zlib" TRUE )

OPTION_WITH_DEFAULT( ${UPPERCASE_LIBRARY_TARGET_NAME}_INSTALL_CONFIG "Do you want to install CMake configuration files?" TRUE )

OPTION_WITH_DEFAULT( ${UPPERCASE_LIBRARY_TARGET_NAME}_USE_HDF5 "Do you want to use hdf5?" FALSE )

OPTION_WITH_DEFAULT( ${UPPERCASE_LIBRARY_TARGET_NAME}_BUILD_JAVA "Do you want to build the Java bindings" FALSE )


# Specific library options
OPTION_WITH_DEFAULT( ${UPPERCASE_LIBRARY_TARGET_NAME}_BUILD_TEST "Build ${LIBRARY_TARGET_NAME} test application" FALSE )

# Define library type settings
IF( NOT ${UPPERCASE_LIBRARY_TARGET_NAME}_BUILD_TEST )
        SET( EXCLUDE_TEST EXCLUDE_FROM_ALL )
ENDIF( NOT ${UPPERCASE_LIBRARY_TARGET_NAME}_BUILD_TEST )

# Run ADD_SUBDIRECTORY command
SET( LIBRARY_TARGET_NAME fieldml_api )
SET( FIELDML_API_LIBRARY_TARGET_NAME ${LIBRARY_TARGET_NAME} )
ADD_SUBDIRECTORY( core )
SET( LIBRARY_TARGET_NAME fieldml_io_api )
SET( FIELDML_IO_API_LIBRARY_TARGET_NAME ${LIBRARY_TARGET_NAME} )
ADD_SUBDIRECTORY( io )
GET_DIRECTORY_PROPERTY(HDF5_INCLUDE_DIRS DIRECTORY io DEFINITION HDF5_INCLUDE_DIRS)
GET_DIRECTORY_PROPERTY(HDF5_MINE_LIBRARIES DIRECTORY io DEFINITION HDF5_MINE_LIBRARIES)
GET_DIRECTORY_PROPERTY(MPI_LIBRARIES DIRECTORY io DEFINITION MPI_MINE_LIBRARIES)
GET_DIRECTORY_PROPERTY(MPI_INCLUDE_DIRS DIRECTORY io DEFINITION MPI_INCLUDE_DIRS)
GET_DIRECTORY_PROPERTY(HDF5_USE_MPI DIRECTORY io DEFINITION HDF5_USE_MPI)
ADD_SUBDIRECTORY( test )

foreach(arg ${MPI_LIBRARIES})
	set(MPI_MINE_LIBRARIES "${MPI_MINE_LIBRARIES} ${arg}")
endforeach(arg ${MPI_LIBRARIES})

IF( ${FIELDML_NAMESPACE_NAME}_INSTALL_CONFIG )
	SET( LIBRARY_CONFIG_FILE ${CMAKE_CURRENT_BINARY_DIR}/fieldml-config.cmake )
	SET( CONFIG_FILE_CONTENTS 
		"\nIF( NOT DEFINED _${FIELDML_NAMESPACE_NAME}_CONFIG_CMAKE )"
		"\nSET( _${FIELDML_NAMESPACE_NAME}_CONFIG_CMAKE TRUE )"
		"\nGET_FILENAME_COMPONENT( SELF_DIR \"\${CMAKE_CURRENT_LIST_FILE}\" PATH )"
		"\nINCLUDE( \${SELF_DIR}/fieldml-targets.cmake )"
		"\nGET_FILENAME_COMPONENT( ${FIELDML_NAMESPACE_NAME}_INCLUDE_DIRS \"\${SELF_DIR}/../../include\" ABSOLUTE )"
		"\nSET( ${FIELDML_NAMESPACE_NAME}_INCLUDE_DIRS \"\${${FIELDML_NAMESPACE_NAME}_INCLUDE_DIRS}\" \"${LIBXML2_INCLUDE_DIR}\" \"${HDF5_INCLUDE_DIRS}\" \"${MPI_INCLUDE_DIRS}\" )"
		"\nSET( ${FIELDML_NAMESPACE_NAME}_LIBRARIES ${FIELDML_API_LIBRARY_TARGET_NAME} ${FIELDML_IO_API_LIBRARY_TARGET_NAME} ${HDF5_MINE_LIBRARIES} ${MPI_MINE_LIBRARIES})"
		"\nSET( ${FIELDML_NAMESPACE_NAME}_DEFINITIONS ${LIBXML2_DEFINITIONS} )"
		"\nSET( ${FIELDML_NAMESPACE_NAME}_FOUND TRUE )" 
		"\nENDIF( NOT DEFINED _${FIELDML_NAMESPACE_NAME}_CONFIG_CMAKE )" 
		"\n" )
	FILE( WRITE ${LIBRARY_CONFIG_FILE} ${CONFIG_FILE_CONTENTS} )
	INSTALL( FILES ${LIBRARY_CONFIG_FILE} DESTINATION lib/cmake )
	INSTALL( EXPORT fieldml-targets 
		DESTINATION lib/cmake )
ENDIF( ${FIELDML_NAMESPACE_NAME}_INSTALL_CONFIG )

IF ( FIELDML_BUILD_JAVA )
  FIND_PACKAGE(SWIG REQUIRED)
  FIND_PACKAGE(JNI)
  FIND_PACKAGE(Java)
  INCLUDE_DIRECTORIES(${JNI_INCLUDE_DIRS})
  INCLUDE_DIRECTORIES(io/src)
  INCLUDE_DIRECTORIES(core/src)
  INCLUDE(${SWIG_USE_FILE})

  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

  SET(CMAKE_SWIG_FLAGS "-Iio/src;-Icore/src;-package;fieldml.jni")
  SET(CMAKE_SWIG_OUTDIR "fieldml/jni")

  SET_SOURCE_FILES_PROPERTIES(jni/fieldml_api.i PROPERTIES CPLUSPLUS ON)
  SET_SOURCE_FILES_PROPERTIES(jni/fieldml_io.i PROPERTIES CPLUSPLUS ON)

  # If you change these version, also change the .i files.
  SWIG_ADD_MODULE(fieldml_jni_0.4 java jni/fieldml_api.i)
  SWIG_LINK_LIBRARIES(fieldml_jni_0.4 fieldml_api fieldml_io_api)

  # Also make the jar file...
  SET(javafiles "fieldml/jni/FieldmlApiConstants.java;fieldml/jni/FieldmlApi.java;fieldml/jni/FieldmlApiJNI.java;fieldml/jni/FieldmlDataDescriptionType.java;fieldml/jni/FieldmlDataResourceType.java;fieldml/jni/FieldmlDataSourceType.java;fieldml/jni/FieldmlEnsembleMembersType.java;fieldml/jni/FieldmlHandleType.java;fieldml/jni/FieldmlIoConstants.java;fieldml/jni/FieldmlIo.java;fieldml/jni/FieldmlIoJNI.java;fieldml/jni/SWIGTYPE_p_FmlBoolean.java;fieldml/jni/SWIGTYPE_p_FmlObjectHandle.java;fieldml/jni/SWIGTYPE_p_FmlSessionHandle.java;fieldml/jni/SWIGTYPE_p_int32_t.java")
  SET(classfiles "fieldml/jni/FieldmlApi.class;fieldml/jni/FieldmlApiConstants.class;fieldml/jni/FieldmlApiJNI.class;fieldml/jni/FieldmlDataDescriptionType.class;fieldml/jni/FieldmlDataDescriptionType\\$$SwigNext.class;fieldml/jni/FieldmlDataResourceType.class;fieldml/jni/FieldmlDataResourceType\\$$SwigNext.class;fieldml/jni/FieldmlDataSourceType.class;fieldml/jni/FieldmlDataSourceType\\$$SwigNext.class;fieldml/jni/FieldmlEnsembleMembersType.class;fieldml/jni/FieldmlEnsembleMembersType\\$$SwigNext.class;fieldml/jni/FieldmlHandleType.class;fieldml/jni/FieldmlHandleType\\$$SwigNext.class;fieldml/jni/FieldmlIo.class;fieldml/jni/FieldmlIoConstants.class;fieldml/jni/FieldmlIoJNI.class;fieldml/jni/SWIGTYPE_p_FmlBoolean.class;fieldml/jni/SWIGTYPE_p_FmlObjectHandle.class;fieldml/jni/SWIGTYPE_p_FmlSessionHandle.class;fieldml/jni/SWIGTYPE_p_int32_t.class")

  ADD_CUSTOM_COMMAND(OUTPUT ${classfiles}
                     COMMAND ${Java_JAVAC_EXECUTABLE} ARGS ${javafiles}
                     DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/jni/fieldml_apiJAVA_wrap.cxx)
  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/jni/fieldml.jar
                     COMMAND ${Java_JAR_EXECUTABLE} ARGS cf ${CMAKE_CURRENT_BINARY_DIR}/jni/fieldml.jar ${classfiles}
                     DEPENDS ${classfiles})
  ADD_CUSTOM_TARGET(jnijar ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/jni/fieldml.jar)

ENDIF ( FIELDML_BUILD_JAVA )
