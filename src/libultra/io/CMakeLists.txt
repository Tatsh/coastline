# The input and output managers: audio, the cartridge and disk interfaces, the controllers and
# their Paks, the serial interface, the signal processor, and video.
if(NOT TGR_PLATFORM STREQUAL "n64")
  # The reading of the controllers the PC port retained.
  add_library(coastline_libultra_io STATIC contreaddata.c controller.c)
  target_link_libraries(coastline_libultra_io PRIVATE coastline_options)
  return()
endif()
add_library(
  coastline_libultra_io STATIC
          ai.c
          cartrominit.c
          contpfs.c
          contramread.c
          contramwrite.c
          contreaddata.c
          controller.c
          crc.c
          devmgr.c
          epirawdma.c
          epirawread.c
          epirawwrite.c
          leodiskinit.c
          leointerrupt.c
          motor.c
          pfsallocatefile.c
          pfschecker.c
          pfsdeletefile.c
          pfsfilestate.c
          pfsfindfile.c
          pfsfreeblocks.c
          pfsgetstatus.c
          pfsinit.c
          pfsinitpak.c
          pfsisplug.c
          pfsnumfiles.c
          pfsreadwritefile.c
          pfsrepairid.c
          piacs.c
          pidma.c
          pigetcmdq.c
          pimgr.c
          pirawdma.c
          pirawread.c
          si.c
          sp.c
          sptask.c
          vi.c
          viblack.c
          vigetcurrcontext.c
          vigetcurrframebuf.c
          vimgr.c
          visetevent.c
          visetmode.c
          visetspecial.c
          viswapbuf.c
          viswapcontext.c)
# The Japanese and European releases save to an EEPROM in the cartridge.
if(TGR_REGION STREQUAL "jp" OR TGR_REGION STREQUAL "eu")
  target_sources(coastline_libultra_io PRIVATE eepromlongread.c eepromlongwrite.c eepromprobe.c
                                               eepromread.c eepromwrite.c)
endif()
target_link_libraries(coastline_libultra_io PRIVATE coastline_libultra_options)
