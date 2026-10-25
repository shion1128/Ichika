#!/bin/bash
source /opt/ros/humble/setup.bash
export LD_LIBRARY_PATH=/home/mika/.local/lib/python3.10/site-packages/nvidia/cudnn/lib:/home/mika/.local/lib/python3.10/site-packages/nvidia/cublas/lib:/home/mika/.local/lib/python3.10/site-packages/nvidia/cuda_nvrtc/lib:/usr/local/cuda-12.6/lib64:/home/mika/onnxruntime-linux-x64-gpu-1.20.1/lib:$LD_LIBRARY_PATH
# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/home/inkc/.../Livox-SDK2/sdk_core

# 自动计算多线程
CORES=$(nproc)
AVAILABLE_MEM_GB=$(free -g | awk '/^Mem:/{print $7}')
MEM_LIMITED_JOBS=$((AVAILABLE_MEM_GB / 2))
PARALLEL_JOBS=$((CORES < MEM_LIMITED_JOBS ? CORES : MEM_LIMITED_JOBS))
PARALLEL_JOBS=$((PARALLEL_JOBS < 4 ? 4 : PARALLEL_JOBS))
echo "==> CPU cores: $CORES, Available memory: ${AVAILABLE_MEM_GB}"
echo "==> Building with $PARALLEL_JOBS parallel workers"

# no-warn-unused-cli 这个参数是为了消除未用参数的警告

colcon --log-level error build \
  --symlink-install \
  --parallel-workers $PARALLEL_JOBS \
  --cmake-args \
    -Wno-dev \
    --no-warn-unused-cli \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER_LAUNCHER=ccache \
    -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_CXX_FLAGS="-O3 -pipe -Wno-pragmas -Wno-unknown-pragmas" \
    -DBUILD_TESTING=OFF
    
    
source install/setup.bash
echo "==>🎉 build finished"


# colcon build --symlink-install --cmake-args -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release

# 检查脚本是否被直接执行（而非 source），如果是，则提示用户手动 source
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "提示: 您是在子 shell 中运行此脚本，环境变量更改不会保留到当前终端。"
    echo "请在当前终端手动运行: source install/setup.bash"
fi

# echo "==> 正在为 VS Code 合并全局编译清单 (compile_commands.json)..."
# python3 -c "import json, glob; c=[]; [c.extend(json.load(open(f))) for f in glob.glob('build/*/compile_commands.json')]; open('build/compile_commands.json', 'w').write(json.dumps(c, indent=2))"
# echo "==> 合并完成！VS Code 智能提示已刷新。"

