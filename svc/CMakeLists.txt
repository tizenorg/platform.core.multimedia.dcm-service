SET(fw_name "dcm-service")
SET(DCM_SERVER "dcm-svc")

INCLUDE(FindPkgConfig)
pkg_check_modules(${DCM_SERVER} REQUIRED dlog glib-2.0 gthread-2.0 libmedia-utils libexif libtzplatform-config sqlite3)

FOREACH(flag ${${DCM_SERVER}_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIE -Wall")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--hash-style=both -Wl,-Bsymbolic")

aux_source_directory(. sources)
FOREACH(src ${sources})
    GET_FILENAME_COMPONENT(src_name ${src} NAME_WE)
    MESSAGE("${src_name}")
    ADD_EXECUTABLE(${DCM_SERVER} ${src})
    TARGET_LINK_LIBRARIES(${DCM_SERVER} ${fw_name} ${${DCM_SERVER}_LDFLAGS})
ENDFOREACH()

INSTALL(TARGETS ${DCM_SERVER} DESTINATION bin)
