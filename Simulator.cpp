//
//  Simulator.cpp
//  CloudSim
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 10/19/24.
//

#include "Simulator.hpp"

void Simulator::Simulate() {
    SimOutput("Simulate(): There are " + to_string(eventQueue.size()) + " events in the simulator", 1);
    while(!eventQueue.empty()) {
        shared_ptr<Event> e = eventQueue.top();
        now = e->GetEventTime();
        eventQueue.pop();
        e->Execute();
    }
    SimulationComplete(now);
}

static Simulator Simulator;

void StartSimulation() {
    Simulator.Simulate();
}

void ScheduleMigrationCompletion(Time_t time, VMId_t vm_id) {
    auto me = make_shared<MigrationEvent>(time, vm_id);
    Simulator.AddEvent(shared_ptr<Event> (me));
}

void ScheduleNewTask(Time_t time, TaskId_t task_id) {
    auto te = make_shared<TaskArrivalEvent>(time, task_id);
    Simulator.AddEvent(shared_ptr<Event> (te));
}

void ScheduleTaskCompletion(Time_t time, MachineId_t machine_id, unsigned core_id) {
    SimOutput("ScheduleTaskCompletion(): Scheduling task completion for core " + to_string(core_id) + " machine " + to_string(machine_id) + " at time " + to_string(time), 4);
    auto tce = make_shared<TaskCompletionEvent>(time, machine_id, core_id);
    Simulator.AddEvent(shared_ptr<Event> (tce));
}

void ScheduleTimer(Time_t time) {
    auto te = make_shared<TimerEvent>(time);
    Simulator.AddEvent(shared_ptr<Event> (te));
}

Time_t Now() {
    return Simulator.Now();
}
