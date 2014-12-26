set(fourier_SOURCES
  main.cpp
  )

add_executable(fourier ${fourier_SOURCES})
target_link_libraries(fourier allegro allegro_primitives m)
