add_python_targets(common
  __init__
  properties
)
dune_add_pybind11_module(NAME _common)
set_property(TARGET _common PROPERTY LINK_LIBRARIES dunecommon dunegrid APPEND)

if(SKBUILD)
  install(TARGETS _common LIBRARY DESTINATION python/dumux/common)
endif()
