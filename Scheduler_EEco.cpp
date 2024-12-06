//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
//

#include "Scheduler.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

static bool migrating = false;
static unsigned active_machines = 16;

// Managing machines
vector<vector<MachineId_t>> machine_matrix;

unsigned total_machines;

// Gets the total utilization of a task
double task_utilization(TaskId_t task_id, MachineId_t machine_id, Time_t curr) {
    TaskInfo_t info = GetTaskInfo(task_id);

    double remaining_instr = (double) info.remaining_instructions;
    double time_frame = (double) info.target_completion - curr;

    double eff_mips = remaining_instr / time_frame;

    MachineInfo_t machine_info = Machine_GetInfo(machine_id);
    double actual_mips = machine_info.performance[machine_info.p_state] * machine_info.num_cpus;
    
	return eff_mips / actual_mips;
}

// Gets the total utilization of a vm
double vm_utilization(VMId_t vm_id, MachineId_t machine_id, Time_t curr) {
    double eff_mips = 0.0;

    for (TaskId_t task_id : VM_GetInfo(vm_id).active_tasks) {
        TaskInfo_t info = GetTaskInfo(task_id);

        double remaining_instr = (double) info.remaining_instructions;
        double time_frame = (double) info.target_completion - curr;

        eff_mips += remaining_instr / time_frame;
    }

    MachineInfo_t machine_info = Machine_GetInfo(machine_id);
    double actual_mips = machine_info.performance[machine_info.p_state] * machine_info.num_cpus;
    
	return eff_mips / actual_mips;
}

// Gets the total utilization of a machine
double machine_utilization(MachineId_t machine_id, Time_t curr) {
	double eff_mips = 0.0;

    for (VMId_t vm_id : machine_matrix[machine_id]) {
        for (TaskId_t task_id : VM_GetInfo(vm_id).active_tasks) {
            TaskInfo_t info = GetTaskInfo(task_id);

            double remaining_instr = (double) info.remaining_instructions;
            double time_frame = (double) info.target_completion - curr;

            eff_mips += remaining_instr / time_frame;
        }
    }

    MachineInfo_t machine_info = Machine_GetInfo(machine_id);
    double actual_mips = machine_info.performance[machine_info.p_state] * machine_info.num_cpus;
    
	return eff_mips / actual_mips;
}

// Converts SLA to Priority
Priority_t sla_to_prio(SLAType_t sla) {
    if (sla == SLA0 || sla == SLA1 || sla == SLA2) {
        return HIGH_PRIORITY;
    } 
    return LOW_PRIORITY;
}

// Managing states
vector<MachineId_t> running;
vector<MachineId_t> idle;
vector<MachineId_t> off;

const MachineState_t RUNNING_S_STATE = S0;
const MachineState_t IDLE_S_STATE = S3;
const MachineState_t OFF_S_STATE = S5;

struct stateChangeInfo {
    MachineState_t old_state;
    MachineState_t new_state;
    TaskId_t task;
};

unordered_map<unsigned, stateChangeInfo> idle_adjust_set;
unordered_map<unsigned, stateChangeInfo> changing_state;

// For Idle set adjustment
const uint64_t SECOND = 1000000;
unsigned desired_idle_set_size;

uint64_t global_total_instr = 0;
uint64_t prev_total_instr = 0;


// Find suitable VM (or create one) on Machine for Task
void FindVMAddTask(MachineId_t id, TaskId_t task_id) {
    MachineInfo_t machine = Machine_GetInfo(id);
    TaskInfo_t task = GetTaskInfo(task_id);

    for (VMId_t vm : machine_matrix[id]) {
        if (task.required_vm == VM_GetInfo(vm).vm_type) {
            VM_AddTask(vm, task_id, sla_to_prio(task.required_sla));
            return;
        }
    }

    VMId_t new_vm = VM_Create(task.required_vm, machine.cpu);
    VM_Attach(new_vm, id);
    machine_matrix[id].push_back(new_vm);

    VM_AddTask(new_vm, task_id, sla_to_prio(task.required_sla));
    return;
}

