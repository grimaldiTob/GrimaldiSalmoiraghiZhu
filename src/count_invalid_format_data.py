import json
import argparse
import sys
import os

# Script used to detect defected measurements for DataIngestor testing
#
# Usage:
#   Single file:      python3 count_invalid_format_data.py path/to/file.txt
#   All files in dir: python3 count_invalid_format_data.py path/to/dir/
#
# Example output (directory mode):
#
# ══════════════════════════════════════════
#  Processing: raw_data_1777721417_0000.txt
# ══════════════════════════════════════════
# ❌ Line 4 INVALID: Missing 'timestamp'
#    Raw data: {"sensor_id": "ACCEL-004", "value": 321.924, "priority": "HIGH"}
# ...
# ✅ Valid measurements:   47
# ❌ Invalid measurements: 3
#
# ══════════════════════════════════════════
#  Processing: raw_data_1777721417_0001.txt
# ══════════════════════════════════════════
# ...
#
# ══════════════════════════════════════════
#              GLOBAL SUMMARY
# ══════════════════════════════════════════
#  Files processed:        3
#  Total ✅ valid:         141
#  Total ❌ invalid:       7


REQUIRED_FIELDS = ['sensor_id', 'timestamp', 'value']
VALID_PRIORITIES = {"HIGH", "MEDIUM", "LOW"}


def validate_line(data: dict) -> list[str]:
    """Return a list of validation error strings for a parsed JSON object."""
    errors = []

    # 1. Check Sensor ID
    if 'sensor_id' not in data:
        errors.append("Missing 'sensor_id'")
    elif not isinstance(data['sensor_id'], str):
        errors.append("'sensor_id' is not a string")

    # 2. Check Timestamp
    if 'timestamp' not in data:
        errors.append("Missing 'timestamp'")
    elif not isinstance(data['timestamp'], str):
        errors.append("'timestamp' is not a string")

    # 3. Check Value
    if 'value' not in data:
        errors.append("Missing 'value'")
    elif not isinstance(data['value'], (int, float)):
        errors.append(f"'value' is not a number (got {data['value']})")

    # 4. Check Priority (optional field)
    if 'priority' in data and data['priority'] not in VALID_PRIORITIES:
        errors.append(f"Invalid priority '{data['priority']}'")

    return errors


def process_file(filepath: str) -> tuple[int, int]:
    """
    Process a single file and print invalid lines with their reasons.
    Returns (valid_count, invalid_count).
    """
    valid_count = 0
    invalid_count = 0

    with open(filepath, 'r') as f:
        for line_num, line in enumerate(f, start=1):
            if not line.strip():
                continue

            try:
                data = json.loads(line)
                errors = validate_line(data)

                if not errors:
                    valid_count += 1
                else:
                    invalid_count += 1
                    print(f"❌ Line {line_num} INVALID: {', '.join(errors)}")
                    print(f"   Raw data: {line.strip()}")

            except json.JSONDecodeError as e:
                invalid_count += 1
                print(f"❌ Line {line_num} INVALID: Broken JSON syntax -> {e}")
                print(f"   Raw data: {line.strip()}")

    print(f"\n✅ Valid measurements:   {valid_count}")
    print(f"❌ Invalid measurements: {invalid_count}")

    return valid_count, invalid_count


def print_separator(label: str = ""):
    width = 42
    print("\n" + "═" * width)
    if label:
        print(f"  {label}")
        print("═" * width)


def detect_invalid_packets():
    parser = argparse.ArgumentParser(
        description="Find invalid telemetry JSON lines and explain why. "
                    "Pass a file path to inspect a single file, or a directory "
                    "path to inspect all .txt files inside it."
    )
    parser.add_argument(
        "path",
        help="Path to a single telemetry file or a directory containing telemetry files"
    )
    parser.add_argument(
        "--ext",
        default=".txt",
        help="File extension to filter when scanning a directory (default: .txt)"
    )
    args = parser.parse_args()

    target = args.path

    # ── Single file mode ──────────────────────────────────────────────────────
    if os.path.isfile(target):
        print_separator(f"Processing: {os.path.basename(target)}")
        try:
            process_file(target)
        except FileNotFoundError:
            print(f"❌ Error: Could not find the file '{target}'")
            sys.exit(1)
        return

    # ── Directory mode ────────────────────────────────────────────────────────
    if os.path.isdir(target):
        files = sorted(
            f for f in os.listdir(target)
            if f.endswith(args.ext) and os.path.isfile(os.path.join(target, f))
        )

        if not files:
            print(f"⚠️  No *{args.ext} files found in '{target}'")
            sys.exit(0)

        total_valid = 0
        total_invalid = 0

        for filename in files:
            filepath = os.path.join(target, filename)
            print_separator(f"Processing: {filename}")
            try:
                valid, invalid = process_file(filepath)
                total_valid += valid
                total_invalid += invalid
            except Exception as e:
                print(f"❌ Could not process '{filename}': {e}")

        # Global summary across all files
        print_separator("GLOBAL SUMMARY")
        print(f"  Files processed:        {len(files)}")
        print(f"  Total ✅ valid:         {total_valid}")
        print(f"  Total ❌ invalid:       {total_invalid}")
        print()
        return

    # ── Neither ───────────────────────────────────────────────────────────────
    print(f"❌ Error: '{target}' is not a valid file or directory.")
    sys.exit(1)


if __name__ == "__main__":
    detect_invalid_packets()