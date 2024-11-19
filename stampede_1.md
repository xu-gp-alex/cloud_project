# Approximation of STAMPEDE3 with realistic test cases, no GPU
# (each Node is dual processor, so we double the TDP)

# Sapphire Rapids Compute Nodes
# CPU: Intel Xeon CPU MAX 9480 ("Sapphire Rapids HBM")
# TDP: 350 W
machine class:
{
        Number of machines: 560
        CPU type: X86
        Number of cores: 112
        Memory: 131072
        S-States: [700, 590, 590, 470, 240, 60, 0]
        P-States: [12, 10, 6, 2]
        C-States: [12, 10, 6, 0]
        MIPS: [1900, 1500, 1300, 1000]
        GPUs: no
}

# Ponte Vecchio Compute Nodes
# CPU: Intel Xeon Platinum 8480 ("Sapphire Rapids")
# TDP: 350 W
# GPU: Intel Data Center GPU Max 1550s ("Ponte Vecchio")
machine class:
{
        Number of machines: 20
        Number of cores: 96
        CPU type: X86
        Memory: 519168
        S-States: [700, 590, 590, 470, 240, 60, 0]
        P-States: [12, 10, 6, 2]
        C-States: [12, 10, 6, 0]
        MIPS: [2000, 1600, 1200, 800]
        GPUs: yes
}

# Skylake Compute Nodes
# CPU: Intel Xeon Platinum 8160 ("Skylake")
# TDP: 150 W
machine class:
{
        Number of machines: 1060
        Number of cores: 48
        CPU type: X86
        Memory: 196608
        S-States: [300, 250, 250, 200, 100, 50, 0]
        P-States: [12, 8, 4, 2]
        C-States: [12, 8, 4, 0]
        MIPS: [2100, 1800, 1600, 1400]
        GPUs: no
}

# ICX Compute Nodes
# CPU: Intel Xeon Platinum 8380 ("Ice Lake")
# TDP: 270 W
machine class:
{
        Number of machines: 224
        Number of cores: 80
        CPU type: X86
        Memory: 262144
        S-States: [540, 360, 360, 288, 144, 36, 0]
        P-States: [12, 8, 4, 2]
        C-States: [12, 8, 4, 0]
        MIPS: [2300, 2100, 1900, 1500]
        GPUs: no
}

# GoCJ Dataset

# Our task classes are based on the Google Cloud Jobs (GoCJ) Dataset 
# (https://www.mdpi.com/2306-5729/3/4/38). Based off real world Google cluster 
# traces, they found that 20% of tasks will be small, 40% medium, 30% large, 
# 4% extra-large, and 6% huge. For each task class, we set the run time to the average
# of the bounds for each class of tasks in the Google cluster data. We split the tasks 
# into various SLAs to simulate different tasks having different requirements
# -generally larger tasks have looser SLAs, and vice versa. For this input, we start
# with 10,000 tasks assigned 0 and 100,000 microseconds.

# MI = Millions of Instructions
# "small" tasks (15,000 - 55,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 100
        Expected runtime: 35000000
        Memory: 2048
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 100
        Expected runtime: 35000000
        Memory: 4096
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA1
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

# "medium" tasks (59,000 - 99,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 50
        Expected runtime: 79000000
        Memory: 8192
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 50
        Expected runtime: 79000000
        Memory: 16384
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA1
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

# "large" tasks (101,000 - 135,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 33.3
        Expected runtime: 118000000
        Memory: 32768
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA2
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

# "extra-large" tasks (150,000 - 375,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 250
        Expected runtime: 262500000
        Memory: 65536
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA2
        CPU type: X86
        Task type: WEB
        Seed: 318282
}

# "huge" tasks (525,000 - 900,000 MI)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 166.7
        Expected runtime: 712500000
        Memory: 131072
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA3
        CPU type: X86
        Task type: WEB
        Seed: 318282
}