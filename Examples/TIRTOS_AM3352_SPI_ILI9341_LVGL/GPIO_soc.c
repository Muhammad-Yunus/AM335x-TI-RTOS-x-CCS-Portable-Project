#include <ti/csl/csl_utils.h>
#include <ti/drv/gpio/GPIO.h>
#include <ti/starterware/include/hw/soc_am335x.h>
#include <ti/csl/csl_types.h>
#include <ti/drv/gpio/soc/GPIO_v1.h>

#define CSL_GPIO_PER_CNT    4U

GPIO_v1_hwAttrs_list GPIO_v1_hwAttrs = {
    {
       SOC_GPIO_0_REGS,
       96,
       97,
       0,
       0
    },
    {
       SOC_GPIO_1_REGS,
       98,
       99,
       0,
       0
    },
    {
       SOC_GPIO_2_REGS,
       32,
       33,
       0,
       0
    },
    {
       SOC_GPIO_3_REGS,
       62,
       63,
       0,
       0
    },
    {   0,0,0,0,0   },
    {   0,0,0,0,0   },
    {   0,0,0,0,0   },
    {   0,0,0,0,0   },
};

CSL_PUBLIC_CONST GPIOConfigList GPIO_config =
{
    {
        &GPIO_FxnTable_v1,
        NULL,
        NULL
    },
    {
        NULL,
        NULL,
        NULL
    },
    {
        NULL,
        NULL,
        NULL
    }
};
