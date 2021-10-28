# Air101/Air103 固件源码库

Air101和Air103是基于平头哥XT804内核设计的soc，使用相同内核的W800和W806同样可以使用本仓库代码。

W806对应Air103，可以看Air103相关教程

## 编译说明
见wiki编译教程[Air101 / Air103 - LuatOS 文档](https://wiki.luatos.com/develop/compile/Air101.html)

## lib编译

全部编译比较耗时，因此将lib部分预编译进行使用，平时仅编译app部分即可，当需要改动lib部分时需要在lib目录下执行xmake -j1 -P即可



