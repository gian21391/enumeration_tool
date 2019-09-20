################################################################################
# API header files
################################################################################
# This lists the API header files that are scanned by
# some of the build rules to generate some files needed
# by the build
set(Z3_API_HEADER_FILES_TO_SCAN
        z3_api.h
        z3_ast_containers.h
        z3_algebraic.h
        z3_polynomial.h
        z3_rcf.h
        z3_fixedpoint.h
        z3_optimization.h
        z3_fpa.h
        z3_spacer.h
        )
set(Z3_FULL_PATH_API_HEADER_FILES_TO_SCAN "")
foreach (header_file ${Z3_API_HEADER_FILES_TO_SCAN})
    set(full_path_api_header_file "${CMAKE_CURRENT_SOURCE_DIR}/api/${header_file}")
    list(APPEND Z3_FULL_PATH_API_HEADER_FILES_TO_SCAN "${full_path_api_header_file}")
    if (NOT EXISTS "${full_path_api_header_file}")
        message(FATAL_ERROR "API header file \"${full_path_api_header_file}\" does not exist")
    endif()
endforeach()

add_subdirectory(util)
add_subdirectory(math/polynomial)
add_subdirectory(sat)
add_subdirectory(nlsat)
add_subdirectory(util/lp)
add_subdirectory(math/hilbert)
add_subdirectory(math/simplex)
add_subdirectory(math/automata)
add_subdirectory(math/interval)
add_subdirectory(math/realclosure)
add_subdirectory(math/subpaving)
add_subdirectory(ast)
add_subdirectory(ast/rewriter)
add_subdirectory(ast/normal_forms)
add_subdirectory(model)
add_subdirectory(tactic)
add_subdirectory(ast/substitution)
add_subdirectory(parsers/util)
add_subdirectory(math/grobner)
add_subdirectory(math/euclid)
add_subdirectory(tactic/core)
add_subdirectory(math/subpaving/tactic)
add_subdirectory(tactic/aig)
add_subdirectory(solver)
add_subdirectory(sat/tactic)
add_subdirectory(tactic/arith)
add_subdirectory(nlsat/tactic)
add_subdirectory(ackermannization)
add_subdirectory(cmd_context)
add_subdirectory(cmd_context/extra_cmds)
add_subdirectory(parsers/smt2)
add_subdirectory(ast/proofs)
add_subdirectory(ast/fpa)
add_subdirectory(ast/macros)
add_subdirectory(ast/pattern)
add_subdirectory(ast/rewriter/bit_blaster)
add_subdirectory(smt/params)
add_subdirectory(smt/proto_model)
add_subdirectory(smt)
add_subdirectory(tactic/bv)
add_subdirectory(smt/tactic)
add_subdirectory(tactic/sls)
add_subdirectory(qe)
add_subdirectory(muz/base)
add_subdirectory(muz/dataflow)
add_subdirectory(muz/transforms)
add_subdirectory(muz/rel)
add_subdirectory(muz/clp)
add_subdirectory(muz/tab)
add_subdirectory(muz/bmc)
add_subdirectory(muz/ddnf)
add_subdirectory(muz/spacer)
add_subdirectory(muz/fp)
add_subdirectory(tactic/ufbv)
add_subdirectory(sat/sat_solver)
add_subdirectory(tactic/smtlogics)
add_subdirectory(tactic/fpa)
add_subdirectory(tactic/fd_solver)
add_subdirectory(tactic/portfolio)
add_subdirectory(opt)
add_subdirectory(api)
add_subdirectory(api/dll)