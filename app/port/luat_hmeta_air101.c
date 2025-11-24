
#include "luat_base.h"
#include "luat_hmeta.h"

int luat_hmeta_model_name(char* buff) {
    #ifdef AIR101
    strcpy(buff, "Air101");
    #elif defined(AIR103)
    strcpy(buff, "Air103");
    #elif defined(AIR601)
    strcpy(buff, "Air601");
    #else
    strcpy(buff, "XT806");
    #endif
    return 0;
}
// 获取硬件版本号, 例如A11, A14
int luat_hmeta_hwversion(char* buff) {
    strcpy(buff, "A10");
    return 0;
}

// 获取芯片组型号, 原始型号, 传入的buff最少要8字节空间
int luat_hmeta_chip(char* buff) {
    strcpy(buff, "XT806");
    return 0;
}
