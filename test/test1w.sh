#!/bin/bash

# 服务器 IP 和端口
SERVER_IP="127.0.0.1"
SERVER_PORT=10888

# 并发总数
TOTAL=10000

# 进度计数
COUNT=0

echo "Starting $TOTAL concurrent connections to $SERVER_IP:$SERVER_PORT ..."

for i in $(seq 1 $TOTAL); do
    (
        # 发送数据到服务器
        echo "hello from $i" | nc -w1 $SERVER_IP $SERVER_PORT
    ) &

    # 进度统计
    COUNT=$((COUNT+1))
    if (( COUNT % 1000 == 0 )); then
        echo "Sent $COUNT / $TOTAL"
    fi
done

# 等待所有后台进程完成
wait

echo "All $TOTAL connections sent."
