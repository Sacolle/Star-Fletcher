import numpy as np
import struct
import os
import sys
from pathlib import Path

def parse_rsf(header_path):
    params = {}
    with open(header_path, 'r') as f:
        for line in f:
            if '=' in line:
                key, val = line.split('=')
                key = key.strip()
                # Remove quotes and whitespace
                val = val.strip().strip('"').strip("'")
                
                # Try to convert to int, then float, else keep as string
                if val.isdigit():
                    params[key] = int(val)
                else:
                    try:
                        params[key] = float(val)
                    except ValueError:
                        params[key] = val
    return params

def congeal_with_rsf(spec, input_binary_files, final_output_path):

    print(f"spec: {spec}")   
    # Extract dimensions
    n1, n2, n3, n4 = spec['n1'], spec['n2'], spec['n3'], spec['n4']
    seg = spec['seg']
    
    # Map the data format to numpy types
    dtype = np.float32 if spec['data_format'] == "native_float" else np.float64

    # Sub-cube dimensions
    sn1, sn2, sn3 = n1 // seg, n2 // seg, n3 // seg

    print(f"data type: {dtype}")   
    print(f"sn1: {sn1}, sn2: {sn2}, sn3: {sn3}")   
    # 2. Create/Prepare the large output file
    # We create a shape (time, depth, height, width) -> (n4, n1, n2, n3)
    master = np.memmap(final_output_path, dtype=dtype, mode='w+', shape=(n4, n1, n2, n3))
    
    # 3. Process each input file
    header_struct = struct.Struct('QQQQ') # size_t i, j, k, t
    
    for bin_file in input_binary_files:
        with open(bin_file, 'rb') as f:
            while True:
                # Read header
                raw_head = f.read(header_struct.size)
                if not raw_head: break
                
                i, j, k, t = header_struct.unpack(raw_head)

                i_start = i * sn1;
                j_start = j * sn2;
                k_start = k * sn3;
                # print(i, j, k, t)
                # Read data block
                num_elements = sn1 * sn2 * sn3
                raw_data = f.read(num_elements * np.dtype(dtype).itemsize)
                
                # Convert to numpy and reshape to a 3D block
                cube = np.frombuffer(raw_data, dtype=dtype).reshape(sn1, sn2, sn3)
                # print(cube)
                
                # Place the cube into the master volume using slicing
                # This automatically handles the "non-contiguous" logic
                master[t, k_start:(k_start + sn3), j_start:(j_start + sn2), i_start:(i_start + sn1)] = cube

    # Ensure everything is written to disk
    master.flush()
    # print(master)
    print(f"Successfully congealed into {final_output_path}")

# Usage
# congeal_with_rsf("result/out-VTI.rsf", ["part1.bin", "part2.bin"], "final_volume.bin")

def main():
    if len(sys.argv) < 2:
        print("missing arg")
        return
    
    rsf_file = sys.argv[1]

    attributes = parse_rsf(rsf_file)

    rsf_bin_file = attributes['in']

    files_dir = Path(rsf_file).resolve().parent

    seg_files = list(files_dir.glob("*.bin"))

    congeal_with_rsf(attributes, seg_files, rsf_bin_file)


main()
