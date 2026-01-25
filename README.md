# C++ 高并发 HTTP Server

## 项目简介
这是一个基于 **Reactor 模型 + 多线程线程池** 的高并发 HTTP 服务器，支持 **ET（Edge Triggered）非阻塞 I/O**，能够轻松处理 **上万并发连接**。  

核心特点：
- **MainReactor / SubReactor 架构**  
  - `MainReactor` 负责监听端口并 accept 新连接  
  - `SubReactor` 负责客户端事件处理（读写）  
- **非阻塞 I/O + epoll ET 模式**  
  - 高效处理大量并发客户端请求  
- **线程池任务处理**  
  - 每个客户端请求由 `ClientTask` 封装，提交到线程池处理  
  - 避免阻塞 Reactor，充分利用多核 CPU  
---

## 项目结构
.
├── bin # 编译生成文件
├── build
├── config
├── src
│   ├── http
│   ├── log
│   ├── net # 网络封装（ServerSocket 等）
│   ├── reactor # MainReactor / SubReactor
│   ├── Task # 客户端任务封装（ClientTask）
│   ├── threadpool # 线程池
│   ├── timer
│   └── util
├── test # 测试脚本
└── www

核心模块说明:
| 模块            | 功能                                         
| -------------- | ------------------------------------------ 
| `MainReactor`  | 监听端口，accept 新连接，Round-Robin 分发到 SubReactor 
| `SubReactor`   | epoll 事件循环，处理客户端可读可写事件，提交 Task             
| `ThreadPool`   | 管理固定数量线程，执行 ClientTask，减少线程创建销毁开销          
| `ClientTask`   | 封装客户端请求处理逻辑，调用 recv 读取数据                   
| `ServerSocket` | 封装 socket 创建、bind、listen、非阻塞设置             
| `Task`         | 抽象基类，支持线程池调度                               
| `test`         | 并发压力测试脚本，支持 1w 条连接测试               


