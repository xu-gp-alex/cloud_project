//
//  Scheduler.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/20/24.
// Custom

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
    if (sla == SLA0) {
        return HIGH_PRIORITY;
    } 
    else if(sla == SLA1 || sla == SLA2) {
        return MID_PRIORITY;
    } 
    return LOW_PRIORITY;
}

// Managing states
vector<vector<uint64_t>> running;
vector<vector<uint64_t>> idle;
vector<vector<uint64_t>> off;

const CPUPerformance_t RUNNING_P_STATE = P0;
const MachineState_t RUNNING_S_STATE = S0;
const CPUPerformance_t IDLE_P_STATE = P3;
const MachineState_t OFF_S_STATE = S1;

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

uint64_t last_time = 0;
uint64_t wait_queue_time = 0;

deque<unsigned> queue;

vector<vector<uint64_t>> machine_by_cpus;

unordered_map<unsigned, unsigned> task_to_vm;

//machine -> last active time
unordered_map<unsigned, uint64_t> last_active;

// Find suitable VM (or create one) on Machine for Task
void FindVMAddTask(MachineId_t id, TaskId_t task_id) {
    MachineInfo_t machine = Machine_GetInfo(id);
    TaskInfo_t task = GetTaskInfo(task_id);
    
    for (VMId_t vm : machine_matrix[id]) {
        if (task.required_vm == VM_GetInfo(vm).vm_type) {
            VM_AddTask(vm, task_id, sla_to_prio(task.required_sla));
            task_to_vm[task_id] = vm;
            return;
        }
    }

    VMId_t new_vm = VM_Create(task.required_vm, machine.cpu);
    VM_Attach(new_vm, id);
    machine_matrix[id].push_back(new_vm);

    VM_AddTask(new_vm, task_id, sla_to_prio(task.required_sla));
    task_to_vm[task_id] = new_vm;
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

    // we have four cpus
    for(int i = 0 ; i < 4; i++) {
        vector<uint64_t> temp = {};
        machine_by_cpus.push_back(temp);
        vector<uint64_t> temp2 = {};
        running.push_back(temp2);
        vector<uint64_t> temp3 = {};
        idle.push_back(temp3);        
        vector<uint64_t> temp4 = {};
        off.push_back(temp4);
    }
    
    for(int i = 0; i < total_machines; i++) {
        machine_by_cpus[Machine_GetInfo((MachineId_t) i).cpu].push_back(i);
        vector<MachineId_t> temp = {};
        machine_matrix.push_back(temp);
    }

    for(int i = 0; i < machine_by_cpus.size(); i++) {
        int first_set = ceil((double) machine_by_cpus[i].size() * 0.5);
        int second_set = ceil((double) machine_by_cpus[i].size() * 1.0);
        for(int j = 0; j < first_set; j++) {
            running[i].push_back(machine_by_cpus[i][j]);
        } 
        for(int j = first_set; j < second_set; j++) {
            // Machine_SetState(machine_by_cpus[i][j], IDLE_S_STATE);
            Machine_SetCorePerformance(machine_by_cpus[i][j], 0, IDLE_P_STATE);
            last_active[machine_by_cpus[i][j]] = 0;
            // changing_state[machine_by_cpus[i][j]] = {RUNNING_S_STATE, IDLE_S_STATE, false, 0};
        }
        for(int j = second_set; j < machine_by_cpus[i].size(); j++) {
            Machine_SetState(machine_by_cpus[i][j], OFF_S_STATE);
            changing_state[machine_by_cpus[i][j]] = {RUNNING_S_STATE, OFF_S_STATE, false, 0};
        }
    }
}

//useless because we don't migrate
void Scheduler::MigrationComplete(Time_t time, VMId_t vm_id) {
    // Update your data structure. The VM now can receive new tasks
}

