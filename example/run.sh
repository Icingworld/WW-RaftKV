#!/bin/bash

EXECUTABLE=./raft_example

# 检查可执行文件是否存在
if [ ! -f "$EXECUTABLE" ]; then
    echo "Error: $EXECUTABLE not found!"
    exit 1
fi

# 创建日志目录
mkdir -p logs

# 启动 5 个节点（node_id 从 2 到 6）
for id in $(seq 2 6); do
    port=$((4396 + id))
    echo "Starting Raft node $id on port $port..."
    $EXECUTABLE $id > logs/node_$id.log 2>&1 &
    echo "Raft node $id started with PID $!"
done

echo "All 5 Raft nodes started."
