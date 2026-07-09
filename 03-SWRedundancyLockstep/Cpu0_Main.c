#include "Ifx_Types.h"
#include "IfxCpu.h"
#include "IfxScuWdt.h"

// Shared memory between cores in DSPR
volatile uint32 g_cpu0_result = 0;
volatile uint32 g_cpu1_result = 0;
volatile uint8  g_cpu0_done   = 0;
volatile uint8  g_cpu1_done   = 0;

IFX_ALIGN(4) IfxCpu_syncEvent cpuSyncEvent = 0;

// The "critical" computation both cores must run identically
static uint32 criticalComputation(uint32 input){
    uint32 result = 0;
    for(uint32 i = 0; i < input; i++)
        result += i * 3;
    return result;
}

void core0_main(void){
    IfxCpu_enableInterrupts();
    IfxScuWdt_disableCpuWatchdog(IfxScuWdt_getCpuWatchdogPassword());
    IfxScuWdt_disableSafetyWatchdog(IfxScuWdt_getSafetyWatchdogPassword());

    // Sync both cores ONCE before starting
    IfxCpu_emitEvent(&cpuSyncEvent);
    IfxCpu_waitEvent(&cpuSyncEvent, 1);

    while(1){
        // Reset flags each iteration
        g_cpu0_done = 0;
        g_cpu1_done = 0;

        // CPU0 runs the computation
        g_cpu0_result = criticalComputation(100);
        g_cpu0_done   = 1;

        // Wait for CPU1 to finish
        while(g_cpu1_done == 0);

        // Compare results
        if(g_cpu0_result == g_cpu1_result){
            // Match — safe, continue next iteration
        }
        else{
            // Mismatch — fault detected, halt
            while(1);
        }
    }
}
