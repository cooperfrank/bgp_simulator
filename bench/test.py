#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path
import tempfile
import shutil
import time

base_dir = Path(__file__).parent.absolute()
executable = base_dir / "bgp_simulator"

for test_name in ["prefix", "subprefix", "many"]:
    test_dir = base_dir / test_name
    print(f"\nTesting {test_name}...")

    with tempfile.TemporaryDirectory() as tmp:
        # Run simulator
        cmd = [
            str(executable),
            "--relationships", str(test_dir / "CAIDAASGraphCollector_2025.10.16.txt"),
            "--announcements", str(test_dir / "anns.csv"),
            "--rov-asns", str(test_dir / "rov_asns.csv")
        ]
        print("About to run command")
        start = time.perf_counter()
        result = subprocess.run(cmd, cwd=tmp, capture_output=True, text=True)
        print(f"Ran command in {time.perf_counter() - start}s")
        if test_name == "cycle" and result.returncode == 0:
            print("FAIL FOR CYCLE")
            continue
        elif test_name == "cycle":
            print("cycle passes")
            continue
        elif test_name != "cycle" and result.returncode != 0:
            print(f"  FAIL: {result.stderr}")
            raise Exception("Failure")

        # Compare outputs using diff with sort
        output_file = Path(tmp) / "ribs.csv"
        expected_file = test_dir / "ribs.csv"

        # Check line counts first
        expected_lines = len(expected_file.read_text().splitlines())
        output_lines = len(output_file.read_text().splitlines())
        if expected_lines != output_lines:
            print(f"  FAIL: Line count mismatch (expected {expected_lines}, got {output_lines})")
            input("waiting for input")

        diff_cmd = f"diff -b <(sort '{expected_file}') <(sort '{output_file}')"
        diff_result = subprocess.run(diff_cmd, shell=True, executable="/bin/bash", capture_output=True)

        if diff_result.returncode == 0:
            print(f"  PASS")
        else:
            print(f"  FAIL: Output differs")
            diff_output = diff_result.stdout.decode() if isinstance(diff_result.stdout, bytes) else diff_result.stdout
            for line in diff_output.splitlines():
                print(f"    {line}")