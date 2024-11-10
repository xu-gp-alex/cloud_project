//
//  CPU.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//

#include "CPU.hpp"

CPU::CPU(CPUType_t type, std::vector<unsigned> cpu_dynamic, std::vector<unsigned> cpu_static, std::vector<unsigned> perf, bool gpu, CPUId_t id) {
    cpu_type        = type;
    start_meter     = 0;
    start_run       = 0;
    energy_consumed = 0;
    slow_down       = 100;
    c_state         = C1;
    p_state         = P0;
    gpu_flag        = gpu;
    dynamic_power   = cpu_dynamic;
    static_power    = cpu_static;
    performance     = perf;
    identifier      = id;
}

void CPU::TaskRun(TaskId_t t_id) {
    if(c_state == C0) {
        ThrowException("CPU::TaskRun(): Fatal error, CPU was already in C0 state!");
    }
    start_run = Now();
    // SetState(C0, Now());
    c_state = C0;
    task_id = t_id;
}

void CPU::TaskStop() {
    if(c_state != C0) {
        ThrowException("CPU::TaskStop(): Fatal error, stopping a CPU that was not in C0 state!");
    }
    //SetState(C1, Now());
    c_state = C1;
    unsigned effective_mips = (performance[p_state] / slow_down) * 100;
    unsigned instructions = unsigned((Now() - start_run)) * effective_mips;
    unsigned remaining = GetRemainingInstructions(task_id);
    if(remaining <= instructions) {
        CompleteTask(task_id);  // Task has completed
    }
    else {
        SetRemainingInstructions(task_id, remaining - instructions);
    }
}

void CPU::TaskAbort() {
    if(c_state != C0) {
        ThrowException("CPU::TaskStop(): Fatal error, stopping a CPU that was not in C0 state!");
    }
    //SetState(C1, Now());
    c_state = C1;
}

#ifdef IMPLEMENTED
Time_t CPU::Attach(Time_t now, unsigned remaining_instructions) {
    if(c_state == C0) {
        throw(std::string("Fatal error in Attach: CPU was already in C0 state!"));
    }
    
    SetState(C0, now);
    
    start_run = now;
 
    unsigned effective_mips = performance[p_state] / slow_down;
    return now + remaining_instructions/effective_mips + (remaining_instructions % effective_mips != 0);
}

unsigned CPU::Detach(Time_t now) {
    if(c_state != C0) {
        throw(std::string("Fatal error in Detach: CPU was not in C0 state!"));
    }
    ComputeEnergy(now);
    c_state = C1;       // We are idle
    
    return unsigned((now - start_run)) * performance[p_state] / slow_down;
}

void CPU::SetState(CPUState_t cstate, Time_t now) {
    ComputeEnergy(now);
    c_state = cstate;
}

void CPU::SetPerformance(CPUPerformance_t pstate, Time_t now) {
    ComputeEnergy(now);
    p_state = pstate;
}


#endif
unsigned long long CPU::GetEnergy(Time_t now) {
    ComputeEnergy(now);
    return energy_consumed;
}

void CPU::ComputeEnergy(Time_t now) {
    if(c_state == C0) {
        energy_consumed += (now - start_meter) * dynamic_power[p_state];
    }
    else {
        energy_consumed += (now - start_meter) * static_power[c_state];
    }
    start_meter = now;
}

static vector<CPU> CPUs;
static CPUId_t CPUId_gen = 0;

static void ValidateCPUId(CPUId_t cpu_id, string err_msg) {
    if(cpu_id >= CPUs.size()) {
        ThrowException(err_msg);
    }
}

CPUId_t AddCPU(CPUType_t cpu, vector<unsigned>cpu_dynamic, vector<unsigned> cpu_static, vector<unsigned> performance, bool gpu_flag) {
    CPUId_t cpu_id = CPUId_gen++;
    CPUs.push_back(CPU(cpu, cpu_dynamic, cpu_static, performance, gpu_flag, cpu_id));
    return cpu_id;
}

CPUType_t GetCPUType(CPUId_t cpu_id) {
    ValidateCPUId(cpu_id, "GetCPUType(): Invalid CPU Id" + to_string(cpu_id));
    return CPUs[cpu_id].GetType();
}

void CPU_AbortTask(CPUId_t cpu_id) {
    ValidateCPUId(cpu_id, "CPU_AbortTask(): Invalid CPU Id " + to_string(cpu_id));
    CPUs[cpu_id].TaskAbort();
}

void CPU_RunTask(CPUId_t cpu_id, TaskId_t task_id) {
    ValidateCPUId(cpu_id, "CPU_RunTask(): Invalid CPU Id " + to_string(cpu_id));
    CPUs[cpu_id].TaskRun(task_id);
}

void CPU_StopTask(CPUId_t cpu_id) {
    ValidateCPUId(cpu_id, "CPU_StopTask(): Invalid CPU Id " + to_string(cpu_id));
    CPUs[cpu_id].TaskStop();
}

long long GetConsumedEnergy(CPUId_t cpu_id) {
    ValidateCPUId(cpu_id, "GetConsumedEnergy(): Invalid CPU Id" + to_string(cpu_id));
    ThrowException("GetConsumedEnergy: Unimplemented");
    return 0;
}


