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
#include <math.h>
#include <deque>

// Managing machines
vector<vector<MachineId_t>> machine_matrix;

unsigned total_machines;

// Current mips of task
double task_eff_mips(MachineId_t machine_id, TaskId_t task_id, Time_t curr) {
    TaskInfo_t info = GetTaskInfo(task_id);

    // somehow, remaining_instr is subject to unsigned integer overflow
    if (info.remaining_instructions > info.total_instructions) {
        return 0;
    }

    uint64_t remaining_instr = info.remaining_instructions;
    uint64_t time_frame = info.target_completion - curr;

    double eff_mips = (double) remaining_instr / (double) time_frame;

	return eff_mips;
}

// Current mips of vm
unsigned vm_eff_mips(MachineId_t machine_id, VMId_t vm_id, Time_t curr) {
    unsigned eff_mips = 0;

    for (TaskId_t task_id : VM_GetInfo(vm_id).active_tasks) {
        TaskInfo_t info = GetTaskInfo(task_id);

        uint64_t remaining_instr = info.remaining_instructions;
        uint64_t time_frame = info.target_completion - curr;

        eff_mips += remaining_instr / time_frame;
    }
    
	return eff_mips;
}

// Current mips of machine
double machine_eff_mips(MachineId_t machine_id, Time_t curr) {
    // unsigdobuned eff_mips = 0;
    double eff_mips = 0.0;

    for (VMId_t vm_id : machine_matrix[machine_id]) {
        for (TaskId_t task_id : VM_GetInfo(vm_id).active_tasks) {
            TaskInfo_t info = GetTaskInfo(task_id);

            uint64_t remaining_instr = info.remaining_instructions;
            uint64_t time_frame = info.target_completion - curr;

            // somehow, remaining_instr is subject to unsigned integer overflow
            if (remaining_instr > info.total_instructions) {
                continue;
            }

            eff_mips += (double) remaining_instr / (double) time_frame;
        }
    }
    
    return eff_mips;
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
const MachineState_t IDLE_S_STATE = S1;
const MachineState_t OFF_S_STATE = S2;

struct stateChangeInfo {
    MachineState_t old_state;
    MachineState_t new_state;
    bool attach_task;
    TaskId_t task;
};

unordered_map<unsigned, stateChangeInfo> idle_adjust_set;
unordered_map<unsigned, stateChangeInfo> changing_state;

// For Idle set adjustment
const uint64_t SECOND = 1000000;
unsigned desired_idle_set_size;

uint64_t last_time = 0;

deque<unsigned> queue;

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


void Scheduler::Init() {
    // Find the parameters of the clusters
    // Get the total number of machines
    // For each machine:
    //      Get the type of the machine
    //      Get the memory of the machine
    //      Get the number of CPUs
    //      Get if there is a GPU or not
    
    total_machines = Machine_GetTotal();

    vector<vector<uint64_t>> machine_by_cpus;
    for(int i = 0 ; i < 4; i++) {
        vector<uint64_t> temp = {};
        machine_by_cpus.push_back(temp);
    }
    
    for(int i = 0; i < total_machines; i++) {
        machine_by_cpus[Machine_GetInfo((MachineId_t) i).cpu].push_back(i);
        vector<MachineId_t> temp = {};
        machine_matrix.push_back(temp);
    }

    for(int i = 0; i < machine_by_cpus.size(); i++) {
        int first_thirty = ceil((double) machine_by_cpus[i].size() * 0.5);
        int first_seventy = ceil((double) machine_by_cpus[i].size() * 0.7);
        for(int j = 0; j < first_thirty; j++) {
            running.push_back(machine_by_cpus[i][j]);
        } 
        for(int j = first_thirty; j < first_seventy; j++) {
            // printf("init to idle machine_id: %d\n", machine_by_cpus[i][j]);
            Machine_SetState(machine_by_cpus[i][j], IDLE_S_STATE);
            changing_state[machine_by_cpus[i][j]] = {RUNNING_S_STATE, IDLE_S_STATE, false, 0};
        }
        for(int j = first_seventy; j < machine_by_cpus[i].size(); j++) {
            // printf("init to off machine_id: %d\n", machine_by_cpus[i][j]);
            Machine_SetState(machine_by_cpus[i][j], OFF_S_STATE);
            changing_state[machine_by_cpus[i][j]] = {RUNNING_S_STATE, OFF_S_STATE, false, 0};
        }
    }
}

void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    TaskInfo_t task = GetTaskInfo(task_id);
    
    // dev notes: update "running" and "idle" to account for machines
    //            of diff types and gpu capability

    // for (MachineId_t id : running) {
    //     cout << machine_eff_mips(id, now) << " ";
    // } cout << endl;

    for (MachineId_t id : running) {
        MachineInfo_t machine = Machine_GetInfo(id);

        // double machine_util = machine_utilization(id, now);
        // double task_util = task_utilization(task_id, id, now);

        double machine_util = machine_eff_mips(id, now);
        double task_util = task_eff_mips(task_id, id, now);
        double machine_max_util = machine.performance[machine.p_state] * machine.num_cpus;

        bool correct_cpu = task.required_cpu == machine.cpu;
        bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;
        bool enough_util = machine_util + task_util <= machine_max_util;

        // dev notes: data struct to keep track of vm by vm_type?
        // dev notes: is the change_state.count required?
        if (!changing_state.count(id) && correct_cpu && enough_mem && enough_util) {
            // if (task_id == 0) printf("task #%u assigned to machine #%u\n", task_id, id);
            FindVMAddTask(id, task_id);
            return;
        }
    }

    for (MachineId_t id : idle) {
        MachineInfo_t machine = Machine_GetInfo(id);

        bool correct_cpu = task.required_cpu == machine.cpu;
        bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;

        // dev notes: state change issues? (if idle is moving around)
        // dev notes: is the change_state.count required?
        if (!changing_state.count(id) && correct_cpu && enough_mem) {
            // dev notes: state change issues?
            // printf("assign to idle machine_id: %d\n", id);
            Machine_SetState(id, RUNNING_S_STATE);
            changing_state[id] = {IDLE_S_STATE, RUNNING_S_STATE, true, task_id};
            return;
        }
    }

    for (MachineId_t id : off) {
        MachineInfo_t machine = Machine_GetInfo(id);

        bool correct_cpu = task.required_cpu == machine.cpu;
        bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;

        // dev notes: state change issues? (if idle is moving around)
        // dev notes: is the change_state.count required?
        if (!changing_state.count(id) && correct_cpu && enough_mem) {
            // dev notes: state change issues?
            // printf("assign to off machine_id: %d\n", id);
            Machine_SetState(id, RUNNING_S_STATE);
            changing_state[id] = {OFF_S_STATE, RUNNING_S_STATE, true, task_id};
            return;
        }
    }


    // printf("task #%u not assigned\n", task_id);
    queue.push_back(task_id);
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
    // about 0.25 sec between checks
    if(now - last_time >= SECOND) {
        last_time = now;

        while(queue.size() > 0) {
            TaskId_t task_id = queue[0];
            TaskInfo_t task = GetTaskInfo(task_id);
            bool done = false;
            for (MachineId_t id : running) {
                MachineInfo_t machine = Machine_GetInfo(id);

                // double machine_util = machine_utilization(id, now);
                // double task_util = task_utilization(task_id, id, now);

                double machine_util = machine_eff_mips(id, now);
                double task_util = task_eff_mips(task_id, id, now);
                double machine_max_util = machine.performance[machine.p_state] * machine.num_cpus;

                bool correct_cpu = task.required_cpu == machine.cpu;
                bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;
                bool enough_util = machine_util + task_util <= machine_max_util;

                // dev notes: data struct to keep track of vm by vm_type?
                // dev notes: is the change_state.count required?
                if (!changing_state.count(id) && correct_cpu && enough_mem && enough_util) {
                    // if (task_id == 0) printf("task #%u assigned to machine #%u\n", task_id, id);
                    FindVMAddTask(id, task_id);
                    queue.pop_front();
                    done = true;
                    break;
                }
            }
            if(done) {
                continue;
            }

            for (MachineId_t id : idle) {
                MachineInfo_t machine = Machine_GetInfo(id);

                bool correct_cpu = task.required_cpu == machine.cpu;
                bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;

                // dev notes: state change issues? (if idle is moving around)
                // dev notes: is the change_state.count required?
                if (!changing_state.count(id) && correct_cpu && enough_mem) {
                    // dev notes: state change issues?
                    // printf("assign to idle machine_id: %d\n", id);
                    Machine_SetState(id, RUNNING_S_STATE);
                    changing_state[id] = {IDLE_S_STATE, RUNNING_S_STATE, true, task_id};
                    queue.pop_front();
                    done = true;
                    break;
                }
            }
            if(done) {
                continue;
            }

            for (MachineId_t id : off) {
                MachineInfo_t machine = Machine_GetInfo(id);

                bool correct_cpu = task.required_cpu == machine.cpu;
                bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;

                // dev notes: state change issues? (if idle is moving around)
                // dev notes: is the change_state.count required?
                if (!changing_state.count(id) && correct_cpu && enough_mem) {
                    // dev notes: state change issues?
                    // printf("assign to off machine_id: %d\n", id);
                    Machine_SetState(id, RUNNING_S_STATE);
                    changing_state[id] = {OFF_S_STATE, RUNNING_S_STATE, true, task_id};
                    queue.pop_front();
                    done = true;
                    break;
                }
            }

            if(done) {
                continue;
            }
            break;
        }

        

        unsigned global_total_mips = 0;
        unsigned global_max_mips = 0;

        // printf("running size: %ld\n", running.size());
        
        for (MachineId_t id : running) {
            MachineInfo_t info = Machine_GetInfo(id);

            global_total_mips += machine_eff_mips(id, now);
            global_max_mips += info.performance[info.p_state] * info.num_cpus;
        }
        
        double global_util = global_max_mips == 0 ? 0 : (double) global_total_mips / (double) global_max_mips;

        // printf("holy: %f\n", global_util);

        if (global_util > 0.7) {
            double theoretical_util = global_util;
            
            unsigned idle_index = 0;
            while (idle_index < idle.size() && theoretical_util > 0.7) {
                MachineId_t id = idle[idle_index];
                if(!changing_state.count(id)) {
                    // printf("periodic (idle --> running), id = #%u\n", id);
                    Machine_SetState(id, RUNNING_S_STATE);
                    changing_state[id] = {IDLE_S_STATE, RUNNING_S_STATE, false, 0};
                    
                    MachineInfo_t info = Machine_GetInfo(id);
                    global_max_mips += info.performance[P0] * info.num_cpus;
                    theoretical_util = global_max_mips == 0 ? 0 : global_total_mips / global_max_mips;
                }

                idle_index++;
            }
    
            //idle change did not solve it
            unsigned off_index = 0;
            while (off_index < off.size() && theoretical_util > 0.7) {
                MachineId_t id = off[off_index];
                if(!changing_state.count(id)) {
                    // printf("periodic (off --> running), id = #%u\n", id);
                    Machine_SetState(id, RUNNING_S_STATE);
                    changing_state[id] = {OFF_S_STATE, RUNNING_S_STATE, false, 0};
                    
                    MachineInfo_t info = Machine_GetInfo(id);
                    global_max_mips += info.performance[P0] * info.num_cpus;
                    theoretical_util = global_max_mips == 0 ? 0 : global_total_mips / global_max_mips;
                }

                off_index++;
            }

            //check if idle size is too small
            int idle_size = idle.size();
            unsigned offset_index = 0;
            while(offset_index < idle_size && idle_size < total_machines * 0.2) {
                MachineId_t id = off[offset_index];
                if(!changing_state.count(id)) {
                    // printf("periodic (off --> idle), id = #%u\n", id);
                    Machine_SetState(id, IDLE_S_STATE);
                    changing_state[id] = {OFF_S_STATE, IDLE_S_STATE, false, 0};
                    idle_size++;
                }

                offset_index++;
            }

        } else if (global_util < 0.3) {

            double theoretical_util = global_util;
            
            unsigned index = 0;
            while (index < running.size() && theoretical_util < 0.3) {
                MachineId_t id = running[index];
                if(machine_eff_mips(id, now) == 0.0) {
                    if (!changing_state.count(id)) {
                        // printf("running -> idle machine id: %d\n", id);
                        Machine_SetState(id, IDLE_S_STATE);
                        changing_state[id] = {RUNNING_S_STATE, IDLE_S_STATE, false, 0};

                        MachineInfo_t info = Machine_GetInfo(id);

                        if (info.performance[P0] * info.num_cpus > global_max_mips) {
                            break;
                        }

                        global_max_mips -= info.performance[P0] * info.num_cpus;

                        theoretical_util = global_max_mips == 0 ? 0 : global_total_mips / global_max_mips;
                    }
                }
                // printf("theortical util: %f\n", theoretical_util);

                index++;
            }

            // printf("are setting running --> off\n");

            //check if idle size is too big
            int idle_size = idle.size();
            unsigned idle_index = 0;
            while(idle_index < idle_size && idle_size > (double) total_machines * 0.5) {
                MachineId_t id = idle[idle_index];
                if(!changing_state.count(id)) {
                    // printf("idle -> off machine id: %d\n", id);
                    Machine_SetState(id, OFF_S_STATE);
                    changing_state[id] = {IDLE_S_STATE, OFF_S_STATE, false, 0};
                    idle_size--;
                }

                idle_index++;
            }
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
    // printf("done with task: %u\n at time %lu", task_id, now);
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
}

void SchedulerCheck(Time_t time) {
    // This function is called periodically by the simulator, no specific event
    SimOutput("SchedulerCheck(): SchedulerCheck() called at " + to_string(time), 4);
    Scheduler.PeriodicCheck(time);
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

    // printf("Machine #%u change from S%d --> S%d\n", machine_id, info.old_state, info.new_state);
    
    // either idle machine or off machine woken for task assignment
    if (info.new_state == RUNNING_S_STATE && info.attach_task) {
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

    changing_state.erase(machine_id);
}