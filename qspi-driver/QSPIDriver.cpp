#include <stm32h7xx_hal.h>
#include <stm32_hal_legacy.h>
#include "FLASHPluginInterface.h"
#include "FLASHPluginConfig.h"
#include "..\Components\mt25tl01g\mt25tl01g.h"

// https://visualgdb.com/tutorials/arm/stm32/flash/

#ifdef __cplusplus
extern "C"
#endif
void SysTick_Handler(void)
{
	HAL_IncTick();
	HAL_SYSTICK_IRQHandler();
}

FLASHBankInfo FLASHPlugin_Probe(unsigned base, unsigned size, unsigned width1, unsigned width2)
{
    InterruptEnabler enabler;

    FLASHBankInfo result = {
        .BaseAddress = base,
        .BlockCount = MT25TL01G_FLASH_SIZE / MT25TL01G_SUBSECTOR_SIZE,
        .BlockSize = MT25TL01G_SUBSECTOR_SIZE,
        .WriteBlockSize = MINIMUM_PROGRAMMED_BLOCK_SIZE
    };
    return result;
}

WorkAreaInfo FLASHPlugin_FindWorkArea(void* endOfStack)
{
    InterruptEnabler enabler;

    WorkAreaInfo info = { .Address = endOfStack, .Size = 4096 };
    return info;
}
int FLASHPlugin_EraseSectors(unsigned firstSector, unsigned sectorCount)
{
    InterruptEnabler enabler;

    for (unsigned i = 0; i < sectorCount; i++)
    {
        uint8_t error = BSP_QSPI_EraseBlock(0, (firstSector + i) * MT25TL01G_SUBSECTOR_SIZE, MT25TL01G_ERASE_4K);
        if (error != BSP_ERROR_NONE)
            return -1;
    }
    return sectorCount;
}
int FLASHPlugin_DoProgramSync(unsigned startOffset, const void* pData, int bytesToWrite)
{
    uint8_t result = BSP_QSPI_Write(0, (uint8_t*)pData, startOffset, bytesToWrite);
    if (result != BSP_ERROR_NONE)
        return 0;
    return bytesToWrite;
}
int FLASHPlugin_Unload()
{
    BSP_QSPI_DeInit(0);
 //  - causes VisualGDB to hang ? HAL_DeInit();
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;

    for (int i = 0; i < sizeof(NVIC->ICER) / sizeof(NVIC->ICER[0]); i++)
        NVIC->ICER[0] = -1;

    return 0;
}
int main(void)
{
    extern void* g_pfnVectors;

    SCB->VTOR = (uint32_t)&g_pfnVectors;
    HAL_Init();
    /* Initialize the NOR */
    BSP_QSPI_Init_t init;
    init.InterfaceMode = MT25TL01G_QPI_MODE;
    init.TransferRate = MT25TL01G_DTR_TRANSFER;
    init.DualFlashMode = MT25TL01G_DUALFLASH_ENABLE;
    BSP_QSPI_Init(0, &init);

    // test code
    //TestFLASHProgramming(0x90000000, 0);

    FLASHPlugin_InitDone();

    for (;;) {};
}
