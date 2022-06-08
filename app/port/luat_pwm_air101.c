
#include "luat_base.h"
#include "luat_pwm.h"

#define LUAT_LOG_TAG "luat.pwm"
#include "luat_log.h"

#include "wm_type_def.h"
#include "wm_cpu.h"
#include "wm_regs.h"
#include "wm_dma.h"
#include "wm_pwm.h"
#include "wm_io.h"
#include "luat_msgbus.h"

uint32_t pwmDmaCap0[10]={0}; 
uint32_t pwmDmaCap4[10]={0};

static luat_pwm_conf_t pwm_confs[5];

int l_pwm_dma_capture(lua_State *L, void* ptr) {
	int pwmH,pwmL,pulse;
    // 给 sys.publish方法发送数据
    rtos_msg_t* msg = (rtos_msg_t*)lua_topointer(L, -1);
    int channel = msg->arg1;
	if (channel ==0){
		pwmH = (int)(pwmDmaCap0[5]>>16);
		pwmL = (int)(pwmDmaCap0[5]&0x0000ffff);
		pulse = pwmH*100/(pwmH+pwmL);
	}else if(channel ==4){
		pwmH = (int)(pwmDmaCap4[5]>>16);
		pwmL = (int)(pwmDmaCap4[5]&0x0000ffff);
		pulse = pwmH*100/(pwmH+pwmL);
	}
	lua_getglobal(L, "sys_pub");
    if (lua_isnil(L, -1)) {
        lua_pushinteger(L, 0);
        return 1;
    }
	lua_pushstring(L, "PWM_CAPTURE");
	lua_pushinteger(L, channel);
	lua_pushinteger(L, pulse);
	lua_pushinteger(L, pwmH);
	lua_pushinteger(L, pwmL);
	lua_call(L, 5, 0);
    return 0;
}

static void pwm_dma_callback(void * channel)
{
	rtos_msg_t msg={0};
	msg.handler = l_pwm_dma_capture;
	msg.arg1 = (int)channel;
	luat_msgbus_put(&msg, 0);
	tls_pwm_stop(channel);
	tls_dma_free(1);
}

