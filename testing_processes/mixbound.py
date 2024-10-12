import time
import os

def is_prime(n):
    if n <= 1:
        return False
    if n <= 3:
        return True
    if n % 2 == 0 or n % 3 == 0:
        return False
    i = 5
    while i * i <= n:
        if n % i == 0 or n % (i + 2) == 0:
            return False
        i += 6
    return True

def mix_bound(passed_pid):
    file_name = f"mixbound_test_{passed_pid}.txt"
    count = 0
    for i in range(10):
        for j in range(2, 500000):
            if is_prime(j):
                count += 1
        with open(file_name, "a") as f:
            f.write(f"Writing iteration {i + 1} from process {passed_pid}\n")
            f.flush()  # ensure data written
            time.sleep(0.05)  # simulate i/o delay
        print(f"Mixed-bound process {passed_pid} iteration {i + 1} completed.")
    print(f"Mixed-bound process {passed_pid} found that {count} numbers were prime.")


if __name__ == "__main__":
    process_id = os.getpid()
    print(f"Running mixed-bound process {process_id}...")
    mix_bound(process_id)
