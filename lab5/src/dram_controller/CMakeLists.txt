add_library(ramulator-controller OBJECT)

target_sources(
  ramulator-controller PRIVATE
  controller.h
  scheduler.h
  plugin.h
  refresh.h

  impl/dummy_controller.cpp
  impl/generic_dram_controller.cpp
  impl/atlas_common.h
  impl/bliss_common.h

  impl/scheduler/generic_scheduler.cpp
  impl/scheduler/fcfs.cpp
  impl/scheduler/atlas.cpp
  impl/scheduler/bliss.cpp

  impl/refresh/all_bank_refresh.cpp

  # impl/plugin/trr.cpp
  impl/plugin/trace_recorder.cpp
  impl/plugin/cmd_counter.cpp
  impl/plugin/para.cpp
  impl/plugin/graphene.cpp
  impl/plugin/oracle_rh.cpp
  impl/plugin/twice.cpp
  impl/plugin/hydra.cpp
  impl/plugin/rrs.cpp
  impl/plugin/atlas.cpp
  impl/plugin/bliss.cpp

  )

target_link_libraries(
  ramulator
  PRIVATE
  ramulator-controller
)