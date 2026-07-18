#include <stdint.h>
#include <ti/csl/hw_types.h>
#include <ti/starterware/include/hw/hw_i2c.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/starterware/include/am335x/hw_cm_per.h>
#include <ti/starterware/include/hw/hw_control_am335x.h>

#include "i2c1_scanner.h"

#define I2C1_SCL_PIN_VAL  (0x3A)
#define I2C1_SDA_PIN_VAL  (0x3A)

void Board_initI2C1(void)
{
    HWREG(SOC_CM_PER_REGS + CM_PER_I2C1_CLKCTRL) =
        (HWREG(SOC_CM_PER_REGS + CM_PER_I2C1_CLKCTRL) & ~CM_PER_I2C1_CLKCTRL_MODULEMODE) |
        CM_PER_I2C1_CLKCTRL_MODULEMODE_ENABLE;

    while ((HWREG(SOC_CM_PER_REGS + CM_PER_I2C1_CLKCTRL) & CM_PER_I2C1_CLKCTRL_IDLEST) !=
           CM_PER_I2C1_CLKCTRL_IDLEST_FUNC);

    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_SPI0_D1) = I2C1_SCL_PIN_VAL;
    HWREG(SOC_CONTROL_REGS + CONTROL_CONF_SPI0_CS0) = I2C1_SDA_PIN_VAL;
}

void SetupI2C1Master(void)
{
    unsigned int base = SOC_I2C_1_REGS;
    unsigned int prescaler;
    unsigned int divider;

    HWREG(base + I2C_CON) &= ~I2C_CON_EN_MASK;

    HWREG(base + I2C_SYSC) &= ~I2C_SYSC_AUTOIDLE_MASK;

    prescaler = (48000000u / 12000000u) - 1u;
    divider   = (12000000u / 100000u) / 2u;

    HWREG(base + I2C_PSC)  = prescaler;
    HWREG(base + I2C_SCLL) = divider - 7u;
    HWREG(base + I2C_SCLH) = divider - 5u;

    HWREG(base + I2C_CON) |= I2C_CON_EN_MASK;

    while (!(HWREG(base + I2C_SYSS) & I2C_SYSS_RDONE_MASK));
}

static void i2c_soft_recover(unsigned int base)
{
    HWREG(base + I2C_CON) &= ~I2C_CON_EN_MASK;
    HWREG(base + I2C_SYSC) |= I2C_SYSC_SRST_MASK;
    HWREG(base + I2C_SYSC) &= ~I2C_SYSC_AUTOIDLE_MASK;
    {
        unsigned int p = (48000000u / 12000000u) - 1u;
        unsigned int d = (12000000u / 100000u) / 2u;
        HWREG(base + I2C_PSC)  = p;
        HWREG(base + I2C_SCLL) = d - 7u;
        HWREG(base + I2C_SCLH) = d - 5u;
    }
    HWREG(base + I2C_CON) |= I2C_CON_EN_MASK;
}

int I2C1ProbeAddress(unsigned char addr)
{
    unsigned int base = SOC_I2C_1_REGS;
    unsigned int status;
    unsigned int waited;
    unsigned int con;
    const unsigned int TIMEOUT = 200000u;
    int present = 0;

    if (HWREG(base + I2C_IRQSTS_RAW) & I2C_IRQSTS_RAW_BB_MASK)
    {
        HWREG(base + I2C_CON) |= I2C_CON_STP_MASK;
        waited = 0;
        while ((HWREG(base + I2C_IRQSTS_RAW) & I2C_IRQSTS_RAW_BB_MASK) && (waited++ < TIMEOUT));

        if (HWREG(base + I2C_IRQSTS_RAW) & I2C_IRQSTS_RAW_BB_MASK)
            i2c_soft_recover(base);
    }
    HWREG(base + I2C_IRQSTS) = 0x7FFFu;

    HWREG(base + I2C_SA)  = addr;
    HWREG(base + I2C_CNT) = 1;

    con = HWREG(base + I2C_CON);
    con &= ~(I2C_CON_STT_MASK | I2C_CON_STP_MASK);
    con |= I2C_CON_MST_MASK | I2C_CON_TRX_MASK;
    con |= I2C_CON_STT_MASK;
    HWREG(base + I2C_CON) = con;

    HWREG(base + I2C_DATA) = 0;

    waited = 0;
    while (!(HWREG(base + I2C_IRQSTS_RAW) & I2C_IRQSTS_RAW_BB_MASK) && (waited++ < TIMEOUT));

    waited = 0;
    do {
        status = HWREG(base + I2C_IRQSTS_RAW);
        waited++;
    } while (!(status & I2C_IRQSTS_RAW_ARDY_MASK) && (waited < TIMEOUT));

    if ((status & I2C_IRQSTS_RAW_ARDY_MASK) &&
        !(status & I2C_IRQSTS_RAW_NACK_MASK) &&
        !(status & I2C_IRQSTS_RAW_AL_MASK))
    {
        present = 1;
    }

    HWREG(base + I2C_CON) |= I2C_CON_STP_MASK;

    waited = 0;
    do {
        status = HWREG(base + I2C_IRQSTS_RAW);
        waited++;
    } while (!(status & I2C_IRQSTS_RAW_BF_MASK) && (waited < TIMEOUT));

    if (!(status & I2C_IRQSTS_RAW_BF_MASK))
    {
        i2c_soft_recover(base);
        present = 0;
    }

    HWREG(base + I2C_IRQSTS) = 0x7FFFu;

    return present;
}
