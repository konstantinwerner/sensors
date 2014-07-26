/*
 * btm222_module.c
 *
 *  Created on: 15.03.2014
 *      Author: Konstantin
 */

#include "btm222_module.h"
#include "lpc17xx_libcfg.h"

#include "lpc17xx_uart.h"
#include "lpc17xx_pinsel.h"

#if (BTM222_UART_PORT==0)
	#define UART_PORT	LPC_UART0
	#define UARTx_IRQn  UART0_IRQn
	#define UARTx_IRQHandler UART0_IRQHandler

	#define UART_TXD_PORT	0
	#define UART_TXD_PIN	2
	#define UART_TXD_FUNC	1

	#define UART_RXD_PORT	0
	#define UART_RXD_PIN	3
	#define UART_RXD_FUNC	1

#elif (BTM222_UART_PORT==1)
	#define UART_PORT	LPC_UART1
	#define UARTx_IRQn  UART1_IRQn
	#define UARTx_IRQHandler UART1_IRQHandler

	#define UART_TXD_PORT	BTM222_TXD_PORT
	#define UART_TXD_PIN	BTM222_TXD_PIN

	#if (UART_TXD_PORT == 0)
		#define	UART_TXD_FUNC	1
	#elif (UART_TXD_PORT == 2)
		#define UART_TXD_FUNC	2
	#endif

	#define UART_RXD_PORT	BTM222_RXD_PORT
	#define UART_RXD_PIN	BTM222_RXD_PIN

	#if (UART_RXD_PORT == 0)
		#define	UART_TXD_FUNC	1
	#elif (UART_RXD_PORT == 2)
		#define UART_TXD_FUNC	2
	#endif

#elif (BTM222_UART_PORT==2)
	#define UART_PORT	LPC_UART2
	#define UARTx_IRQn  UART2_IRQn
	#define UARTx_IRQHandler UART2_IRQHandler

	#define UART_TXD_PORT	BTM222_TXD_PORT
	#define UART_TXD_PIN	BTM222_TXD_PIN

	#if (UART_TXD_PORT == 0)
		#define	UART_TXD_FUNC	1
	#elif (UART_TXD_PORT == 2)
		#define UART_TXD_FUNC	2
	#endif

	#define UART_RXD_PORT	BTM222_RXD_PORT
	#define UART_RXD_PIN	BTM222_RXD_PIN

	#if (UART_RXD_PORT == 0)
		#define	UART_RXD_FUNC	1
	#elif (UART_RXD_PORT == 2)
		#define UART_RXD_FUNC	2
	#endif

#elif (BTM222_UART_PORT==3)
	#define UART_PORT	LPC_UART3
	#define UARTx_IRQn  UART3_IRQn
	#define UARTx_IRQHandler UART3_IRQHandler

	#define UART_TXD_PORT	BTM222_TXD_PORT
	#define UART_TXD_PIN	BTM222_TXD_PIN

	#if (UART_TXD_PORT == 0)
	# if (UART_TXD_PIN == 0)
		#define UART_TXD_FUNC	2
	# elif (UART_TXD_PIN == 25)
		#define UART_TXD_FUNC	3
	# endif
	#elif (UART_TXD_PORT == 4)
		#define UART_TXD_FUNC	3
	#endif

	#define UART_RXD_PORT	BTM222_RXD_PORT
	#define UART_RXD_PIN	BTM222_RXD_PIN

	#if (UART_RXD_PORT == 0)
	# if (UART_RXD_PIN == 1)
		#define UART_RXD_FUNC	2
	# elif (UART_RXD_PIN == 26)
		#define UART_RXD_FUNC	3
	# endif
	#elif (UART_RXD_PORT == 4)
		#define UART_RXD_FUNC	3
	#endif

#endif

#define IER_RBR		0x01
#define IER_THRE	0x02
#define IER_RLS		0x04

#define IIR_PEND	0x01
#define IIR_RLS		0x03
#define IIR_RDA		0x02
#define IIR_CTI		0x06
#define IIR_THRE	0x01

#define LSR_RDR		0x01
#define LSR_OE		0x02
#define LSR_PE		0x04
#define LSR_FE		0x08
#define LSR_BI		0x10
#define LSR_THRE	0x20
#define LSR_TEMT	0x40
#define LSR_RXFE	0x80

volatile uint32_t btm222_uart_status;
volatile uint8_t btm222_uart_txempty = 1;
volatile uint8_t btm222_uart_buffer[BTM222_BUFSIZE];
volatile uint32_t btm222_uart_count = 0;

uint8_t echo = 0;