void Scheduler::NewTask(Time_t now, TaskId_t task_id) {
    TaskInfo_t task = GetTaskInfo(task_id);
    
    // dev notes: update "running" and "idle" to account for machines
    //            of diff types and gpu capability
    CPUType_t task_cpu = task.required_cpu;
    if(task.gpu_capable) {
        for (MachineId_t id : running[task_cpu]) {
            MachineInfo_t machine = Machine_GetInfo(id);
            if(machine.gpus) {
                double machine_util = machine_eff_mips(id, now);
                double task_util = task_eff_mips(task_id, id, now);
                double machine_max_util = machine.performance[P0] * machine.num_cpus;

                bool correct_cpu = task.required_cpu == machine.cpu;
                bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;
                bool enough_util = machine_util + task_util <= machine_max_util;

                // dev notes: data struct to keep track of vm by vm_type?
                // dev notes: is the change_state.count required?
                if (!changing_state.count(id) && correct_cpu && enough_mem && enough_util) {
                    FindVMAddTask(id, task_id);
                    return;
                }
            }
        }

        for (MachineId_t id : idle[task_cpu]) {
            MachineInfo_t machine = Machine_GetInfo(id);
            if(machine.gpus) {
                bool correct_cpu = task.required_cpu == machine.cpu;
                bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;

                // dev notes: state change issues? (if idle is moving around)
                // dev notes: is the change_state.count required?
                if (!changing_state.count(id) && correct_cpu && enough_mem) {
                    // dev notes: state change issues?
                    Machine_SetCorePerformance(id, 0, RUNNING_P_STATE);
                    running[task_cpu].push_back(id);
                    idle[task_cpu].erase(remove(idle[task_cpu].begin(), 
                            idle[task_cpu].end(), id), idle[task_cpu].end());
                    FindVMAddTask(id, task_id);
                    return;
                }
            }
        }
    }

    for (MachineId_t id : running[task_cpu]) {
        MachineInfo_t machine = Machine_GetInfo(id);

        double machine_util = machine_eff_mips(id, now);
        double task_util = task_eff_mips(task_id, id, now);
        double machine_max_util = machine.performance[P0] * machine.num_cpus;

        bool correct_cpu = task.required_cpu == machine.cpu;
        bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;
        bool enough_util = machine_util + task_util <= machine_max_util;

        // dev notes: data struct to keep track of vm by vm_type?
        // dev notes: is the change_state.count required?
        if (!changing_state.count(id) && correct_cpu && enough_mem && enough_util) {
            FindVMAddTask(id, task_id);
            return;
        }
    }

    for (MachineId_t id : idle[task_cpu]) {
        MachineInfo_t machine = Machine_GetInfo(id);

        bool correct_cpu = task.required_cpu == machine.cpu;
        bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;

        // dev notes: state change issues? (if idle is moving around)
        // dev notes: is the change_state.count required?
        if (!changing_state.count(id) && correct_cpu && enough_mem) {
            // dev notes: state change issues?
            Machine_SetCorePerformance(id, 0, RUNNING_P_STATE);
            FindVMAddTask(id, task_id);
            idle[task_cpu].erase(remove(idle[task_cpu].begin(), idle[task_cpu].end(), id), idle[task_cpu].end());
            running[task_cpu].push_back(id);
            return;
        }
    }

    queue.push_back(task_id);
}

