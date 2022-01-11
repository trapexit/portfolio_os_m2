/******************************************************************************
**
**  @(#) dspp_addresses_anvil.h 95/12/04 1.2
**
**  Anvil DSPI addresses required for remapping
**
******************************************************************************/

/* ======================== Opera/Anvil ==================== */

/* -------------------- Opera/Anvil stuff needed for absolute address remapping */

    /* I/O Registers */
#define DSPI_ANVIL_NOISE                (0x0ea)
#define DSPI_ANVIL_CLOCK                (0x0ef)


/* -------------------- Opera/Anvil stuff needed for compatibility mode */

    /* Memory Segments */
#define DSPI_ANVIL_CODE_MEMORY_BASE     (0x000)
#define DSPI_ANVIL_CODE_MEMORY_SIZE     (0x200)

#define DSPI_ANVIL_EI_MEMORY_BASE       (0x000)
#define DSPI_ANVIL_EI_MEMORY_SIZE       (0x070)

#define DSPI_ANVIL_I_MEMORY_BASE        (0x100)
#define DSPI_ANVIL_I_MEMORY_SIZE        (0x100)

#define DSPI_ANVIL_EO_MEMORY_BASE       (0x300)
#define DSPI_ANVIL_EO_MEMORY_SIZE       (0x010)


    /* DMA */
#define DSPI_ANVIL_NUM_INPUT_DMAS       (13)
#define DSPI_ANVIL_NUM_FOREIGN_DMAS     (3)         /* 3 non-audio DMA channels sandwiched between input and output */
#define DSPI_ANVIL_NUM_OUTPUT_DMAS      (4)
#define DSPI_ANVIL_NUM_DMA_CHANNELS     (DSPI_ANVIL_NUM_INPUT_DMAS + DSPI_ANVIL_NUM_FOREIGN_DMAS + DSPI_ANVIL_NUM_OUTPUT_DMAS)

    /* clock */
#define DSPP_ANVIL_CLOCK_RATE           25000000    /* must be a constant because we presumably can't get it on M2, yet we need to emulate anvil behavior */