// dev notes: data type issues
void AdjustIdleSet(double alpha) {
    // 1. Update the desired set by same % as % change in prev util with curr util
    // 2. If desired set is more than a certain % diff, enact change

    desired_idle_set_size *= 1.0 + alpha;

    double percent_change = abs((int) desired_idle_set_size - (int) idle.size());
    if (percent_change > 0.05) {
        int diff = desired_idle_set_size - idle.size();

        if (diff > 0) {
            for (int i = 0; i < max(diff, (int) off.size()); i++) {
                // dev notes: state change issues?
                Machine_SetState(off[i], IDLE_S_STATE);
                changing_state[off[i]] = {OFF_S_STATE, IDLE_S_STATE, 0};
            }
        } else {
            diff = -diff;
            for (int i = 0; i < max(diff, (int) idle.size()); i++) {
                // dev notes: state change issues?
                Machine_SetState(idle[i], OFF_S_STATE);
                changing_state[idle[i]] = {IDLE_S_STATE, OFF_S_STATE, 0};
            }
        }
    }
}


void Scheduler::Init() {
    // Find the parameters of the clusters
    // Get the total number of machines
    // For each machine:
    //      Get the type of the machine
    //      Get the memory of the machine
    //      Get the number of CPUs
    //      Get if there is a GPU or not
    
    total_machines = Machine_GetTotal();

    // first test, half active / half idle
    for (int i = 0; i < total_machines / 2; i++) {
        // dev notes: state change issues?
        Machine_SetState(i, IDLE_S_STATE);
        idle.push_back(i);
    }

    for (int i = total_machines / 2; i < total_machines; i++) {
        running.push_back(i);
    }

    desired_idle_set_size = idle.size();
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    TaskInfo_t task = GetTaskInfo(task_id);
    
    // dev notes: update "running" and "idle" to account for machines
    //            of diff types and gpu capability
    for (MachineId_t id : running) {
        MachineInfo_t machine = Machine_GetInfo(id);

        double machine_util = machine_utilization(id, now);
        double task_util = task_utilization(task_id, id, now);

        bool correct_cpu = task.required_cpu == machine.cpu;
        bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;
        bool enough_util = machine_util + task_util <= 1.0;

        // dev notes: data struct to keep track of vm by vm_type?
        if (correct_cpu && enough_mem && enough_util) {
            FindVMAddTask(id, task_id);
            return;
        }
    }

    for (MachineId_t id : idle) {
        MachineInfo_t machine = Machine_GetInfo(id);

        double machine_util = machine_utilization(id, now);
        double task_util = task_utilization(task_id, id, now);

        bool correct_cpu = task.required_cpu == machine.cpu;
        bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;

        // dev notes: state change issues? (if idle is moving around)
        if (correct_cpu && enough_mem) {
            // dev notes: state change issues?
            Machine_SetState(id, RUNNING_S_STATE);
            changing_state[id] = {IDLE_S_STATE, RUNNING_S_STATE, task_id};
            return;
        }
    }
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary

    if (now % SECOND == 0) {
        if (prev_total_instr == 0) {
            prev_total_instr = global_total_instr;
            global_total_instr = 0;
        } else {
            // dev notes: data type issues?
            double g = (double) global_total_instr;
            double p = (double) prev_total_instr;
            double alpha = (g - p) / p;
            AdjustIdleSet(alpha);
        }
    }
}

void Scheduler::Shutdown(Time_t time) {
    // Do your final reporting and bookkeeping here.
    // Report about the total energy consumed
    // Report about the SLA compliance
    // Shutdown everything to be tidy :-)
    for(auto & vm: vms) {
        VM_Shutdown(vm);
    }
    SimOutput("SimulationComplete(): Finished!", 4);
    SimOutput("SimulationComplete(): Time is " + to_string(time), 4);
}

