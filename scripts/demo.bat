@echo off
rem Local demo build entry point. Limits parallelism to 3 jobs by default
rem (issue #371). Override via the PMM_JOBS env var, e.g. `set PMM_JOBS=8`.
if "%PMM_JOBS%"=="" set PMM_JOBS=3

mkdir build
cd build
cmake .. -DPMM_BUILD_DEMO=ON
cmake --build . --config=Release --target pmm_demo -j %PMM_JOBS%
cd ..
build\demo\Release\pmm_demo.exe
