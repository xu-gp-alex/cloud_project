//
//  Machine.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//

#include "Machine.hpp"

// Internal Module Definitions
static vector<Machine> Machines;

static void ValidateMachineId(MachineId_t machine_id, string err_msg) {
    if(machine_id >= Machines.size()) {
        ThrowException(err_msg);
    }
}

// External Interface
CPUType_t Machine_GetCPUType(MachineId_t machine_id) {
    ValidateMachineId(machine_id, "Machine_GetCPUType(): Invalid machine id " + to_string(machine_id));
    return Machines[machine_id].GetMachineCPUType();
}

double Machine_GetClusterEnergy() {
    uint64_t total_energy = 0;
    for(auto & machine: Machines) {
        total_energy += machine.GetEnergy();
    }
    return double(total_energy)/double(3600000000000);
}

uint64_t Machine_GetEnergy(MachineId_t machine_id) {
    ValidateMachineId(machine_id, "Machine_GetEnergy(): Invalid machine id " + to_string(machine_id));
    return Machines[machine_id].GetEnergy();
}

MachineInfo_t Machine_GetInfo(MachineId_t machine_id) {
    ValidateMachineId(machine_id, "Machine_GetInfo(): Invalid machine id " + to_string(machine_id));
    return Machines[machine_id].GetInfo();
}

unsigned Machine_GetTotal() {
    return unsigned(Machines.size());
}

void Machine_SetCorePerformance(MachineId_t machine_id, unsigned core_id, CPUPerformance_t p_state) { // Not recommended
    ValidateMachineId(machine_id, "Machine_SetCorePerformance(): Invalid machine id " + to_string(machine_id));
    Machines[machine_id].SetPerformance(p_state);
}

void Machine_SetState(MachineId_t machine_id, MachineState_t s_state) {
    ValidateMachineId(machine_id, "Machine_SetState(): Invalid machine id " + to_string(machine_id));
    Machines[machine_id].SetState(s_state);
}

// Internal Interface
void Machine_Add(u_int mem, u_int cores, vector<u_int> & s_states, vector<u_int> & c_states, vector<u_int> & p_states, vector<u_int> & mips, bool gpu, CPUType_t cpu) {
    static bool timer_initiated = false;
    static unsigned MachineId_gen = 0;
    if(timer_initiated == false) {
        ScheduleTimer(QUANTUM);
        timer_initiated = true;
    }
    MachineId_t machine_id = MachineId_gen++;
    Machines.push_back(Machine(mem, cores, s_states, c_states, p_states, mips, gpu, cpu, machine_id));
}

void Machine_AttachTask(MachineId_t machine_id, TaskId_t task_id, VMId_t vm_id) {
    ValidateMachineId(machine_id, "Machine_AttachTask(): Invalid machine id " + to_string(machine_id));
    SimOutput("AttachTask(): Attaching Task " + to_string(task_id) + " to machine " + to_string(machine_id) + " at time " + to_string(Now()), 4);
    Machines[machine_id].TaskAdd(task_id, vm_id);
}

void Machine_AttachVM(MachineId_t machine_id, VMId_t vm_id) {
    ValidateMachineId(machine_id, "Machine_AttachVM(): Invalid machine id " + to_string(machine_id));
    SimOutput("AttachVM(): Attaching VM " + to_string(vm_id) + " to machine " + to_string(machine_id), 4);
    Machines[machine_id].AttachVM(vm_id);
}

void Machine_CompleteTask(MachineId_t machine_id, unsigned core_id) {
    ValidateMachineId(machine_id, "Machine_CompleteTask(): Invalid machine id " + to_string(machine_id));
    Machines[machine_id].TaskFinish(core_id);
}

bool Machine_CheckMemoryOverflow(MachineId_t machine_id) {
    ValidateMachineId(machine_id, "Machine_CheckMemoryOverflow(): Invalid machine id " + to_string(machine_id));
    return Machines[machine_id].MemoryOverflow();
}

void Machine_DetachVM(MachineId_t machine_id, VMId_t vm_id) {
    ValidateMachineId(machine_id, "Machine_DetachVM(): Invalid machine id " + to_string(machine_id));
    SimOutput("DetachVM(): " + to_string(vm_id) + " underway",4);
    Machines[machine_id].DetachVM(vm_id);
}

