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

def cpu_bound(passed_pid):
    count = 0
    for i in range(2, 1000000):  # check how many prime numbers exist between 2 and 100,000
        if is_prime(i):
            count += 1
    print(f"CPU-bound process {passed_pid} found that {count} numbers were prime.")


if __name__ == "__main__":
    process_id = os.getpid()
    print(f"Running CPU-bound process {process_id}...")
    cpu_bound(process_id)