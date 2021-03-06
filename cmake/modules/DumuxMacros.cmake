# additional macros
include(AddGstatFileLinks)
include(AddInputFileLinks)
include(DumuxDoxygen)
include(DumuxTestMacros)

find_package(GLPK QUIET)
find_package(Gnuplot QUIET)
set(HAVE_GNUPLOT ${GNUPLOT_FOUND})
find_package(Gstat QUIET)
find_package(Gmsh QUIET)
find_package(NLOPT QUIET)
find_package(PTScotch QUIET)
include(AddPTScotchFlags)
find_package(PVPython QUIET)