void Machine_HandleTimer(Time_t now) {
    SimOutput("HandleTimer() called at time " + to_string(now), 4);
    for(auto & machine: Machines) {
        machine.HandleTimer();
    }
    if(GetActiveTasks() > 0) {
        ScheduleTimer(Now() + QUANTUM);
    }
    SchedulerCheck(Now());
}

void Machine_MigrateVM(VMId_t vm_id, MachineId_t current, MachineId_t next) {
    ValidateMachineId(current, "MigrateVM(): Invalid machine id " + to_string(current));
    ValidateMachineId(next, "MigrateVM(): Invalid machine id " + to_string(next));
    if(!Machines[current].IsReady()) {
        ThrowException("MigrateVM(): Trying to migrate VM " + to_string(vm_id) + " from machine " + to_string(current) + " while the machine is in sleep mode");
    }
    if(!Machines[next].IsReady()) {
        ThrowException("MigrateVM(): Trying to migrate VM " + to_string(vm_id) + " to machine " + to_string(current) + " while the machine is in sleep mode");
    }
    Machines[current].Migrate(vm_id);
}

// Object implementation

CPU::CPU(vector<unsigned> & p_states, vector<unsigned> & c_states, vector<unsigned> & perf, bool gpu_flag, CPUId_t cpu_id) {
    startMeter     = 0;
    startRun       = 0;
    energyConsumed = 0;
    cState         = C1;
    pState         = P0;
    gpuFlag        = gpu_flag;
    pStates        = p_states;
    cStates        = c_states;
    performance    = perf;
    cpuId          = cpu_id;
}

void CPU::ComputeEnergy() {
    Time_t now = Now();
    if(cState == C0) {
        energyConsumed += (now - startMeter) * pStates[pState];
    }
    else {
        energyConsumed += (now - startMeter) * cStates[cState];
    }
    startMeter = Now();
}

void CPU::SetCState(CPUState_t c_state) {
    if(cState == C0) {
        ThrowException("Machine::CPU::SetState(): Fatal error, CPU cannot go idle while running a job!");
    }
    ComputeEnergy();
    cState = c_state;
}

void CPU::SetPState(CPUPerformance_t p_state) {
    ComputeEnergy();
    pState = p_state;
}

void CPU::TaskRun(Job & job, unsigned slow_down, Time_t next_timer) {
    SimOutput("CPU:TaskRun(): Now " + to_string(Now()), 4);
    SimOutput("CPU:TaskRun(): Slowdown " + to_string(slow_down) + " " + " next timer " + to_string(next_timer), 4);
    if(cState == C0) {
        ThrowException("Machine::CPU::TaskRun(): Fatal error, CPU was already in C0 state!");
    }
    ComputeEnergy();
    cState          = C0;
    jobRunning      = job;
    
    uint64_t remaining = GetRemainingInstructions(job.task_id);
    uint64_t timeq     = uint64_t(next_timer - Now());
    uint64_t mips      = uint64_t(performance[pState]) * 100 / slow_down;
    instrToRun         = mips * timeq;
    SimOutput("CPU:TaskRun(): Instr to run  " + to_string(instrToRun), 4);

    instrToRun = gpuFlag && IsTaskGPUCapable(job.task_id) ? instrToRun * 20 : instrToRun;
    SimOutput("CPU:TaskRun(): Remaining " + to_string(remaining) + " " + " instr to run  " + to_string(instrToRun), 4);
    SimOutput("CPU:TaskRun(): Performance parameter was " + to_string(performance[pState]), 4);

    if(instrToRun > remaining) {
        uint64_t time_to_run = (timeq * remaining)/instrToRun;
        if(time_to_run == 0) {
            time_to_run = 1;
        }
        projectedFinish = Now() + time_to_run;
        SimOutput("CPU:TaskRun(): Timeq is " + to_string(timeq), 4);

        instrToRun = remaining;
        SimOutput("CPU:TaskRun(): Positive, Projected finish " + to_string(projectedFinish) + " " + " instr to run  " + to_string(instrToRun), 4);
    }
    else {
        projectedFinish = next_timer;
        SimOutput("CPU:TaskRun(): Negatove, Projected finish " + to_string(projectedFinish) + " " + " instr to run  " + to_string(instrToRun), 4);
    }
}

