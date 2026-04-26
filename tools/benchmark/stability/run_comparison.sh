#!/bin/bash

# Usage: ./run_comparison.sh [LOAD_LEVEL]
# LOAD_LEVEL: 1=light, 2=medium, 3=heavy, 4=ultra (default: 1)

LOAD_LEVEL=${1:-1}

export PYTHONPATH=$(pwd)/bindings/python:$(pwd)/build/lib
export LOAD_LEVEL

echo "Starting Stability Benchmark Comparison (Level ${LOAD_LEVEL}, 60s each)..."
echo "----------------------------------------------------"

# Run Unilink Benchmark
python3 tools/benchmark/stability/stability_bench.py

echo ""

# Run Raw Socket Benchmark
python3 tools/benchmark/stability/stability_bench_raw.py

echo ""
echo "Benchmark Comparison Complete."
