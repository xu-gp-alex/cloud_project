//
//  Machine.hpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//

#ifndef Machine_hpp
#define Machine_hpp

#include <iostream>
#include <queue>
#include <string>
#include <vector>

#include "Interfaces.h"
#include "Internal_Interfaces.h"

#define QUANTUM 60000

typedef struct {
    TaskId_t task_id;
    VMId_t vm_id;
} Job;

class CPU {
public:
    CPU(vector<unsigned> & cpu_dynamic, vector<unsigned> & cpu_static, vector<unsigned> & performance, bool gpu, CPUId_t cpu_id);
    uint64_t GetEnergy()                    { ComputeEnergy(); return energyConsumed; }
    Job GetJob()                            { return jobRunning; }
    CPUId_t GetId()                         { return cpuId; }
    Time_t GetProjectedFinish()             { return projectedFinish; }
    bool IsBusy()                           { return cState == C0; }
    void SetCState(CPUState_t cstate);
    void SetPState(CPUPerformance_t pstate);
    void TaskRun(Job & job, unsigned slow_down, Time_t next_timer);         // Schedule task to run on the CPU
    void TaskStop();                        // Stopping the task on a normal context switch
private:
    // Intrinsic
    bool gpuFlag;                           // True if there is a GPU
    CPUId_t cpuId;
    // Tasks
    Job jobRunning;                         // Job currently running
    uint64_t instrToRun;                    // Instructions to run this quantum
    Time_t startRun;                        // For tracking execution
    Time_t projectedFinish;                 // Potential finish time
    // Energy
    void ComputeEnergy();                   // Update the energy consumption. Must be called whenever P or C state changes
    Time_t startMeter;                      // For tracking energy consumption
    uint64_t energyConsumed;      // A counter of the joules consumed, initialized to 0
    CPUState_t cState;                      // Current C state, initialized to C1 (idle)
    CPUPerformance_t pState;                // Current P state, initialized to P0
    vector<unsigned> pStates;               // Dynamic power for the P states
    vector<unsigned> cStates;               // Power for C states C1 and higher
    vector<unsigned> performance;           // Available MIPS
 };

class Machine {
public:
    Machine(u_int memory, u_int cpus, vector<u_int> s_states, vector<u_int> c_states, vector<u_int> p_states, vector<u_int> mips, bool gpu, CPUType_t cpu_type, MachineId_t machine_id);
    void TaskAdd(TaskId_t task_id, VMId_t vm_id);
    void TaskFinish(unsigned core_id);
    void TaskRemove(TaskId_t task_id, VMId_t vm_id);
    void TaskRun(TaskId_t task_id, VMId_t vm_id, unsigned core_id);

    void AttachVM(VMId_t vm_id);
    void DetachVM(VMId_t vm_id);
    uint64_t GetEnergy();
    CPUType_t GetMachineCPUType()                   { return machineInfo.cpu; }
    MachineState_t GetState()                       { return sState; }
    void HandleTimer();

    bool IsReady()                                  { return sState == S0; }
    bool MemoryOverflow()                           { return machineInfo.memory_used > machineInfo.memory_size; }
    void Migrate(VMId_t vm_id);
    void SetPerformance(CPUPerformance_t p_state);
    void SetState(MachineState_t s_state);
    void ScheduleMigrationStart(VMId_t vm_id);
    MachineInfo_t GetInfo();
    
private:
    // Tasks & Scheduling
    queue<Job> runQueue[PRIORITY_LEVELS];
    Time_t nextTimer;
    // CPUs
    vector<CPU> cores;
    // Memory
    void UpdateMemory(int memory);
    unsigned memorySlowdown;
    // Energy
    void ComputeEnergy();
    Time_t startMeter;                     // For tracking energy consumption
    MachineState_t sState;                 // To identify the current energy consumption state
    MachineState_t nextState;              // The next s_state
    bool stateChangeScheduled;             // To indicate that the state needs to change at the next quantum
    unsigned stateChangeCounter;           // To define when the state change takes place
    vector<unsigned> sStates;              // The power consumption depending on the s state
    uint64_t energyConsumed;               // Counter for the consumed energy
    void SetNewState(MachineState_t s_state); 
    // Reporting
    MachineInfo_t machineInfo;
};

#endif /* Machine_hpp */
