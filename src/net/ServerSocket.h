#pragma once

//创建socket
int setupServerSocket(int port);

//设置fd为非阻塞
int setnonblocking(int fd);