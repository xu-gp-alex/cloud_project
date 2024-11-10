//
//  Simulator.hpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//

#ifndef Simulator_hpp
#define Simulator_hpp

#include <iostream>
#include <queue>
#include <memory>

#include "Interfaces.h"
#include "Internal_Interfaces.h"

class Event {
public:
    Event(Time_t t) : time(t)                       {}
    Time_t GetEventTime()                           { return time; }
    virtual void Execute() = 0;
protected:
    Time_t time;
};

class TaskArrivalEvent : public Event {
public:
typedef enum { NEW_TASK, TASK_COMPLETION} TaskEvent_t;
    TaskArrivalEvent(Time_t time, TaskId_t id) : Event(time), task_id(id) {}
    void Execute() override { HandleNewTask(time, task_id); }
private:
    TaskId_t task_id;
};

class TaskCompletionEvent : public Event {
public:
    TaskCompletionEvent(Time_t time, MachineId_t machine_id, u_int core_id) : Event(time), machineId(machine_id), coreId(core_id) {}
    void Execute() override { Machine_CompleteTask(machineId, coreId); }
private:
    MachineId_t machineId;
    unsigned coreId;
};

class TimerEvent: public Event {
public:
    TimerEvent(Time_t time) : Event(time)  {}
    void Execute() override { Machine_HandleTimer(time); }
};

class MigrationEvent: public Event {
public:
    MigrationEvent(Time_t time, VMId_t vm_id) : Event(time), vmId(vm_id) {}
    void Execute() override { VM_MigrationCompleted(vmId); }
private:
    VMId_t vmId;
};

class Simulator {
public:
    Simulator()                                 { now = Time_t(0); }
    void AddEvent(shared_ptr<Event> event)      { eventQueue.push(event); }
    Time_t Now()                                { return now; }
    void Simulate();
private:
    struct EventComparator {
        bool operator()(const std::shared_ptr<Event>& e1, const std::shared_ptr<Event>& e2) const {
            return e1->GetEventTime() > e2->GetEventTime();
        }
    };
    priority_queue<shared_ptr<Event>, vector<shared_ptr<Event>>, EventComparator> eventQueue;
    Time_t now;
};
#endif /* Simulator_hpp */
