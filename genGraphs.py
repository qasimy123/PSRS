import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import os

experiments = [
    f
    for f in os.listdir("experiments")
    if os.path.isdir(os.path.join("experiments", f))
]


def perf(experiment, files):
    speedup_files = [f for f in files if "speedup" in f]
    speedup_files.sort()
    time_files = [f for f in files if "speedup" not in f]
    time_files.sort()
    plt.figure(figsize=(10, 10))
    for i in range(len(speedup_files)):
        df = pd.read_csv(speedup_files[i])
        numElements = speedup_files[i].split("_")[-1].split(".")[0]
        plt.plot(df["numThreads"], df["speedup"], label=numElements)
    # Plot a unit line
    plt.plot([1, 16], [1, 16], label="Ideal")
    # plot grid
    plt.grid()
    plt.title(f"Speedup vs number of threads for different number of elements")
    plt.xlabel("Number of threads")
    plt.ylabel("Speedup")
    plt.legend()
    plt.savefig(f"experiments/{experiment}/speedup.png")
    plt.figure(figsize=(10, 10))
    for i in range(len(time_files)):
        df = pd.read_csv(time_files[i])
        numElements = speedup_files[i].split("_")[-1].split(".")[0]
        plt.plot(df["numThreads"], df["time"], label=numElements)
    plt.grid()
    plt.title(f"Time vs number of threads for different number of elements")
    plt.xlabel("Number of threads")
    plt.ylabel("Time")
    plt.legend()
    plt.savefig(f"experiments/{experiment}/time.png")


for experiment in experiments:
    files = [
        os.path.join("experiments", experiment, f)
        for f in os.listdir(os.path.join("experiments", experiment))
        if f.endswith(".csv")
    ]
    if experiment == "performance":
        perf(experiment, files)

    elif experiment == "phases":
        for i in range(len(files)):
            df = pd.read_csv(files[i])
            numElements = files[i].split("_")[-1].split(".")[0]
            # numThreads,phase1,phase2,phase3,phase4
            plt.figure(figsize=(10, 10))
            # using stackplot
            plt.stackplot(
                df["numThreads"],
                df["phase1"],
                df["phase2"],
                df["phase3"],
                df["phase4"],
                labels=["phase1", "phase2", "phase3", "phase4"],
            )
            plt.grid()
            plt.title(f"Time spent in each phase for {numElements} elements")
            plt.xlabel("Number of threads")
            plt.ylabel("Time")
            plt.legend()
            plt.savefig(f"experiments/{experiment}/phases_{numElements}.png")
    elif experiment == "sampling":
        random_time_files = [f for f in files if "random" in f and "speedup" not in f]
        random_time_files.sort()
        regular_time_files = [f for f in files if "regular" in f and "speedup" not in f]
        regular_time_files.sort()
        combined_time_files = list(zip(random_time_files, regular_time_files))
        for i in range(len(combined_time_files)):
            random_df = pd.read_csv(combined_time_files[i][0])
            regular_df = pd.read_csv(combined_time_files[i][1])
            numElements = combined_time_files[i][0].split("_")[-1].split(".")[0]
            plt.figure(figsize=(10, 10))
            plt.plot(random_df["numThreads"], random_df["time"], label="random")
            plt.plot(regular_df["numThreads"], regular_df["time"], label="regular")
            plt.grid()
            plt.title(f"Time vs number of threads for {numElements} elements")
            plt.xlabel("Number of threads")
            plt.ylabel("Time")
            plt.legend()
            plt.savefig(f"experiments/{experiment}/sampling_{numElements}.png")
        random_speedup_files = [f for f in files if "random" in f and "speedup" in f]
        random_speedup_files.sort()
        regular_speedup_files = [f for f in files if "regular" in f and "speedup" in f]
        regular_speedup_files.sort()
        combined_speedup_files = list(zip(random_speedup_files, regular_speedup_files))
        for i in range(len(combined_speedup_files)):
            random_df = pd.read_csv(combined_speedup_files[i][0])
            regular_df = pd.read_csv(combined_speedup_files[i][1])
            numElements = combined_speedup_files[i][0].split("_")[-2]
            plt.figure(figsize=(10, 10))
            plt.plot(random_df["numThreads"], random_df["speedup"], label="random")
            plt.plot(regular_df["numThreads"], regular_df["speedup"], label="regular")
            plt.grid()
            plt.title(f"Speedup vs number of threads for {numElements} elements")
            plt.xlabel("Number of threads")
            plt.ylabel("Speedup")
            plt.legend()
            plt.savefig(f"experiments/{experiment}/speedup_sampling_{numElements}.png")
    elif experiment == "distribution":
        normal_time_files = [f for f in files if "normal" in f and "speedup" not in f]
        normal_time_files.sort()
        uniform_time_files = [f for f in files if "uniform" in f and "speedup" not in f]
        uniform_time_files.sort()
        combined_time_files = list(zip(normal_time_files, uniform_time_files))
        for i in range(len(combined_time_files)):
            normal_df = pd.read_csv(combined_time_files[i][0])
            uniform_df = pd.read_csv(combined_time_files[i][1])
            numElements = combined_time_files[i][0].split("_")[-1].split(".")[0]
            plt.figure(figsize=(10, 10))
            plt.plot(normal_df["numThreads"], normal_df["time"], label="normal")
            plt.plot(uniform_df["numThreads"], uniform_df["time"], label="uniform")
            plt.grid()
            plt.title(f"Time vs number of threads for {numElements} elements")
            plt.xlabel("Number of threads")
            plt.ylabel("Time")
            plt.legend()
            plt.savefig(f"experiments/{experiment}/distribution_{numElements}.png")
        normal_speedup_files = [f for f in files if "normal" in f and "speedup" in f]
        normal_speedup_files.sort()
        uniform_speedup_files = [f for f in files if "uniform" in f and "speedup" in f]
        uniform_speedup_files.sort()
        combined_speedup_files = list(zip(normal_speedup_files, uniform_speedup_files))
        for i in range(len(combined_speedup_files)):
            normal_df = pd.read_csv(combined_speedup_files[i][0])
            uniform_df = pd.read_csv(combined_speedup_files[i][1])
            numElements = combined_speedup_files[i][0].split("_")[-2]
            plt.figure(figsize=(10, 10))
            plt.plot(normal_df["numThreads"], normal_df["speedup"], label="normal")
            plt.plot(uniform_df["numThreads"], uniform_df["speedup"], label="uniform")
            plt.grid()
            plt.title(f"Speedup vs number of threads for {numElements} elements")
            plt.xlabel("Number of threads")
            plt.ylabel("Speedup")
            plt.legend()
            plt.savefig(
                f"experiments/{experiment}/distribution_{numElements}_speedup.png"
            )