void btm222_init(uint32_t baud_rate)
{
	UART_CFG_Type UART_config;
	UART_FIFO_CFG_Type UART_fifo_config;
	PINSEL_CFG_Type pin_config;

	// Configure TXD and RXD Pins
	pin_config.OpenDrain = PINSEL_PINMODE_NORMAL;
	pin_config.Pinmode = PINSEL_PINMODE_PULLUP;

	pin_config.Funcnum = UART_TXD_FUNC;
	pin_config.Portnum = UART_TXD_PORT;
	pin_config.Pinnum  = UART_TXD_PIN;
	PINSEL_ConfigPin(&pin_config);

	pin_config.Funcnum = UART_RXD_FUNC;
	pin_config.Portnum = UART_RXD_PORT;
	pin_config.Pinnum  = UART_RXD_PIN;
	PINSEL_ConfigPin(&pin_config);

#pragma message "BT TXD on Pin : P" STRING(UART_TXD_PORT) "_" STRING(UART_TXD_PIN)
#pragma message "BT RXD on Pin : P" STRING(UART_RXD_PORT) "_" STRING(UART_RXD_PIN)

	// Configure UART
	UART_config.Baud_rate = baud_rate;
	UART_config.Databits  = UART_DATABIT_8;
	UART_config.Parity    = UART_PARITY_NONE;
	UART_config.Stopbits  = UART_STOPBIT_1;
	UART_Init((LPC_UART_TypeDef *)UART_PORT, &UART_config);

	// Configure FIFO
	UART_fifo_config.FIFO_DMAMode = DISABLE;
	UART_fifo_config.FIFO_Level = UART_FIFO_TRGLEV0;
	UART_fifo_config.FIFO_ResetRxBuf = ENABLE;
	UART_fifo_config.FIFO_ResetTxBuf = ENABLE;
	UART_FIFOConfig((LPC_UART_TypeDef *)UART_PORT, &UART_fifo_config);

	// Enable UART Transmit
	UART_TxCmd((LPC_UART_TypeDef *)UART_PORT, ENABLE);
	// Enable UART interrupts
	UART_IntConfig((LPC_UART_TypeDef *)UART_PORT, UART_INTCFG_RLS, ENABLE);
	UART_IntConfig((LPC_UART_TypeDef *)UART_PORT, UART_INTCFG_RBR, ENABLE);
	UART_IntConfig((LPC_UART_TypeDef *)UART_PORT, UART_INTCFG_THRE, ENABLE);
	// preemption = 1, sub-priority = 1
	NVIC_SetPriority(UARTx_IRQn, ((0x01 << 3) | 0x01));
	// Enable Interrupt for UART
	NVIC_EnableIRQ(UARTx_IRQn);
}

void btm222_enableEcho(void)
{
	echo = 1;
}

void btm222_disableEcho(void)
{
	echo = 0;
}

inline void btm222_send(const uint8_t *buf, uint32_t buflen)
{
	UART_Send((LPC_UART_TypeDef *)UART_PORT, buf, buflen, BLOCKING);
}

inline uint32_t btm222_receive(uint8_t *buf, uint32_t buflen)
{
	return UART_Receive((LPC_UART_TypeDef *)UART_PORT, buf, buflen, BLOCKING);
}

inline void btm222_putc(uint8_t c)
{
	UART_Send((LPC_UART_TypeDef *)UART_PORT, &c, 1, BLOCKING);
}

void btm222_puts(const void *str)
{
	uint8_t *s = (uint8_t *) str;

	while (*s)
	{
		btm222_putc(*s++);
	}
}

inline void btm222_puts_(const void *str)
{
	btm222_puts(str);
	btm222_puts("\r\n");
}

void UARTx_IRQHandler (void)
{
	uint32_t IIRValue, LSRValue;
	uint8_t tmp = tmp;

	IIRValue = UART_PORT->IIR;

	IIRValue >>= 1;			// skip pending bit in IIR
	IIRValue &= 0x07;		// check bit 1~3, interrupt identification

	if (IIRValue == IIR_RLS)		/* Receive Line Status */
	{
		LSRValue = UART_PORT->LSR;

		if (LSRValue & (LSR_OE|LSR_PE|LSR_FE|LSR_RXFE|LSR_BI))
		{
			/* There are errors or break interrupt */
			/* Read LSR will clear the interrupt */
			btm222_uart_status = LSRValue;
			tmp = UART_PORT->RBR;		/* Dummy read on RX to clear interrupt, then bail out */
			return;
		}
		if (LSRValue & LSR_RDR)	/* Receive Data Ready */
		{
			/* If no error on RLS, normal ready, save into the data buffer. */
			/* Note: read RBR will clear the interrupt */

			tmp = UART_PORT->RBR;

			if (echo)
			{
				UART_Send((LPC_UART_TypeDef *)UART_PORT, &tmp, 1, BLOCKING);
			} else
			{
				btm222_uart_buffer[btm222_uart_count] = tmp;
				btm222_uart_count++;

				if (btm222_uart_count == BTM222_BUFSIZE)
				{
					btm222_uart_count = 0;
				}
			}
		}
	}
	else if (IIRValue == IIR_RDA)	/* Receive Data Available */
	{
		tmp = UART_PORT->RBR;

		if (echo)
		{
			UART_Send((LPC_UART_TypeDef *)UART_PORT, &tmp, 1, BLOCKING);
		} else
		{
			btm222_uart_buffer[btm222_uart_count] = tmp;
			btm222_uart_count++;

			if (btm222_uart_count == BTM222_BUFSIZE)
			{
				btm222_uart_count = 0;
			}
		}
	}
	else if (IIRValue == IIR_CTI)	/* Character timeout indicator */
	{
		// Bit 9 as the CTI error
		btm222_uart_status |= 0x100;
	}
	else if (IIRValue == IIR_THRE)	/* THRE, transmit holding register empty */
	{
		// Check status in the LSR to see if
		// valid data in U0THR or not
		LSRValue = UART_PORT->LSR;
		if ( LSRValue & LSR_THRE )
		{
			btm222_uart_txempty = 1;
		}
		else
		{
			btm222_uart_txempty = 0;
		}
	}
}

