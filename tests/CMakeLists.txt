include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    libtest 
    src/find_mutual_nns.cpp
    src/RobustAverage.cpp
    src/correct_target.cpp
    src/AutomaticOrder.cpp
    src/CustomOrder.cpp
    src/restore_order.cpp
    src/utils.cpp
    src/MnnCorrect.cpp
)

target_link_libraries(
    libtest
    gtest_main
    mnncorrect 
)

set(CODE_COVERAGE "Enable coverage testing" OFF)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(libtest PRIVATE -O0 -g --coverage)
    target_link_options(libtest PRIVATE --coverage)
endif()

set(USE_OPENMP OFF CACHE BOOL "Compile with OpenMP support")
set(USE_CUSTOM_PARALLEL OFF CACHE BOOL "Compile with custom parallelization")

if (USE_OPENMP)
    find_package(OpenMP)
    target_link_libraries(libtest OpenMP::OpenMP_CXX)
elseif(USE_CUSTOM_PARALLEL)
    target_compile_definitions(libtest PRIVATE TEST_CUSTOM_PARALLEL)
endif()

include(GoogleTest)
gtest_discover_tests(libtest)
