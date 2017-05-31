add_definitions(" -DAUDIOIMPL_IS_REMOTE -DNAO_ENABLED -DBOOST_ASIO_DISABLE_STD_CHRONO -DBOOST_FILESYSTEM_VERSION=3")

file(GLOB_RECURSE SELF_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
qi_create_lib(self SHARED ${SELF_CPP})

if(APPLE)
	qi_use_lib(self sqlite jsoncpp tinythread++ utils boost tinyxml_static OPENCV2_CORE OPENCV2_HIGHGUI OPENCV2_OBJDETECT OPENCV2_IMGPROC)
else()
	qi_use_lib(self sqlite jsoncpp tinythread++ utils boost tinyxml_static OPENCV2_CORE OPENCV2_HIGHGUI OPENCV2_OBJDETECT OPENCV2_IMGPROC rt)
endif()

qi_stage_lib(self)

file(GLOB_RECURSE SELF_H RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.h")
qi_install_header(${SELF_H} KEEP_RELATIVE_PATHS)

