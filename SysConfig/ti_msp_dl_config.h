/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the LP_MSPM0G3507
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_LP_MSPM0G3507
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     64000000
/* Defines for SYSPLL_ERR_01 Workaround */
/* Represent 1.000 as 1000 */
#define FLOAT_TO_INT_SCALE                                               (1000U)
#define FCC_EXPECTED_RATIO                                                  2000
#define FCC_UPPER_BOUND                       (FCC_EXPECTED_RATIO * (1 + 0.003))
#define FCC_LOWER_BOUND                       (FCC_EXPECTED_RATIO * (1 - 0.003))

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);


/* Defines for PWM_0 */
#define PWM_0_INST                                                         TIMA0
#define PWM_0_INST_IRQHandler                                   TIMA0_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMA0_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                             64000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOA
#define GPIO_PWM_0_C0_PIN                                          DL_GPIO_PIN_0
#define GPIO_PWM_0_C0_IOMUX                                       (IOMUX_PINCM1)
#define GPIO_PWM_0_C0_IOMUX_FUNC                      IOMUX_PINCM1_PF_TIMA0_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOA
#define GPIO_PWM_0_C1_PIN                                          DL_GPIO_PIN_1
#define GPIO_PWM_0_C1_IOMUX                                       (IOMUX_PINCM2)
#define GPIO_PWM_0_C1_IOMUX_FUNC                      IOMUX_PINCM2_PF_TIMA0_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX




/* Defines for I2C_1 */
#define I2C_1_INST                                                          I2C1
#define I2C_1_INST_IRQHandler                                    I2C1_IRQHandler
#define I2C_1_INST_INT_IRQN                                        I2C1_INT_IRQn
#define I2C_1_BUS_SPEED_HZ                                                100000
#define GPIO_I2C_1_SDA_PORT                                                GPIOA
#define GPIO_I2C_1_SDA_PIN                                        DL_GPIO_PIN_16
#define GPIO_I2C_1_IOMUX_SDA                                     (IOMUX_PINCM38)
#define GPIO_I2C_1_IOMUX_SDA_FUNC                      IOMUX_PINCM38_PF_I2C1_SDA
#define GPIO_I2C_1_SCL_PORT                                                GPIOA
#define GPIO_I2C_1_SCL_PIN                                        DL_GPIO_PIN_17
#define GPIO_I2C_1_IOMUX_SCL                                     (IOMUX_PINCM39)
#define GPIO_I2C_1_IOMUX_SCL_FUNC                      IOMUX_PINCM39_PF_I2C1_SCL

/* Defines for I2C_0 */
#define I2C_0_INST                                                          I2C0
#define I2C_0_INST_IRQHandler                                    I2C0_IRQHandler
#define I2C_0_INST_INT_IRQN                                        I2C0_INT_IRQn
#define I2C_0_BUS_SPEED_HZ                                                100000
#define GPIO_I2C_0_SDA_PORT                                                GPIOA
#define GPIO_I2C_0_SDA_PIN                                        DL_GPIO_PIN_10
#define GPIO_I2C_0_IOMUX_SDA                                     (IOMUX_PINCM21)
#define GPIO_I2C_0_IOMUX_SDA_FUNC                      IOMUX_PINCM21_PF_I2C0_SDA
#define GPIO_I2C_0_SCL_PORT                                                GPIOA
#define GPIO_I2C_0_SCL_PIN                                        DL_GPIO_PIN_11
#define GPIO_I2C_0_IOMUX_SCL                                     (IOMUX_PINCM22)
#define GPIO_I2C_0_IOMUX_SCL_FUNC                      IOMUX_PINCM22_PF_I2C0_SCL


/* Defines for UART_0 */
#define UART_0_INST                                                        UART2
#define UART_0_INST_FREQUENCY                                           32000000
#define UART_0_INST_IRQHandler                                  UART2_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART2_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_22
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_21
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM47)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM46)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM47_PF_UART2_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM46_PF_UART2_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_0_FBRD_32_MHZ_115200_BAUD                                      (23)
/* Defines for UART_1 */
#define UART_1_INST                                                        UART3
#define UART_1_INST_FREQUENCY                                           64000000
#define UART_1_INST_IRQHandler                                  UART3_IRQHandler
#define UART_1_INST_INT_IRQN                                      UART3_INT_IRQn
#define GPIO_UART_1_RX_PORT                                                GPIOA
#define GPIO_UART_1_TX_PORT                                                GPIOA
#define GPIO_UART_1_RX_PIN                                        DL_GPIO_PIN_13
#define GPIO_UART_1_TX_PIN                                        DL_GPIO_PIN_14
#define GPIO_UART_1_IOMUX_RX                                     (IOMUX_PINCM35)
#define GPIO_UART_1_IOMUX_TX                                     (IOMUX_PINCM36)
#define GPIO_UART_1_IOMUX_RX_FUNC                      IOMUX_PINCM35_PF_UART3_RX
#define GPIO_UART_1_IOMUX_TX_FUNC                      IOMUX_PINCM36_PF_UART3_TX
#define UART_1_BAUD_RATE                                                (115200)
#define UART_1_IBRD_64_MHZ_115200_BAUD                                      (34)
#define UART_1_FBRD_64_MHZ_115200_BAUD                                      (46)





/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOB)

/* Defines for PIN_2: GPIOB.2 with pinCMx 15 on package pin 50 */
#define LED_PIN_2_PIN                                            (DL_GPIO_PIN_2)
#define LED_PIN_2_IOMUX                                          (IOMUX_PINCM15)
/* Port definition for Pin Group IN */
#define IN_PORT                                                          (GPIOA)