int luat_pwm_setup(luat_pwm_conf_t* conf) {
    int channel = conf->channel;
	size_t period = conf->period;
	size_t pulse = conf->pulse;
	size_t pnum = conf->pnum;
	size_t precision = conf->precision;

	tls_sys_clk sysclk;

	if (precision != 100 && precision != 256) {
		LLOGW("only 100 or 256 PWM precision supported");
		return -1;
	}
	if (pulse >= precision)
		pulse = precision;

	if (precision == 100)
		pulse = pulse * 2.55;
	else if (precision == 256) {
		if (pulse > 0)
			pulse --;
	}

    int ret = -1;
    switch (channel)
	{
// #ifdef AIR101
// 		case 0:
// 			wm_pwm0_config(WM_IO_PB_00);
// 			break;
// 		case 1:
// 			wm_pwm1_config(WM_IO_PB_01);
// 			break;
// 		case 2:
// 			wm_pwm2_config(WM_IO_PB_02);
// 			break;
// 		case 3:
// 			wm_pwm3_config(WM_IO_PB_03);
// 			break;
// 		case 4:
// 			wm_pwm4_config(WM_IO_PA_07);
// 			break;
// #else
		case 00:
			wm_pwm0_config(WM_IO_PB_00);
			break;
		case 10:
			wm_pwm0_config(WM_IO_PA_10);
			break;
		case 20:
			wm_pwm0_config(WM_IO_PB_12);
			break;
		case 30:
			wm_pwm0_config(WM_IO_PA_02);
			break;
		case 01:
			wm_pwm1_config(WM_IO_PB_01);
			break;
		case 11:
			wm_pwm1_config(WM_IO_PA_11);
			break;
		case 21:
			wm_pwm1_config(WM_IO_PB_13);
			break;
		case 31:
			wm_pwm1_config(WM_IO_PA_03);
			break;
		case 02:
			wm_pwm2_config(WM_IO_PB_02);
			break;
		case 12:
			wm_pwm2_config(WM_IO_PA_12);
			break;
		case 22:
			wm_pwm2_config(WM_IO_PB_14);
			break;
		case 32:
			wm_pwm2_config(WM_IO_PB_24);
			break;
		case 03:
			wm_pwm3_config(WM_IO_PB_03);
			break;
		case 13:
			wm_pwm3_config(WM_IO_PA_13);
			break;
		case 23:
			wm_pwm3_config(WM_IO_PB_15);
			break;
		case 33:
			wm_pwm3_config(WM_IO_PB_25);
			break;
		case 04:
			wm_pwm4_config(WM_IO_PA_07);
			break;
		case 14:
			wm_pwm4_config(WM_IO_PA_14);
			break;
		case 24:
			wm_pwm4_config(WM_IO_PB_16);
			break;
		case 34:
			wm_pwm4_config(WM_IO_PB_26);
			break;
// #endif
		// TODO 再选一组PWM0~PWM4
		default:
			LLOGW("unkown pwm channel %d", channel);
			return -1;
	}
	channel = channel%10;
	if (channel < 0 || channel > 4)
		return -1;
	if (conf->pulse == 0) {
		return luat_pwm_close(conf->channel);
	}
	
	tls_sys_clk_get(&sysclk);

	// 判断一下是否只修改了占空比
	if (memcmp(&pwm_confs[channel], conf, sizeof(luat_pwm_conf_t))) {
		while (1) {
			if (pwm_confs[channel].period != conf->period) {
				break;
				// TODO 支持只修改频率
				//tls_pwm_freq_config(channel, sysclk.apbclk*UNIT_MHZ/256/period, period);
			}
			if (pwm_confs[channel].pnum != conf->pnum) {
				break;
			}
			if (pwm_confs[channel].precision != conf->precision) {
				break;
			}
			if (pwm_confs[channel].pulse != conf->pulse) {
				// 仅占空比不同,修改即可, V0006
				tls_pwm_duty_config(channel, pulse);
				pwm_confs[channel].pulse = conf->pulse;
				return 0;
			}
			break;
		}
	}
	else {
		// 完全相同, 那不需要重新配置了
		return 0;
	}
	
	// 属于全新配置
	ret = tls_pwm_init(channel, period, pulse, pnum);
    if(ret != WM_SUCCESS)
        return ret;
    tls_pwm_start(channel);
	memcpy(&pwm_confs[channel], conf, sizeof(luat_pwm_conf_t));
    
    return 0;
}

