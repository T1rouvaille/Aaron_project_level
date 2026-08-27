/* generated vector source file - do not edit */
#include "bsp_api.h"
/* Do not build these data structures if no interrupts are currently allocated because IAR will have build errors. */
#if VECTOR_DATA_IRQ_COUNT > 0
        BSP_DONT_REMOVE const fsp_vector_t g_vector_table[BSP_ICU_VECTOR_MAX_ENTRIES] BSP_PLACE_IN_SECTION(BSP_SECTION_APPLICATION_VECTORS) =
        {
                        [0] = sci_uart_rxi_isr, /* SCI1 RXI (Receive data full) */
            [1] = sci_uart_txi_isr, /* SCI1 TXI (Transmit data empty) */
            [2] = sci_uart_tei_isr, /* SCI1 TEI (Transmit end) */
            [3] = sci_uart_eri_isr, /* SCI1 ERI (Receive error) */
            [4] = adc_scan_end_isr, /* ADC0 SCAN END (End of A/D scanning operation) */
            [5] = iic_master_txi_isr, /* IIC0 TXI (Transmit data empty) */
            [6] = iic_master_tei_isr, /* IIC0 TEI (Transmit end) */
            [7] = iic_master_eri_isr, /* IIC0 ERI (Transfer error) */
            [8] = iic_master_rxi_isr, /* IIC0 RXI (Receive data full) */
            [9] = spi_txi_isr, /* SPI0 TXI (Transmit buffer empty) */
            [10] = gpt_counter_overflow_isr, /* GPT9 COUNTER OVERFLOW (Overflow) */
            [11] = spi_eri_isr, /* SPI0 ERI (Error) */
            [12] = agt_int_isr, /* AGT1 INT (AGT interrupt) */
            [14] = spi_tei_isr, /* SPI0 TEI (Transmission complete event) */
            [16] = spi_rxi_isr, /* SPI0 RXI (Receive buffer full) */
        };
        #if BSP_FEATURE_ICU_HAS_IELSR
        const bsp_interrupt_event_t g_interrupt_event_link_select[BSP_ICU_VECTOR_MAX_ENTRIES] =
        {
            [0] = BSP_PRV_VECT_ENUM(EVENT_SCI1_RXI,GROUP0), /* SCI1 RXI (Receive data full) */
            [1] = BSP_PRV_VECT_ENUM(EVENT_SCI1_TXI,GROUP1), /* SCI1 TXI (Transmit data empty) */
            [2] = BSP_PRV_VECT_ENUM(EVENT_SCI1_TEI,GROUP2), /* SCI1 TEI (Transmit end) */
            [3] = BSP_PRV_VECT_ENUM(EVENT_SCI1_ERI,GROUP3), /* SCI1 ERI (Receive error) */
            [4] = BSP_PRV_VECT_ENUM(EVENT_ADC0_SCAN_END,GROUP4), /* ADC0 SCAN END (End of A/D scanning operation) */
            [5] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TXI,GROUP5), /* IIC0 TXI (Transmit data empty) */
            [6] = BSP_PRV_VECT_ENUM(EVENT_IIC0_TEI,GROUP6), /* IIC0 TEI (Transmit end) */
            [7] = BSP_PRV_VECT_ENUM(EVENT_IIC0_ERI,GROUP7), /* IIC0 ERI (Transfer error) */
            [8] = BSP_PRV_VECT_ENUM(EVENT_IIC0_RXI,GROUP0), /* IIC0 RXI (Receive data full) */
            [9] = BSP_PRV_VECT_ENUM(EVENT_SPI0_TXI,GROUP1), /* SPI0 TXI (Transmit buffer empty) */
            [10] = BSP_PRV_VECT_ENUM(EVENT_GPT9_COUNTER_OVERFLOW,GROUP2), /* GPT9 COUNTER OVERFLOW (Overflow) */
            [11] = BSP_PRV_VECT_ENUM(EVENT_SPI0_ERI,GROUP3), /* SPI0 ERI (Error) */
            [12] = BSP_PRV_VECT_ENUM(EVENT_AGT1_INT,GROUP4), /* AGT1 INT (AGT interrupt) */
            [14] = BSP_PRV_VECT_ENUM(EVENT_SPI0_TEI,GROUP6), /* SPI0 TEI (Transmission complete event) */
            [16] = BSP_PRV_VECT_ENUM(EVENT_SPI0_RXI,GROUP0), /* SPI0 RXI (Receive buffer full) */
        };
        #endif
        #endif
