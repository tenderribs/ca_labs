#!/bin/bash

# Ensure the output directory exists
mkdir -p report/result_logs

# Use xargs to run up to 10 processes in parallel
# -0 handles filenames with spaces safely
# -P 10 sets the number of parallel workers
printf "%s\0" report/yml_configs/all/*.yaml | xargs -0 -n 1 -P 8 -I {} bash -c '
    yaml_file="{}"
    test_name=$(basename "$yaml_file" .yaml)
    echo "Running: $test_name"
    ./build/ramulator2 -f "$yaml_file" > "report/result_logs/${test_name}.log"
'

echo "All tests completed."