void CPU::TaskStop() {
    if(cState != C0) {
        ThrowException("Machine::CPU::TaskStop(): Fatal error, stopping a CPU that was not in C0 state!");
    }
    ComputeEnergy();
    cState = C1;
    
    uint64_t remaining = GetRemainingInstructions(jobRunning.task_id);
    SetRemainingInstructions(jobRunning.task_id, remaining - instrToRun);
}

Machine::Machine(u_int mem, u_int cpus, vector<u_int> s_states, vector<u_int> c_states, vector<u_int> p_states, vector<u_int> mips, bool gpu, CPUType_t cpu_type, MachineId_t machine_id) {
    memorySlowdown       = 100;
    sState               = S0;
    sStates              = s_states;
    startMeter           = 0;
    energyConsumed       = 0;
    stateChangeScheduled = false;
    stateChangeCounter   = 0;
    nextTimer            = QUANTUM;
    
    // Add CPUs
    for(unsigned i = 0; i < cpus; i++) {
        cores.push_back(CPU(p_states, c_states, mips, gpu, i));
    }
    machineInfo.num_cpus        = cpus;
    machineInfo.cpu             = cpu_type;
    machineInfo.memory_size     = mem;
    machineInfo.memory_used     = 0;
    machineInfo.active_tasks    = 0;
    machineInfo.active_vms      = 0;
    machineInfo.gpus            = gpu;
    machineInfo.energy_consumed = 0;
    machineInfo.performance     = mips;
    machineInfo.p_states        = p_states;
    machineInfo.c_states        = c_states;
    machineInfo.s_state         = S0;
    machineInfo.p_state         = P0;
    machineInfo.machine_id      = machine_id;
}

unsigned num_cpus;                      // Number of CPU's on the machine
CPUType_t cpu;                          // CPU types deployed in the machine
unsigned memory_size;                   // Size of memory
unsigned memory_used;                   // The memory currently in use
unsigned active_tasks;                  // Number of tasks that are assigned to this machine
unsigned active_vms;                    // Number of virtual machines that are attached to this machine
bool gpus;                              // True if the processors are equipped with a GPU, false otherwise
uint64_t energy_consumed;               // How much energy has been consumed so far
vector<unsigned> performance;           // The MIPS ratings for the CPUs at different p-state
vector<unsigned> c_states;              // Power consumption under different C states
vector<unsigned> s_states;              // Machine power consumption under different S states
MachineState_t s_state;                 // The current S state of the machine
CPUState_t p_state;                     // The current P state of the CPU


void Machine::TaskAdd(TaskId_t task_id, VMId_t vm_id) {
    machineInfo.active_tasks++;
    Job job;
    job.task_id = task_id;
    job.vm_id   = vm_id;
    UpdateMemory(GetTaskMemory(task_id));
    SimOutput("Machine::AttachTask(): Memory used is " + to_string(machineInfo.memory_used), 4);
    for(auto & core : cores) {
        if(!core.IsBusy()) {
            TaskRun(task_id, vm_id, core.GetId());
            return;
        }
    }
    runQueue[GetTaskPriority(task_id)].push(job);
}

void Machine::TaskFinish(unsigned core_id) {
    Job job = cores[core_id].GetJob();
    SimOutput("Machine::TaskFinish(): About to remove task_id " + to_string(job.task_id) , 4);
    cores[core_id].TaskStop();
    TaskRemove(job.task_id, job.vm_id);
    for(auto & queue: runQueue) {
        while(!queue.empty()) {
            job = queue.front();
            queue.pop();
            TaskRun(job.task_id, job.vm_id, core_id);
            return;
        }
    }
}

