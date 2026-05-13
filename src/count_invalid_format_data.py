import json
import argparse
import sys

# Script used to detect  defected measurements for DataIngestor testing 

# Example of output
#python3 ../src/count_invalid_format_data.py ../src/collector_output/raw_data_1777721417_0000.txt 
#❌ Line 4 INVALID: Missing required fields -> timestamp
#   Raw data: {"sensor_id": "ACCEL-004", "value": 321.924, "priority": "HIGH"}
#❌ Line 38 INVALID: Broken JSON syntax -> Invalid control character at: line 1 column 8 (char 7)
#   Raw data: {"times
#❌ Line 45 INVALID: Missing required fields -> value
#   Raw data: {"timestamp": "2026-05-02T11:30:16Z", "sensor_id": "HUM-009", "priority": "LOW"}
#
# ==============================
#           SUMMARY
# ==============================
# ✅ Valid measurements:   47
# ❌ Invalid measurements: 3

def detectInvalidPackets():
    parser = argparse.ArgumentParser(description="Find invalid telemetry JSON lines and explain why.")
    parser.add_argument("filename", help="Path to the text file containing the telemetry data")
    args = parser.parse_args()

    valid_count = 0
    invalid_count = 0

    try:
        with open(args.filename, 'r') as f:
            for line_num, line in enumerate(f, start=1):
                if not line.strip():
                    continue 
                
                try:
                    data = json.loads(line)
                    
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

                    # 4. Check Priority (if it exists)
                    if 'priority' in data and data['priority'] not in ["HIGH", "MEDIUM", "LOW"]:
                        errors.append(f"Invalid priority '{data['priority']}'")
                    
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

        print("\n" + "="*30)
        print("          SUMMARY")
        print("="*30)
        print(f"✅ Valid measurements:   {valid_count}")
        print(f"❌ Invalid measurements: {invalid_count}")

    except FileNotFoundError:
        print(f"❌ Error: Could not find the file '{args.filename}'")
        sys.exit(1)

if __name__ == "__main__":
    detectInvalidPackets()