/* Defines for LEFT_IN1: GPIOA.31 with pinCMx 6 on package pin 39 */
#define IN_LEFT_IN1_PIN                                         (DL_GPIO_PIN_31)
#define IN_LEFT_IN1_IOMUX                                         (IOMUX_PINCM6)
/* Defines for LEFT_IN2: GPIOA.28 with pinCMx 3 on package pin 35 */
#define IN_LEFT_IN2_PIN                                         (DL_GPIO_PIN_28)
#define IN_LEFT_IN2_IOMUX                                         (IOMUX_PINCM3)
/* Defines for RIGHT_IN1: GPIOA.9 with pinCMx 20 on package pin 55 */
#define IN_RIGHT_IN1_PIN                                         (DL_GPIO_PIN_9)
#define IN_RIGHT_IN1_IOMUX                                       (IOMUX_PINCM20)
/* Defines for RIGHT_IN2: GPIOA.8 with pinCMx 19 on package pin 54 */
#define IN_RIGHT_IN2_PIN                                         (DL_GPIO_PIN_8)
#define IN_RIGHT_IN2_IOMUX                                       (IOMUX_PINCM19)
/* Defines for LEFT_ENA: GPIOA.2 with pinCMx 7 on package pin 42 */
#define ENC_LEFT_ENA_PORT                                                (GPIOA)
// groups represented: ["KEY","ENC"]
// pins affected: ["KEY_0","KEY_1","LEFT_ENA","LEFT_ENB"]
#define GPIO_MULTIPLE_GPIOA_INT_IRQN                            (GPIOA_INT_IRQn)
#define GPIO_MULTIPLE_GPIOA_INT_IIDX            (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define ENC_LEFT_ENA_IIDX                                    (DL_GPIO_IIDX_DIO2)
#define ENC_LEFT_ENA_PIN                                         (DL_GPIO_PIN_2)
#define ENC_LEFT_ENA_IOMUX                                        (IOMUX_PINCM7)
/* Defines for LEFT_ENB: GPIOA.7 with pinCMx 14 on package pin 49 */
#define ENC_LEFT_ENB_PORT                                                (GPIOA)
#define ENC_LEFT_ENB_IIDX                                    (DL_GPIO_IIDX_DIO7)
#define ENC_LEFT_ENB_PIN                                         (DL_GPIO_PIN_7)
#define ENC_LEFT_ENB_IOMUX                                       (IOMUX_PINCM14)
/* Defines for RIGHT_ENA: GPIOB.6 with pinCMx 23 on package pin 58 */
#define ENC_RIGHT_ENA_PORT                                               (GPIOB)
// groups represented: ["KEY","ENC"]
// pins affected: ["KEY_2","KEY_3","RIGHT_ENA","RIGHT_ENB"]
#define GPIO_MULTIPLE_GPIOB_INT_IRQN                            (GPIOB_INT_IRQn)
#define GPIO_MULTIPLE_GPIOB_INT_IIDX            (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define ENC_RIGHT_ENA_IIDX                                   (DL_GPIO_IIDX_DIO6)
#define ENC_RIGHT_ENA_PIN                                        (DL_GPIO_PIN_6)
#define ENC_RIGHT_ENA_IOMUX                                      (IOMUX_PINCM23)
/* Defines for RIGHT_ENB: GPIOB.7 with pinCMx 24 on package pin 59 */
#define ENC_RIGHT_ENB_PORT                                               (GPIOB)
#define ENC_RIGHT_ENB_IIDX                                   (DL_GPIO_IIDX_DIO7)
#define ENC_RIGHT_ENB_PIN                                        (DL_GPIO_PIN_7)
#define ENC_RIGHT_ENB_IOMUX                                      (IOMUX_PINCM24)
/* Defines for KEY_0: GPIOA.15 with pinCMx 37 on package pin 8 */
#define KEY_KEY_0_PORT                                                   (GPIOA)
#define KEY_KEY_0_IIDX                                      (DL_GPIO_IIDX_DIO15)
#define KEY_KEY_0_PIN                                           (DL_GPIO_PIN_15)
#define KEY_KEY_0_IOMUX                                          (IOMUX_PINCM37)
/* Defines for KEY_1: GPIOA.12 with pinCMx 34 on package pin 5 */
#define KEY_KEY_1_PORT                                                   (GPIOA)
#define KEY_KEY_1_IIDX                                      (DL_GPIO_IIDX_DIO12)
#define KEY_KEY_1_PIN                                           (DL_GPIO_PIN_12)
#define KEY_KEY_1_IOMUX                                          (IOMUX_PINCM34)
/* Defines for KEY_2: GPIOB.9 with pinCMx 26 on package pin 61 */
#define KEY_KEY_2_PORT                                                   (GPIOB)
#define KEY_KEY_2_IIDX                                       (DL_GPIO_IIDX_DIO9)
#define KEY_KEY_2_PIN                                            (DL_GPIO_PIN_9)
#define KEY_KEY_2_IOMUX                                          (IOMUX_PINCM26)
/* Defines for KEY_3: GPIOB.8 with pinCMx 25 on package pin 60 */
#define KEY_KEY_3_PORT                                                   (GPIOB)
#define KEY_KEY_3_IIDX                                       (DL_GPIO_IIDX_DIO8)
#define KEY_KEY_3_PIN                                            (DL_GPIO_PIN_8)
#define KEY_KEY_3_IOMUX                                          (IOMUX_PINCM25)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_SYSCTL_CLK_init(void);

bool SYSCFG_DL_SYSCTL_SYSPLL_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_I2C_1_init(void);
void SYSCFG_DL_I2C_0_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_UART_1_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