void Machine::TaskRemove(TaskId_t task_id, VMId_t vm_id) {
    ComputeEnergy();
    machineInfo.active_tasks--;
    UpdateMemory(-GetTaskMemory(task_id));
    SimOutput("Machine::TaskRemove(): About to remove task_id " + to_string(task_id) , 4);
    VM_RemoveTask(vm_id, task_id);
    SimOutput("Machine::TaskRemove(): Checking migration", 4);
    if(VM_IsPendingMigration(vm_id)) {
        Migrate(vm_id);
    }
    SimOutput("Machine::TaskRemove(): Checked migration", 4);
    CompleteTask(task_id);
    HandleTaskCompletion(Now(), task_id);
}

void Machine::TaskRun(TaskId_t task_id, VMId_t vm_id, unsigned core_id) {
    Job job;
    job.task_id = task_id;
    job.vm_id = vm_id;
    cores[core_id].TaskRun(job, memorySlowdown, nextTimer);
    SimOutput("Machine::TaskRun(): About to test next timer versus next", 4);
    Time_t next = cores[core_id].GetProjectedFinish();
    SimOutput("Machine::TaskRun(): About to test! Next timer " + to_string(nextTimer) + " and projected finish " + to_string(next), 4);

    if(next < nextTimer) {
        ScheduleTaskCompletion(next, machineInfo.machine_id, core_id);
    }
}

void Machine::AttachVM(VMId_t vm_id) {
    if(sState != S0) {
        ThrowException("Machine::AttachVM(): Attempt at attaching virtual machine " + to_string(vm_id) + " to machine " + to_string(machineInfo.machine_id) + " while in sleep mode");
    }
    UpdateMemory(VM_MEMORY_OVERHEAD);
    machineInfo.active_vms++;
}

void Machine::DetachVM(VMId_t vm_id) {
    // Check if there are tasks still running
    if(sState != S0) {
        ThrowException("Machine::DetachVM(): Attempt at dettaching virtual machine " + to_string(vm_id) + " from machine " + to_string(machineInfo.machine_id) + " while in sleep mode");
    }
    for(auto & core: cores) {
        if(core.IsBusy() && core.GetJob().vm_id == vm_id) {
            ThrowException("Machine::DettachVM(): Attempt at dettaching virtual machine " + to_string(vm_id) + " from machine " + to_string(machineInfo.machine_id) + " while tasks running");
        }
    }
    for(auto & queue: runQueue) {
        unsigned long numTasks = queue.size();
        for(unsigned i = 0; i < numTasks; i++) {
            Job job = queue.front();
            queue.pop();
            if(job.vm_id == vm_id) {
                ThrowException("Machine::DettachVM(): Attempt at dettaching virtual machine " + to_string(vm_id) + " from machine " + to_string(machineInfo.machine_id) + " while tasks queued");
            }
            queue.push(job);
        }
    }
    UpdateMemory(-VM_MEMORY_OVERHEAD);
    machineInfo.active_vms--;
}

void Machine::HandleTimer() {
    queue<Job> pending_removal;
    ComputeEnergy();
    nextTimer += QUANTUM;

    // Stop all tasks as part of the context switch
    SimOutput("Machine::HandleTimer(): About to remove tasks from processor", 4);
    if(sState == S0) {
        for(auto & core: cores) {
            if(core.IsBusy()) {
                SimOutput("Machine::HandleTimer(): About to remove a task", 4);
                Job job = core.GetJob();
                core.TaskStop();
                if(IsTaskCompleted(job.task_id)) {
                    pending_removal.push(job);
                }
                else {
                    runQueue[GetTaskPriority(job.task_id)].push(job);
                }
            }
        }
    }
    SimOutput("Machine::HandleTimer(): Done removing tasks", 4);
    bool inform_scheduler = false;
    if(stateChangeScheduled) {
        stateChangeCounter--;
        if(stateChangeCounter == 0) {
            stateChangeScheduled = false;
            inform_scheduler = true;
            SetNewState(nextState);
        }
    }
    SimOutput("Machine::HandleTimer(): About to run tasks", 4);
    if(sState == S0) {
        unsigned assigned_cores = 0;
        for(auto & queue: runQueue) {
            while(!queue.empty() && assigned_cores < cores.size()) {
                auto job = queue.front();
                queue.pop();
                SimOutput("Machine::HandleTimer(): Running a task", 4);
                SimOutput("Trying with core " + to_string(assigned_cores), 4);
                cores[assigned_cores].IsBusy()? SimOutput("Core is busy!", 4) : SimOutput("Core is no longer busy", 4);
                TaskRun(job.task_id, job.vm_id, assigned_cores);
                assigned_cores++;
             }
        }
    }
    while(!pending_removal.empty()) {
        Job job = pending_removal.front();
        pending_removal.pop();
        TaskRemove(job.task_id, job.vm_id);
    }
    if(inform_scheduler) {
        StateChangeComplete(Now(), machineInfo.machine_id);
    }
}

