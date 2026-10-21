# The graphics utilities: matrices, views, and the trigonometry they use.
target_sources(
  coastline_libultra
  PRIVATE cosf.c
          lookat.c
          lookathil.c
          lookatref.c
          mtxcatf.c
          mtxcatl.c
          mtxutil.c
          normalize.c
          perspective.c
          rotate.c
          scale.c
          sinf.c
          sins.c
          sqrtf.c
          translate.c)