int luat_pwm_capture(int channel,int freq) {
	uint8_t dmaCh;
	struct tls_dma_descriptor DmaDesc;
	tls_sys_clk sysclk;
	tls_sys_clk_get(&sysclk);
    switch (channel){
// #ifdef AIR101
// 		case 0:
// 			memset(pwmDmaCap0, 0, sizeof(pwmDmaCap0)/sizeof(char));
// 			wm_pwm0_config(WM_IO_PB_00);
// 			tls_pwm_stop(channel);
// 			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
// 			DmaDesc.src_addr = HR_PWM_CAPDAT;
// 			DmaDesc.dest_addr = (unsigned int)pwmDmaCap0;
// 			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
// 			DmaDesc.valid = TLS_DMA_DESC_VALID;
// 			DmaDesc.next = NULL;
// 			tls_dma_start(dmaCh, &DmaDesc, 0);
// 			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
// 			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
// 			tls_pwm_start(channel); 
// 			return 0;
// 		case 4:
// 			memset(pwmDmaCap4, 0, sizeof(pwmDmaCap4)/sizeof(char));
// 			wm_pwm4_config(WM_IO_PA_07);
// 			tls_pwm_stop(channel);
// 			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
// 			DmaDesc.src_addr = HR_PWM_CAPDAT;
// 			DmaDesc.dest_addr = (unsigned int)pwmDmaCap4;
// 			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
// 			DmaDesc.valid = TLS_DMA_DESC_VALID;
// 			DmaDesc.next = NULL;
// 			tls_dma_start(dmaCh, &DmaDesc, 0);
// 			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
// 			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
// 			tls_pwm_start(channel); 
// 			return 0;
// #else
		case 00:
			channel = channel%10;
			memset(pwmDmaCap0, 0, sizeof(pwmDmaCap0)/sizeof(char));
			wm_pwm0_config(WM_IO_PB_00);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap0;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
		case 10:
			channel = channel%10;
			memset(pwmDmaCap0, 0, sizeof(pwmDmaCap0)/sizeof(char));
			wm_pwm0_config(WM_IO_PB_19);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap0;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
		case 20:
			memset(pwmDmaCap0, 0, sizeof(pwmDmaCap0)/sizeof(char));
			wm_pwm0_config(WM_IO_PA_02);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap0;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
		case 30:
			channel = channel%10;
			memset(pwmDmaCap0, 0, sizeof(pwmDmaCap0)/sizeof(char));
			wm_pwm0_config(WM_IO_PA_10);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap0;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
		case 40:
			channel = channel%10;
			memset(pwmDmaCap0, 0, sizeof(pwmDmaCap0)/sizeof(char));
			wm_pwm0_config(WM_IO_PB_12);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap0;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
		case 04:
			channel = channel%10;
			memset(pwmDmaCap4, 0, sizeof(pwmDmaCap4)/sizeof(char));
			wm_pwm4_config(WM_IO_PA_04);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap4;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
		case 14:
			channel = channel%10;
			memset(pwmDmaCap4, 0, sizeof(pwmDmaCap4)/sizeof(char));
			wm_pwm4_config(WM_IO_PA_07);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap4;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
		case 24:
			channel = channel%10;
			memset(pwmDmaCap4, 0, sizeof(pwmDmaCap4)/sizeof(char));
			wm_pwm4_config(WM_IO_PA_14);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap4;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
		case 34:
			channel = channel%10;
			memset(pwmDmaCap4, 0, sizeof(pwmDmaCap4)/sizeof(char));
			wm_pwm4_config(WM_IO_PB_16);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap4;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
		case 44:
			channel = channel%10;
			memset(pwmDmaCap4, 0, sizeof(pwmDmaCap4)/sizeof(char));
			wm_pwm4_config(WM_IO_PB_26);
			tls_pwm_stop(channel);
			dmaCh = tls_dma_request(1, TLS_DMA_FLAGS_CHANNEL_SEL(TLS_DMA_SEL_PWM_CAP0) | TLS_DMA_FLAGS_HARD_MODE);
			DmaDesc.src_addr = HR_PWM_CAPDAT;
			DmaDesc.dest_addr = (unsigned int)pwmDmaCap4;
			DmaDesc.dma_ctrl = TLS_DMA_DESC_CTRL_DEST_ADD_INC | TLS_DMA_DESC_CTRL_BURST_SIZE1 | TLS_DMA_DESC_CTRL_DATA_SIZE_WORD | TLS_DMA_DESC_CTRL_TOTAL_BYTES(400);
			DmaDesc.valid = TLS_DMA_DESC_VALID;
			DmaDesc.next = NULL;
			tls_dma_start(dmaCh, &DmaDesc, 0);
			tls_dma_irq_register(dmaCh, pwm_dma_callback, (void*)channel, TLS_DMA_IRQ_TRANSFER_DONE);
			tls_pwm_cap_init(channel, sysclk.apbclk*UNIT_MHZ/256/freq, DISABLE, WM_PWM_CAP_DMA_INT);
			tls_pwm_start(channel); 
			return 0;
// #endif
		// TODO 再选一组PWM0~PWM4
		default:
			break;
	}
    return -1;
}

// @return -1 关闭失败。 0 关闭成功
int luat_pwm_close(int channel) {
    int ret = -1;
	channel = channel%10;
	if (channel < 0 || channel > 4)
		return 0;
    ret = tls_pwm_stop(channel);
	pwm_confs[channel].period = 0;
    if(ret != WM_SUCCESS)
        return ret;
    return 0;
}

