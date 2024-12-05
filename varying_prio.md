# Designed to evaluate how the scheduler handles scheduling tasks with differing SLA
# requirements when the workload is more than what the machines can concurrently
# handle and some tasks will need to wait longer for completion. Overlaps different
# SLA requirement tasks within different time intervals to see how the scheduler 
# reacts to and considers task progress as a factor rather than just SLA type.

machine class:
{
        Number of machines: 64
        CPU type: X86
        Number of cores: 1
        Memory: 1024
        S-States: [120, 100, 80, 40, 20, 10, 0]
        P-States: [10, 8, 6, 4]
        C-States: [10, 3, 1, 0]
        MIPS: [800, 600, 400, 200]
        GPUs: no
}

task class:
{
        Start time: 0
        End time : 2000
        Inter arrival: 10
        Expected runtime: 750
        Memory: 1024
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA3
        CPU type: X86
        Task type: CRYPTO
        Seed: 520230
}

task class:
{
        Start time: 500
        End time : 2000
        Inter arrival: 10
        Expected runtime: 500
        Memory: 1024
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA2
        CPU type: X86
        Task type: CRYPTO
        Seed: 520230
}

task class:
{
        Start time: 1000
        End time : 2000
        Inter arrival: 10
        Expected runtime: 250
        Memory: 1024
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: CRYPTO
        Seed: 520230
}