void Scheduler::TaskComplete(Time_t now, TaskId_t task_id) {
    // Do any bookkeeping necessary for the data structures
    // Decide if a machine is to be turned off, slowed down, or VMs to be migrated according to your policy
    // This is an opportunity to make any adjustments to optimize performance/energy
    SimOutput("Scheduler::TaskComplete(): Task " + to_string(task_id) + " is complete at " + to_string(now), 4);
}

// Public interface below

static Scheduler Scheduler;

void InitScheduler() {
    SimOutput("InitScheduler(): Initializing scheduler", 4);
    Scheduler.Init();
}

void HandleNewTask(Time_t time, TaskId_t task_id) {
    SimOutput("HandleNewTask(): Received new task " + to_string(task_id) + " at time " + to_string(time), 4);
    Scheduler.NewTask(time, task_id);
}

void HandleTaskCompletion(Time_t time, TaskId_t task_id) {
    SimOutput("HandleTaskCompletion(): Task " + to_string(task_id) + " completed at time " + to_string(time), 4);
    Scheduler.TaskComplete(time, task_id);
}

void MemoryWarning(Time_t time, MachineId_t machine_id) {
    // The simulator is alerting you that machine identified by machine_id is overcommitted
    SimOutput("MemoryWarning(): Overflow at " + to_string(machine_id) + " was detected at time " + to_string(time), 0);
}

void MigrationDone(Time_t time, VMId_t vm_id) {
    // The function is called on to alert you that migration is complete
    SimOutput("MigrationDone(): Migration of VM " + to_string(vm_id) + " was completed at time " + to_string(time), 4);
    Scheduler.MigrationComplete(time, vm_id);
    migrating = false;
}

void SchedulerCheck(Time_t time) {
    // This function is called periodically by the simulator, no specific event
    SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time), 4);
    Scheduler.PeriodicCheck(time);
    // static unsigned counts = 0;
    // counts++;
    // if(counts == 10) {
    //     migrating = true;
    //     VM_Migrate(1, 9);
    // }
}

void SimulationComplete(Time_t time) {
    // This function is called before the simulation terminates Add whatever you feel like.
    cout << "SLA violation report" << endl;
    cout << "SLA0: " << GetSLAReport(SLA0) << "%" << endl;
    cout << "SLA1: " << GetSLAReport(SLA1) << "%" << endl;
    cout << "SLA2: " << GetSLAReport(SLA2) << "%" << endl;     // SLA3 do not have SLA violation issues
    cout << "Total Energy " << Machine_GetClusterEnergy() << "KW-Hour" << endl;
    cout << "Simulation run finished in " << double(time)/1000000 << " seconds" << endl;
    SimOutput("SimulationComplete(): Simulation finished at time " + to_string(time), 4);
    
    Scheduler.Shutdown(time);
}

void SLAWarning(Time_t time, TaskId_t task_id) {
    
}

void StateChangeComplete(Time_t time, MachineId_t machine_id) {
    stateChangeInfo info = changing_state[machine_id];
    
    // either idle machine or off machine woken for task assignment
    if (info.new_state == RUNNING_S_STATE) {
        FindVMAddTask(machine_id, info.task);
    }

    // dev notes: state change issues?
    if (info.old_state == RUNNING_S_STATE) {
        running.erase(remove(running.begin(), running.end(), machine_id), running.end());
    } else if (info.old_state == IDLE_S_STATE) {
        idle.erase(remove(idle.begin(), idle.end(), machine_id), idle.end());
    } else {
        off.erase(remove(off.begin(), off.end(), machine_id), off.end());
    }

    // dev notes: state change issues?
    if (info.new_state == RUNNING_S_STATE) {
        running.push_back(machine_id);
    } else if (info.old_state == IDLE_S_STATE) {
        idle.push_back(machine_id);
    } else {
        off.push_back(machine_id);
    }
}