void Machine::Migrate(VMId_t vm_id) {
    bool migration_possible = true;
    Job job;
    for(auto & core: cores) {
        job = core.GetJob();
        if(job.vm_id == vm_id && core.IsBusy()) {
            if(core.GetProjectedFinish() < nextTimer) {
                migration_possible = false;
                SimOutput("Machine::Migrate(): Task is finishing. Postponing migration", 4);
            }
            else {
                core.TaskStop();
                machineInfo.active_tasks--;
                SimOutput("Machine::Migrate(): Removed task from CPU due to migration.", 4);
            }
        }
    }
    for(auto & queue: runQueue) {
        unsigned long jobs = queue.size();
        for(unsigned i = 0; i < jobs; i++) {
            auto job = queue.front();
            queue.pop();
            if(job.vm_id != vm_id) {
                queue.push(job);
            }
            else {
                machineInfo.active_tasks--;
                SimOutput("Machine::Migrate(): Removed task from the run queue due to migration.", 4);
            }
        }
    }
    if(migration_possible) {
        SimOutput("Machine::Migrate(): Migration is possible", 4);
        UpdateMemory(-VM_MEMORY_OVERHEAD);
        VM_MigrationStarted(vm_id);
        machineInfo.active_vms--;
        ScheduleMigrationCompletion(Now() + 500 * QUANTUM, vm_id);
    }
}

uint64_t Machine::GetEnergy() {
    ComputeEnergy();
    uint64_t cpus_energy = 0;
    for(auto & core: cores) {
        cpus_energy += core.GetEnergy();
    }
    return energyConsumed + cpus_energy;
}

void Machine::ComputeEnergy() {
    energyConsumed += sStates[sState] * (Now() - startMeter);
    startMeter = Now();
}

void Machine::SetNewState(MachineState_t s_state) {
    static CPUState_t s_to_c[S_STATES] = { C1, C1, C2, C4, C4, C4, C4};
    ComputeEnergy();
    sState = s_state;
    for(auto & core: cores) {
        core.SetCState(s_to_c[sState]);
    }
}

void Machine::SetState(MachineState_t s_state) {
    static unsigned transitions[S_STATES][S_STATES] = {
        {    0,    1,    1,   10,   25,   50, 250},
        {    1,    0,    5,   20,   20,   50, 150},
        {    5,    1,    0,   10,   20,   50, 150},
        {   50,   75,   20,    0,   20,   50, 150},
        {  100,   80,   50,   20,    0,   50, 150},
        {  200,  150,  100,  100,   50,    0, 150},
        { 5000, 4000, 3000, 3000, 3000, 3000,   0}};
    if(sState != s_state) {
        stateChangeScheduled = true;
        stateChangeCounter = transitions[sState][s_state];
        nextState = s_state;
    }
    else {
        StateChangeComplete(Now(), machineInfo.machine_id);
    }
}

void Machine::SetPerformance(CPUPerformance_t p_state) {
    for(auto & core: cores) {
        core.SetPState(p_state);
    }
    machineInfo.p_state = p_state;
}

void Machine::UpdateMemory(int memory) {
    machineInfo.memory_used += memory;
    if(machineInfo.memory_used > machineInfo.memory_size) {
        MemoryWarning(Now(), machineInfo.machine_id);
        if(machineInfo.memory_used > machineInfo.memory_size * 2) {
            memorySlowdown = 400;
        }
        else {
            memorySlowdown = 200;
        }
    }
    else {
        memorySlowdown = 100;
    }
}

MachineInfo_t Machine::GetInfo() {
    machineInfo.energy_consumed = GetEnergy();
    machineInfo.s_state         = sState;
    return machineInfo;
}
