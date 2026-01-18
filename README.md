<<<<<<< Updated upstream
这是一个解析http报文的服务器，采用主从Reactor+线程池管理的架构
##/net  包含了对于socket的操作，创建socket和设置为非阻塞
##/reactor 实现主从reactor的定义 以及实现方法
    **MainReactor:负责监听server_fd，accepr新的连接，之后将client_fd交给SubReactor
    **SubReactor:负责IO事件分发，被MainReactor调用后监听client_fd的读写请求，将任务提交给线程池
##/threadpool 线程池文件，实现读/写，http解析等任务。
=======
这是一个解析http报文的服务器，采用主从Reactor+线程池管理的架构
##/net  包含了对于socket的操作，创建socket和设置为非阻塞
##/reactor 实现主从reactor的定义 以及实现方法
    **MainReactor:负责监听server_fd，accepr新的连接，之后将client_fd交给SubReactor
    **SubReactor:负责IO事件分发，被MainReactor调用后监听client_fd的读写请求，将任务提交给线程池
##/threadpool 线程池文件，实现读/写，http解析等任务。
>>>>>>> Stashed changes
