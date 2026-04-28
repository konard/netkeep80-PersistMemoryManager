@echo off
rem Local build/test entry point. Limits parallelism to 3 jobs by default
rem (issue #371). Override via the PMM_JOBS env var, e.g. `set PMM_JOBS=8`.
if "%PMM_JOBS%"=="" set PMM_JOBS=3

mkdir build
cd build
cmake ..
cmake --build . --config=Release -j %PMM_JOBS%
ctest -C Release -j %PMM_JOBS%
rem ctest -C Release --verbose
