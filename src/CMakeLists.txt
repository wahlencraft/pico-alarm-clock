add_executable(alarm-clock)

target_sources(alarm-clock PRIVATE 
  main.c settings.c helpers.c alarm.c node.c timeout_timer.c
  )

target_include_directories(alarm-clock PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(alarm-clock
  PicoTM1637 
  pico_stdlib 
  hardware_pio
  hardware_sleep
  hardware_pwm
  )
