target_sources(
  coastline_common_libultra_gu
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/gu/lookat.c"
          "${CMAKE_CURRENT_LIST_DIR}/gu/lookathil.c"
          "${CMAKE_CURRENT_LIST_DIR}/gu/lookatref.c"
          "${CMAKE_CURRENT_LIST_DIR}/gu/mtxcatf.c"
          "${CMAKE_CURRENT_LIST_DIR}/gu/mtxutil.c"
          "${CMAKE_CURRENT_LIST_DIR}/gu/perspective.c"
          "${CMAKE_CURRENT_LIST_DIR}/gu/rotate.c"
          "${CMAKE_CURRENT_LIST_DIR}/gu/scale.c"
          "${CMAKE_CURRENT_LIST_DIR}/gu/translate.c")
target_sources(coastline_common_libultra_io PRIVATE "${CMAKE_CURRENT_LIST_DIR}/io/contreaddata.c"
                                                  "${CMAKE_CURRENT_LIST_DIR}/io/controller.c")
target_sources(coastline_common_libultra_os PRIVATE "${CMAKE_CURRENT_LIST_DIR}/os/recvmesg.c"
                                                  "${CMAKE_CURRENT_LIST_DIR}/os/sendmesg.c")
