import time
import os

def io_bound(passed_pid):
    file_name = f"iobound_test_{passed_pid}.txt"
    with open(file_name, "a") as f:
        for i in range(10):
            f.write(f"Writing iteration {i + 1} from process {passed_pid}\n")
            f.flush()  # ensure data written
            time.sleep(0.1)  # simulate i/o delay
            print(f"I/O-bound process {passed_pid} iteration {i + 1} completed.")
    print(f"I/O-bound process {passed_pid} completed.")


if __name__ == "__main__":
    process_id = os.getpid()
    print(f"Running I/O bound process {process_id}...")
    io_bound(process_id)
