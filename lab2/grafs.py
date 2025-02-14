import subprocess
import time
import matplotlib.pyplot as plt

def run_program(num_threads, K):

    input_file = "numbers.txt"  
    max_memory = 1024  

    start_time = time.time()
    
    result = subprocess.run(['./lab2', input_file, str(num_threads), str(max_memory)], 
                            capture_output=True, text=True)

    end_time = time.time()
    execution_time = end_time - start_time
    
    
    if result.returncode == 0:
        print(f"Number of threads: {num_threads}, K: {K}, Execution time: {execution_time:.2f} seconds")
    else:
        print(f"Error: {result.stderr}")
        
    return execution_time

def main():
    num_threads_list = list(range(1, 9))  
    K_values = [100, 5000, 700000, 20000000]  

    execution_times = {K: [] for K in K_values}
    speedup = {K: [] for K in K_values}
    efficiency = {K: [] for K in K_values}

    for K in K_values:
        for num_threads in num_threads_list:
            execution_time = run_program(num_threads, K)
            execution_times[K].append(execution_time)


    for K in K_values:
        single_thread_time = execution_times[K][0]
        speedup[K] = [single_thread_time / time for time in execution_times[K]]
        efficiency[K] = [speedup[K][i] / num_threads_list[i] for i in range(len(num_threads_list))]

    
    fig, axs = plt.subplots(3, 1, figsize=(12, 18))

    
    for K in K_values:
        axs[0].plot(num_threads_list, execution_times[K], marker='o', label=f'K={K}')
    axs[0].set_title('Execution Time vs Number of Threads')
    axs[0].set_xlabel('Number of Threads')
    axs[0].set_ylabel('Execution Time (seconds)')
    axs[0].grid(True)
    axs[0].legend()

    
    for K in K_values:
        axs[1].plot(num_threads_list, speedup[K], marker='o', label=f'K={K}')
    axs[1].set_title('Speedup vs Number of Threads')
    axs[1].set_xlabel('Number of Threads')
    axs[1].set_ylabel('Speedup')
    axs[1].grid(True)
    axs[1].legend()

    
    for K in K_values:
        axs[2].plot(num_threads_list, efficiency[K], marker='o', label=f'K={K}')
    axs[2].set_title('Efficiency vs Number of Threads')
    axs[2].set_xlabel('Number of Threads')
    axs[2].set_ylabel('Efficiency')
    axs[2].grid(True)
    axs[2].legend()

    plt.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
