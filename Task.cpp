//
//  Task.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//

#include "CPU.hpp"
#include "Task.hpp"

static vector<Task> Tasks;
static unsigned TaskId_gen = 0;
static unsigned Active_tasks = 0;

static struct {
    unsigned completed_tasks;
    unsigned violations;
} sla_violations[NUM_SLAS];

static void ValidateTaskId(TaskId_t task_id, string err_msg) {
    if(task_id >= Tasks.size()) {
        ThrowException(err_msg);
    }
}

// External Interface

extern VMType_t RequiredVMType(TaskId_t task_id);
extern void SetTaskPriority(TaskId_t task_id, Priority_t priority);

unsigned GetNumTasks() {
    return unsigned(Tasks.size());
}

TaskInfo_t GetTaskInfo(TaskId_t task_id) {
    ValidateTaskId(task_id, "GetTaskInfo(): Invalid task id" + to_string(task_id));
    return Tasks[task_id].GetInfo();

}

unsigned GetTaskMemory(TaskId_t task_id) {
    ValidateTaskId(task_id, "GetTaskMemory(): Invalid task id " + to_string(task_id));
    return Tasks[task_id].GetMemory();
}

unsigned GetTaskPriority(TaskId_t task_id) {
    ValidateTaskId(task_id, "GetTaskPriority(): Invalid task id "+ to_string(task_id));
    return Tasks[task_id].GetPriority();
}

double GetSLAReport(SLAType_t sla) {
    return 100.0 * (sla_violations[sla].completed_tasks == 0 ? 0.0 : double(sla_violations[sla].violations)/double(sla_violations[sla].completed_tasks));
}

bool IsSLAViolation(TaskId_t task_id) {
    ValidateTaskId(task_id, "IsSLAViolation(): Invalid task id " + to_string(task_id));
    return Tasks[task_id].IsSLAViolated();
}

bool IsTaskCompleted(TaskId_t task_id) {
    ValidateTaskId(task_id, "IsTaskCompleted(): Invalid task id " + to_string(task_id));
    return Tasks[task_id].IsCompleted();
}

bool IsTaskGPUCapable(TaskId_t task_id) {
    ValidateTaskId(task_id, "IsTaskGPU Capable(): Invalid task id " + to_string(task_id));
    return Tasks[task_id].IsGPUCapable();
}

CPUType_t RequiredCPUType(TaskId_t task_id) {
    ValidateTaskId(task_id, "RequiredCPUType(): Invalid task id " + to_string(task_id));
    return Tasks[task_id].GetCPUType();
}

SLAType_t RequiredSLA(TaskId_t task_id) {
    ValidateTaskId(task_id, "RequiredSLA(): Invalid task id " + to_string(task_id));
    return Tasks[task_id].GetSLAType();
}

VMType_t RequiredVMType(TaskId_t task_id) {
    ValidateTaskId(task_id, "RequiredVMType(): Invalid task id " + to_string(task_id));
    return Tasks[task_id].GetVMType();
}

void SetTaskPriority(TaskId_t task_id, Priority_t priority) {
    ValidateTaskId(task_id, "SetTaskPriority(): Invalid task id " + to_string(task_id));
    Tasks[task_id].SetPriority(priority);
}

// Internal interface

TaskId_t AddTask(uint64_t inst, Time_t arr, Time_t trgt, VMType_t vm, SLAType_t sla, CPUType_t cpu, bool gpu, unsigned mem, TaskClass_t task_class) {
    TaskId_t task_id = TaskId_gen++;
    Task task(inst, arr, trgt, vm, sla, cpu, gpu, mem, task_class, task_id);
    Tasks.push_back(task);
    ScheduleNewTask(arr, task_id);

    Active_tasks++;
    return task_id;
}

void CompleteTask(TaskId_t task_id) {
    ValidateTaskId(task_id, "CompleteTask(): Invalid task id " + to_string(task_id));
    Tasks[task_id].SetCompleted();
    sla_violations[Tasks[task_id].GetSLAType()].completed_tasks++;
    if(Tasks[task_id].IsSLAViolated()) {
        sla_violations[Tasks[task_id].GetSLAType()].violations++;
        SLAWarning(Now(), task_id);          // Called to alert the schedule of an SLA violation
    }
    Active_tasks--;
    Tasks[task_id].CompletionReport();
}

unsigned GetActiveTasks() {
    return Active_tasks;
}

uint64_t GetRemainingInstructions(TaskId_t task_id) {
    ValidateTaskId(task_id, "GetRemainingInstructions(): Invalid task id " + to_string(task_id));
    return Tasks[task_id].GetRemainingInstructions();
}

void SetRemainingInstructions(TaskId_t task_id, uint64_t instructions) {
    ValidateTaskId(task_id, "SetRmeainingInstructions(): Invalid task id " + to_string(task_id));
    Tasks[task_id].SetRemainingInstructions(instructions);
}

// Object implementation

Task::Task(uint64_t inst, Time_t arr, Time_t trgt, VMType_t vm, SLAType_t sla, CPUType_t cpu, bool gpu, unsigned mem, TaskClass_t task_class, TaskId_t id) {

    arrival      = arr;
    target       = trgt;
    
    vmType       = vm;
    slaType      = sla;
    cpuType      = cpu;
    gpuCapable  = gpu;
    memory       = mem;
    taskId      = id;
    
    instructions = inst;
    remaining    = instructions;
    completed = false;
    priority  = MID_PRIORITY;
}

TaskInfo_t Task::GetInfo() {
    TaskInfo_t task_info;
    task_info.completed              = completed;
    task_info.gpu_capable            = gpuCapable;
    
    task_info.arrival                = arrival;
    task_info.completion             = completion;
    task_info.target_completion      = target;
    task_info.task_id                = taskId;
    
    task_info.total_instructions     = instructions;
    task_info.remaining_instructions = remaining;
    task_info.priority               = priority;
    
    task_info.required_cpu           = cpuType;
    task_info.required_memory        = memory;
    task_info.required_sla           = slaType;
    task_info.required_vm            = vmType;
    
    return task_info;
}

void Task::CompletionReport() {
    SimOutput("Task::CompletionReport(): " + to_string(taskId) + " arrived at " + to_string(arrival) + " with a runtime of " + to_string(instructions / 1000) + " and target of " + to_string(target) + " and Completed at " + to_string(completion), 4);
}
