//
//  CPU.hpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//

#ifndef CPU_hpp
#define CPU_hpp

#ifdef IMPLEMENTED

#include <algorithm>
#include <iostream>
#include <vector>

#include "Interfaces.h"
#include "Internal_Interfaces.h"

class CPU {
public:
    CPU(CPUType_t type, vector<unsigned> cpu_dynamic, vector<unsigned> cpu_static, vector<unsigned> performance, bool gpu, CPUId_t id);
    Time_t Attach(Time_t now, unsigned remaining_instructions); // Returns time remaining
    unsigned Detach(Time_t now);            // Returns the number of instructions executed since Attach
    void SetState(CPUState_t cstate, Time_t now);
    void SetPerformance(CPUPerformance_t pstate, Time_t now);
    unsigned long long GetEnergy(Time_t now);
    void TaskAbort();                       // In support of migration
    void TaskRun(TaskId_t task_id);         // Schedule task to run on the CPU
    void TaskStop();                        // Stopping the task on a normal context switch
    void SetSlowDown(unsigned slowdown)             { slow_down = slowdown; }
    CPUType_t GetType()                             { return cpu_type; }
private:
    CPUType_t cpu_type;                     // For testing
    CPUId_t identifier;                     // Unique CPU identifier in the CPU array
    Time_t start_meter;                     // For tracking energy consumption
    Time_t start_run;                       // For tracking execution
    unsigned slow_down;                     // To factor in the memory load on the system
    unsigned long long energy_consumed;     // A counter of the joules consumed, initialized to 0
    TaskId_t task_id;                       // Id of the task that is currently active
    CPUState_t c_state;                     // Current C state, initialized to C1 (idle)
    CPUPerformance_t p_state;               // Current P state, initialized to P0
    unsigned gpu_flag;                      // True if there is a GPU
    std::vector<unsigned> dynamic_power;    // Dynamic power for the P states
    std::vector<unsigned> static_power;     // Power for C states C1 and higher
    std::vector<unsigned> performance;      // Available MIPS 
    void ComputeEnergy(Time_t now);         // Update the energy consumption. Must be called whenever P or C state changes
};
#endif
#endif /* CPU_hpp */

