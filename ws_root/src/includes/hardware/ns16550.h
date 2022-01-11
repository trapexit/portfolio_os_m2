#ifndef __HARDWARE_NS16550_H
#define __HARDWARE_NS16550_H


/******************************************************************************
**
**  @(#) ns16550.h 96/02/20 1.4
**
******************************************************************************/


#ifndef __KERNEL_TYPES_H
#include <kernel/types.h>
#endif


/*****************************************************************************/


/* National Semiconductor 16550D UART register layout.
 *
 * Note: To access the two divisor latch registers, you must set the LCR_DLAB
 *       bit in the ns_LCR register.
 */
typedef struct NS16550
{
    uint8 ns_Reg0;       /* multiplexed, see names below              */
    uint8 ns_Reg1;       /* multiplexed, see names below              */
    uint8 ns_Reg2;       /* multiplexed, see names below              */
    uint8 ns_LCR;        /* line control register                     */
    uint8 ns_MCR;        /* MODEM control register                    */
    uint8 ns_LSR;        /* line status register                      */
    uint8 ns_MSR;        /* MODEM status register                     */
    uint8 ns_SCR;        /* scratch register                          */
} NS16550;

/* names to use for the first three registers */
#define ns_RBR ns_Reg0    /* receiver buffer register (read-only)      */
#define ns_THR ns_Reg0    /* transmitter holding register (write-only) */
#define ns_DLL ns_Reg0    /* divisor latch (LS)                        */
#define ns_IER ns_Reg1    /* interrupt enable register                 */
#define ns_DLM ns_Reg1    /* divisor latch (MS)                        */
#define ns_IIR ns_Reg2    /* interrupt ID register (read-only)         */
#define ns_FCR ns_Reg2    /* FIFO control register (write-only)        */

#define IER_ERBFI               0x01  /* enable receive data avail int   */
#define IER_ETBEI               0x02  /* enable THRE interrupt           */
#define IER_ELSI                0x04  /* enable receiver line status int */
#define IER_EDSSI               0x08  /* enable modem status interrupt   */

#define IIR_NOTPENDING          0x01  /* interrupt not pending           */
#define IIR_INT_ID0             0x02  /* interrupt ID (bit 0)            */
#define IIR_INT_ID1             0x04  /* interrupt ID (bit 1)            */
#define IIR_INT_ID2             0x08  /* interrupt ID (bit 2)            */
#define IIR_FIFOSENABLED_0      0x40  /* matches FCR_FIFOENABLE          */
#define IIR_FIFOSENABLED_1      0x80  /* matches FCR_FIFOENABLE          */

/* interpretation of IIR_INT_ID? bits */
#define IIR_ID_MODEMSTATUS      0x00  /* modem status change             */
#define IIR_ID_TXFIFOEMPTY      0x02  /* transmitter FIFO empty          */
#define IIR_ID_RXDATAAVAILABLE  0x04  /* receiver data available         */
#define IIR_ID_RXLINESTATUS     0x06  /* receiver line status change     */
#define IIR_ID_CHARTIMEOUT      0x0c  /* character timeout               */

#define FCR_FIFOENABLE          0x01  /* FIFO enable                     */
#define FCR_RXRESET             0x02  /* receiver FIFO reset             */
#define FCR_TXRESET             0x04  /* transmitter FIFO reset          */
#define FCR_DMAMODE             0x08  /* DMA mode select                 */
#define FCR_RCVRTRIG0           0x40  /* receiver trigger (LSB)          */
#define FCR_RCVRTRIG1           0x80  /* receiver trigger (MSB)          */

/* interpretation of the FCR_RCVRTRIG? bits */
#define FCR_TRIG_1              0x00  /* interrupt when 1 byte in FIFO   */
#define FCR_TRIG_4              0x40  /* interrupt when 4 bytes in FIFO  */
#define FCR_TRIG_8              0x80  /* interrupt when 8 bytes in FIFO  */
#define FCR_TRIG_14             0xc0  /* interrupt when 14 bytes in FIFO */

#define LCR_WLS0                0x01  /* word length select (bit 0)      */
#define LCR_WLS1                0x02  /* word length select (bit 1)      */
#define LCR_STB                 0x04  /* number of stop bits             */
#define LCR_PEN                 0x08  /* parity enable                   */
#define LCR_EPS                 0x10  /* even parity select              */
#define LCR_STICK               0x20  /* stick parity                    */
#define LCR_BREAK               0x40  /* set break                       */
#define LCR_DLAB                0x80  /* divisor latch access bit        */

/* interpretation of the LCR_WLS? bits */
#define LCR_5BIT                0x00
#define LCR_6BIT                0x01
#define LCR_7BIT                0x02
#define LCR_8BIT                0x03

#define MCR_DTR                 0x01  /* data terminal ready             */
#define MCR_RTS                 0x02  /* request to send                 */
#define MCR_OUTPUT1             0x04  /* out1                            */
#define MCR_OUTPUT2             0x08  /* out2                            */
#define MCR_LOCALLOOPBACK       0x10  /* loop                            */

#define LSR_DR                  0x01  /* data ready                      */
#define LSR_OE                  0x02  /* overrun error                   */
#define LSR_PE                  0x04  /* parity error                    */
#define LSR_FE                  0x08  /* framing error                   */
#define LSR_BI                  0x10  /* break interrupt                 */
#define LSR_THRE                0x20  /* transmitter holding register    */
#define LSR_TEMT                0x40  /* transmitter empty               */
#define LSR_RXFIFOERROR         0x80  /* error in RCVR FIFO              */

#define MSR_DCTS                0x01  /* delta clear to send             */
#define MSR_DDSR                0x02  /* delta data set ready            */
#define MSR_TERI                0x04  /* trailing edge ring indicator    */
#define MSR_DDCD                0x08  /* delta data carrier detected     */
#define MSR_CTS                 0x10  /* clear to send                   */
#define MSR_DSR                 0x20  /* data set ready                  */
#define MSR_RING                0x40  /* ring indicator                  */
#define MSR_DCD                 0x80  /* data carrier detect             */

#define NS16550_OUTPUT_FIFO_SIZE 16


/*****************************************************************************/


#endif /* __HARDWARE_NS16550_H */
