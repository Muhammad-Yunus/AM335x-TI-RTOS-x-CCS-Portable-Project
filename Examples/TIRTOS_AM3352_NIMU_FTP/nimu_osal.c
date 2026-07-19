/*
 * Copyright (c) 2015 Texas Instruments Incorporated
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met.
 */

/**
 *  @file nimu_osal.c
 *  @brief NIMU OS Abstraction Layer
 *
 *  Provides OS-independent wrapper functions for NIMU driver.
 *  Maps NIMU memory allocation, task creation, and task sleep
 *  to SYS/BIOS equivalents (Memory_alloc, Task_create, Task_sleep).
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <xdc/std.h>
#include <xdc/runtime/IHeap.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Memory.h>
#include <xdc/runtime/Error.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>

#include <ti/csl/tistdtypes.h>
#include <ti/csl/cslr_device.h>
#include <ti/csl/csl_chip.h>

/* ========================================================================== */
/*                         Task Sleep                                         */
/* ========================================================================== */

/**
 *  @brief Sleep for specified number of SYS/BIOS ticks
 *
 *  @param  sleepTime  Number of ticks to sleep (1 tick = 100us by default)
 */
void Osal_TaskSleep(uint32_t sleepTime)
{
    Task_sleep(sleepTime);
}

/* ========================================================================== */
/*                      Task Creation (Single Argument)                       */
/* ========================================================================== */

/**
 *  @brief Create a task with a single callback function pointer
 *
 *  Creates a SYS/BIOS task at priority 10 with default stack size.
 *  Task name is set to "EmacPollPkt".
 *
 *  @param  pCbFn  Pointer to the task entry function
 */
void Osal_TaskCreate(void* pCbFn)
{
    Task_Params     taskParams;
    Error_Block     eb;

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.priority = 10;
    taskParams.instance->name = "EmacPollPkt";
    Task_create((ti_sysbios_knl_Task_FuncPtr)pCbFn, &taskParams, &eb);
}

/* ========================================================================== */
/*                  Task Creation (Two Arguments)                             */
/* ========================================================================== */

/**
 *  @brief Create a task with callback function and argument
 *
 *  Creates a SYS/BIOS task at priority 10 with arg0 passed to the task.
 *  Task name is set to "EmacPollPkt".
 *
 *  @param  pCbFn  Pointer to the task entry function
 *  @param  arg    Argument passed to the task (arg0)
 */
void Osal_TaskCreate_v2(void* pCbFn, uint32_t arg)
{
    Task_Params     taskParams;
    Error_Block     eb;

    Error_init(&eb);
    Task_Params_init(&taskParams);
    taskParams.arg0 = arg;
    taskParams.priority = 10;
    taskParams.instance->name = "EmacPollPkt";
    Task_create((ti_sysbios_knl_Task_FuncPtr)pCbFn, &taskParams, &eb);
}

/* ========================================================================== */
/*                      Memory Allocation                                     */
/* ========================================================================== */

/**
 *  @brief Allocate a memory block
 *
 *  Allocates memory using SYS/BIOS Memory_alloc with 4-byte alignment.
 *
 *  @param  num_bytes  Number of bytes to allocate
 *  @return Pointer to allocated memory, or NULL on failure
 */
void* Osal_malloc(uint32_t num_bytes)
{
    Error_Block     errorBlock;
    Void*           ptr;

    Error_init(&errorBlock);
    ptr = Memory_alloc(NULL, num_bytes, 4, &errorBlock);
    if (ptr == NULL)
    {
        return NULL;
    }
    return ptr;
}

/* ========================================================================== */
/*                      Memory Free                                           */
/* ========================================================================== */

/**
 *  @brief Free a previously allocated memory block
 *
 *  Frees memory using SYS/BIOS Memory_free.
 *
 *  @param  ptr         Pointer to the memory block to free
 *  @param  num_bytes   Size of the memory block being freed
 */
void Osal_free(void* ptr, uint32_t num_bytes)
{
    Memory_free(NULL, ptr, num_bytes);
}
