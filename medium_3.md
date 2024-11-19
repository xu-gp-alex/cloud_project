# MEDIUM + OLD Mainframe, stress test

# x86-based
machine class:
{
        Number of machines: 225
        CPU type: X86
        Number of cores: 48
        Memory: 65536
        S-States: [350, 295, 295, 235, 120, 30, 0]
        P-States: [6, 5, 3, 1]
        C-States: [6, 5, 3, 0]
        MIPS: [1900, 1500, 1300, 1000]
        GPUs: yes
}

# arm-based
machine class:
{
        Number of machines: 225
        CPU type: ARM
        Number of cores: 48
        Memory: 65536
        S-States: [280, 235, 235, 185, 95, 24, 0]
        P-States: [5, 4, 2, 1]
        C-States: [5, 4, 2, 0]
        MIPS: [1900, 1500, 1300, 1000]
        GPUs: yes
}

# power-based
machine class:
{
        Number of machines: 50
        CPU type: POWER
        Number of cores: 16
        Memory: 65536
        S-States: [380, 325, 325, 265, 150, 60, 0]
        P-States: [7, 6, 4, 1]
        C-States: [7, 6, 4, 0]
        MIPS: [1200, 1000, 800, 600]
        GPUs: yes
}

# GoCJ Dataset

# Our task classes are based on the Google Cloud Jobs (GoCJ) Dataset 
# (https://www.mdpi.com/2306-5729/3/4/38). Based off real world Google cluster 
# traces, they found that 20% of tasks will be small, 40% medium, 30% large, 
# 4% extra-large, and 6% huge. For each task class, we set the run time to the average
# of the bounds for each class of tasks in the Google cluster data. SLAs are very strict, with only SLA1 and SLA0
# with 100,000 tasks assigned 0 and 100,000 microseconds.

# MI = Millions of Instructions
# "small" tasks (15,000 - 55,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 10
        Expected runtime: 35000000
        Memory: 2048
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 10
        Expected runtime: 35000000
        Memory: 2048
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA0
        CPU type: ARM
        Task type: WEB
        Seed: 318282
}

# "medium" tasks (59,000 - 99,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 5
        Expected runtime: 79000000
        Memory: 4096
        VM type: WIN
        GPU enabled: yes
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 5
        Expected runtime: 79000000
        Memory: 4096
        VM type: WIN
        GPU enabled: yes
        SLA type: SLA0
        CPU type: ARM
        Task type: WEB
        Seed: 318282
}

# "large" tasks (101,000 - 135,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 3.33
        Expected runtime: 118000000
        Memory: 8192
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

# "extra-large" tasks (150,000 - 375,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 25
        Expected runtime: 262500000
        Memory: 8192
        VM type: AIX
        GPU enabled: yes
        SLA type: SLA0
        CPU type: POWER
        Task type: WEB
        Seed: 318282
}

# "huge" tasks (525,000 - 900,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 16.67
        Expected runtime: 712500000
        Memory: 16384
        VM type: LINUX
        GPU enabled: yes
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}