void updateWaitingQueue(Time_t now) {
    //pop off tasks that are waiting
    while(queue.size() > 0) {
        TaskId_t task_id = queue[0];
        TaskInfo_t task = GetTaskInfo(task_id);
        CPUType_t task_cpu = task.required_cpu;
        bool done = false;
        if(task.gpu_capable) {
            for (MachineId_t id : running[task_cpu]) {
                MachineInfo_t machine = Machine_GetInfo(id);
                if(machine.gpus) {
                    double machine_util = machine_eff_mips(id, now);
                    double task_util = task_eff_mips(task_id, id, now);
                    double machine_max_util = machine.performance[P0] * machine.num_cpus;

                    bool correct_cpu = task.required_cpu == machine.cpu;
                    bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;
                    bool enough_util = machine_util + task_util <= machine_max_util;

                    // dev notes: data struct to keep track of vm by vm_type?
                    // dev notes: is the change_state.count required?
                    if (!changing_state.count(id) && correct_cpu && enough_mem && enough_util) {
                        FindVMAddTask(id, task_id);
                        queue.pop_front();
                        done = true;
                        break;
                    }
                }
            }

            for (MachineId_t id : idle[task_cpu]) {
                MachineInfo_t machine = Machine_GetInfo(id);
                if(machine.gpus) {
                    bool correct_cpu = task.required_cpu == machine.cpu;
                    bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;

                    // dev notes: state change issues? (if idle is moving around)
                    // dev notes: is the change_state.count required?
                    if (!changing_state.count(id) && correct_cpu && enough_mem) {
                        // dev notes: state change issues?
                        Machine_SetCorePerformance(id, 0, RUNNING_P_STATE);
                        idle[task_cpu].erase(remove(idle[task_cpu].begin(), idle[task_cpu].end(), id), idle[task_cpu].end());
                        running[task_cpu].push_back(id);
                        last_active[id] = 0;
                        FindVMAddTask(id, task_id);
                        queue.pop_front();
                        done = true;
                        break;
                    }
                }
            }
            if(done) {
                continue;
            }
        }

        for (MachineId_t id : running[task_cpu]) {
            MachineInfo_t machine = Machine_GetInfo(id);

            double machine_util = machine_eff_mips(id, now);
            double task_util = task_eff_mips(task_id, id, now);
            double machine_max_util = machine.performance[P0] * machine.num_cpus;

            bool correct_cpu = task.required_cpu == machine.cpu;
            bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;
            bool enough_util = machine_util + task_util <= machine_max_util;

            // dev notes: data struct to keep track of vm by vm_type?
            // dev notes: is the change_state.count required?
            if (!changing_state.count(id) && correct_cpu && enough_mem && enough_util) {
                FindVMAddTask(id, task_id);
                queue.pop_front();
                done = true;
                break;
            }
        }
        if(done) {
            continue;
        }

        for (MachineId_t id : idle[task_cpu]) {
            MachineInfo_t machine = Machine_GetInfo(id);

            bool correct_cpu = task.required_cpu == machine.cpu;
            bool enough_mem = machine.memory_used + task.required_memory + 8 < machine.memory_size;

            // dev notes: state change issues? (if idle is moving around)
            // dev notes: is the change_state.count required?
            if (!changing_state.count(id) && correct_cpu && enough_mem) {
                // dev notes: state change issues?
                idle[task_cpu].erase(remove(idle[task_cpu].begin(), idle[task_cpu].end(), id), idle[task_cpu].end());
                running[task_cpu].push_back(id);
                Machine_SetCorePerformance(id, 0, IDLE_P_STATE);
                FindVMAddTask(id, task_id);
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
}

void Scheduler::PeriodicCheck(Time_t now) {
    // This method should be called from SchedulerCheck()
    // SchedulerCheck is called periodically by the simulator to allow you to monitor, make decisions, adjustments, etc.
    // Unlike the other invocations of the scheduler, this one doesn't report any specific event
    // Recommendation: Take advantage of this function to do some monitoring and adjustments as necessary
    // about 1 sec between checks
    
    if(now - wait_queue_time >= SECOND / 10) {
        wait_queue_time = now;
        updateWaitingQueue(now);
    }

    if(now - last_time >= SECOND) {
        last_time = now;
        for(int i = 0; i < 4; i++) {
            if(machine_by_cpus[i].size() != 0) {
                unsigned cpu_total_mips = 0;
                unsigned cpu_max_mips = 0;
                
                for (MachineId_t id : running[i]) {
                    MachineInfo_t info = Machine_GetInfo(id);
                    cpu_total_mips += machine_eff_mips(id, now);
                    cpu_max_mips += info.performance[P0] * info.num_cpus;
                }

                for (MachineId_t id : idle[i]) {
                    MachineInfo_t info = Machine_GetInfo(id);
                    cpu_max_mips += info.performance[P0] * info.num_cpus;
                }
                
                double cpu_util = cpu_max_mips == 0 ? 0 : (double) cpu_total_mips / (double) cpu_max_mips;

                if (cpu_util > 0.5) {
                    double theoretical_util = cpu_util;
                    
                    unsigned idle_index = 0;
                    while (idle_index < off[i].size() && theoretical_util > 0.5) {
                        MachineId_t id = off[i][idle_index];
                        if(!changing_state.count(id)) {
                            Machine_SetState(id, RUNNING_S_STATE);
                            changing_state[id] = {OFF_S_STATE, RUNNING_S_STATE, false, 0};
                            
                            MachineInfo_t info = Machine_GetInfo(id);
                            cpu_max_mips += info.performance[P0] * info.num_cpus;
                            theoretical_util = cpu_max_mips == 0 ? 0 : cpu_total_mips / cpu_max_mips;
                        }
                        idle_index++;
                    }
                } 

                // else if (cpu_u{
                //     for(int j = 0; j < idle[i].size(); j++) {
                //         if(now - last_active[idle[i][j]] >= SECOND * 10) {
                //             if(!changing_state.count(idle[i][j])) {
                //                 Machine_SetState(idle[i][j], OFF_S_STATE);
                //                 changing_state[idle[i][j]] = {RUNNING_S_STATE, OFF_S_STATE, false, 0};
                //             }
                //         }
                //     }
                // }
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
    VMId_t old_vm = task_to_vm[task_id];
    MachineId_t curr_machine = VM_GetInfo(old_vm).machine_id;
    task_to_vm.erase(task_id);
    CPUType_t machine_cpu = Machine_GetInfo(curr_machine).cpu;
    if(machine_eff_mips(curr_machine, now) == 0.0) {
        running[machine_cpu].erase(remove(running[machine_cpu].begin(), running[machine_cpu].end(), curr_machine), running[machine_cpu].end());
        idle[machine_cpu].push_back(curr_machine);
        // if(vm_eff_mips(curr_machine, old_vm, now) == 0.0) {            
        //     machine_matrix[curr_machine].erase(remove(machine_matrix[curr_machine].begin(), 
        //                         machine_matrix[curr_machine].end(), old_vm), machine_matrix[curr_machine].end());
            
        //     VM_Shutdown(old_vm);
        // }
        Machine_SetCorePerformance(curr_machine, 0, IDLE_P_STATE);
        // last_active[idle[machine_cpu][curr_machine]] = now;
    }
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
    
    // // either idle machine or off machine woken for task assignment
    // if (info.new_state == RUNNING_S_STATE && info.attach_task) {
    //     FindVMAddTask(machine_id, info.task);
    // }

    CPUType_t machine_cpu = Machine_GetInfo(machine_id).cpu;
    // dev notes: state change issues?
    // idle->off
    if (info.old_state == RUNNING_S_STATE) {
        idle[machine_cpu].erase(remove(idle[machine_cpu].begin(), idle[machine_cpu].end(), machine_id), idle[machine_cpu].end());
    } 
    //off->idle
    else if (info.old_state == OFF_S_STATE) {
        off[machine_cpu].erase(remove(off[machine_cpu].begin(), off[machine_cpu].end(), machine_id), off[machine_cpu].end());
    }

    // dev notes: state change issues?
    //off->idle
    if (info.new_state == RUNNING_S_STATE) {
       idle[machine_cpu].push_back(machine_id);
    } 
    //idle->off
    else if (info.new_state == OFF_S_STATE) {
        off[machine_cpu].push_back(machine_id);
    }

    changing_state.erase(machine_id);
}