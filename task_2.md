machine class:
{
# comment
        Number of machines: 16
        CPU type: X86
        Number of cores: 8
        Memory: 16384
        S-States: [120, 100, 100, 80, 40, 10, 0]
        P-States: [12, 8, 6, 4]
        C-States: [12, 3, 1, 0]
        MIPS: [1000, 800, 600, 400]
        GPUs: yes
}
machine class:
{
        Number of machines: 24
        Number of cores: 16
        CPU type: ARM
        Memory: 16384
        S-States: [120, 100, 100, 80, 40, 10, 0]
        P-States: [12, 8, 6, 4]
        C-States: [12, 3, 1, 0]
        MIPS: [1000, 800, 600, 400]
        GPUs: yes
}

# GoCJ Dataset
# question: how frequently do tasks arrive in real world?
# arbitrarily, we start with 10,000 tasks
# arbitrarily, we have t_0 = 0, t_1 = 100,000
# arbitrarily, set expected runtime to avg of extremes

# "small" tasks (15,000 - 55,000 MI) (20%)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 500
        Expected runtime: 35000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

# "medium" tasks (59,000 - 99,000 MI) (40%)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 250
        Expected runtime: 79000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

# "large" tasks (101,000 - 135,000 MI) (30%)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 333
        Expected runtime: 118000
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

# "extra-large" tasks (150,000 - 375,000 MI) (4%)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 2500
        Expected runtime: 262500
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}

# "huge" tasks (525,000 - 900,000 MI) (6%)
task class:
{
        Start time: 0
        End time : 100000
        Inter arrival: 1667
        Expected runtime: 712500
        Memory: 8
        VM type: LINUX
        GPU enabled: no
        SLA type: SLA0
        CPU type: X86
        Task type: WEB
        Seed: 520230
}