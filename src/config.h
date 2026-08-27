#ifndef CONFIG_H_
#define CONFIG_H_

#include "hal_data.h"

/* ======================================================================
 *  系统配置
 * ====================================================================== */

/* ----------------------------------------------------------------------
 *  LATCH 电源控制引脚
 *  P301 作为 LATCH 信号：
 *    拉高 (HIGH) = 开机
 *    拉低 (LOW)  = 关机
 * ---------------------------------------------------------------------- */
#define LATCH_PIN            (BSP_IO_PORT_03_PIN_01)                                /* P301 */
#define LATCH_PIN_INIT_CFG   (IOPORT_CFG_PORT_DIRECTION_OUTPUT | IOPORT_CFG_PORT_OUTPUT_LOW) /* 初始输出低电平 */
#define LATCH_LEVEL_ON       (BSP_IO_LEVEL_HIGH)                                     /* 开机电平 */
#define LATCH_LEVEL_OFF      (BSP_IO_LEVEL_LOW)                                      /* 关机电平 */

/* ----------------------------------------------------------------------
 *  电源时序引脚 (EN 7V -> EN 5V)
 *  上电时序: 先拉高 EN 7V, 延时 10ms, 再拉高 EN 5V
 *  (两引脚在 pin_data.c 中已初始化为输出低)
 * ---------------------------------------------------------------------- */
#define EN_7V_PIN            (BSP_IO_PORT_09_PIN_14)                                  /* P914 EN 7V */
#define EN_5V_PIN            (BSP_IO_PORT_09_PIN_15)                                  /* P915 EN 5V */
#define PWR_SEQ_DELAY_MS     (10U)                                                    /* EN 7V -> EN 5V 延时 */

#endif /* CONFIG_H_ */
