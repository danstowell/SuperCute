# - Find sndfile
# Find the native sndfile includes and libraries
#
#  SNDFILE_INCLUDE_DIR - where to find sndfile.h, etc.
#  SNDFILE_LIBRARIES   - List of libraries when using libsndfile.
#  SNDFILE_FOUND       - True if libsndfile found.

if(SNDFILE_INCLUDE_DIR)
    # Already in cache, be silent
    set(SNDFILE_FIND_QUIETLY TRUE)
endif(SNDFILE_INCLUDE_DIR)

if (APPLE)
	set(SNDFILE_FOUND TRUE)
	set(SNDFILE_INCLUDE_DIR ../include/libsndfile)
	set(SNDFILE_LIBRARIES ${CMAKE_SOURCE_DIR}/mac/lib/scUBlibsndfile.a)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${CMAKE_SOURCE_DIR}/common/include/libsndfile")

else()
	find_path(SNDFILE_INCLUDE_DIR sndfile.h)

	find_library(SNDFILE_LIBRARY NAMES sndfile)

	# Handle the QUIETLY and REQUIRED arguments and set SNDFILE_FOUND to TRUE if
	# all listed variables are TRUE.
	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(SNDFILE DEFAULT_MSG
		SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY)

	if(SNDFILE_FOUND)
		set(SNDFILE_LIBRARIES ${SNDFILE_LIBRARY})
	else(SNDFILE_FOUND)
		set(SNDFILE_LIBRARIES)
	endif(SNDFILE_FOUND)

	mark_as_advanced(SNDFILE_INCLUDE_DIR SNDFILE_LIBRARY)
endif()