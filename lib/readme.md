## lib编译

全部编译比较耗时，因此将lib部分预编译进行使用，平时仅编译app部分即可，当需要改动lib部分时需要在lib目录下执行xmake -j1 -P即可。**注意：编译lib后一定要将build删掉重新编译app**

**注意：libbtcontroller.a libwlan.a libdsp.a这三个静态库无源码，不要删除**

