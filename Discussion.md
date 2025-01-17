

1. Solutions Considered

 A) Naive Line-by-Line Search
Approach
- For each query, read the entire file (1 TB) line by line.
- Match lines that start with the requested date (`YYYY-MM-DD`).
- Write matching lines to output.

Pros
- Very simple to implement.

Cons
- Highly inefficient for large files (1 TB).
- Repeatedly scanning the file is time-consuming.




 B) Index-Based (Preprocessing + Random Access)
Approach 
- Make a single pass over the log file to build an index of `date -> byte offset in file`.
- Store these offsets in a small file, e.g., `log_index.txt`.
- For a given date query, look up the date in the index to find the corresponding offset in the log file.
- Seek directly to that offset and read until the date changes or until EOF.

Pros
- Each query after index-building is extremely fast (only read a day’s worth of logs).
- One-time cost of building the index is acceptable for a 1 TB file.

Cons
- Requires preprocessing time and additional storage for the index file (though small relative to 1 TB).
- More complex than a naive line-by-line search.

 C) Split Logs by Date in Real-Time
Approach
- If logs were stored in separate daily files from the start (e.g., `YYYY-MM-DD.log`), no search needed.

Pros 
- Trivial to retrieve logs for a date—just open the file for that day.

Cons 
- Doesn’t help if the logs are already in one monolithic file.
- Requires changing the logging process from the beginning.

2. Final Solution Summary

We chose **Approach B (Index-Based)** for this project because:
1. A one-time index building pass is a reasonable cost for a 1 TB log file.
2. Subsequent queries for any date are extremely fast (minimal I/O).

Overview:
1. Build the Index:  
   - Read `test_logs.log` line by line and record each new date’s byte offset.
   - Write these (date, offset) pairs to `log_index.txt`.

2. Extract Logs:
   - Look up the given date in the index file to find the start offset.
   - Seek directly to that position in `test_logs.log`.
   - Read lines until we reach the next date’s offset or EOF.
   - Write all matching lines to `output/output_YYYY-MM-DD.txt`.

3. Steps to Run

1. Compile the C++ code (example for Linux/Mac):
   ```bash
   cd src
   g++ -std=c++17 extract_logs.cpp -o extract_logs
