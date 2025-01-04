import time
import os
import argparse

# select & compile circuit
parser = argparse.ArgumentParser(description='Generate witness and proof')
parser.add_argument('--circuit', default='recursion', help='Circuit name (default: recursion)')
args = parser.parse_args()

os.chdir(args.circuit)
os.system('nargo compile')

# witness generation
witness_start = time.time()
os.system(f'nargo execute {args.circuit}.gz')
witness_end = time.time()

# proof generation
proof_start = time.time()
os.system(f'bb prove -b ./target/{args.circuit}.json -w ./target/{args.circuit}.gz -o ./target/proof')
proof_end = time.time()

print(f"Witness generation time: {(witness_end - witness_start) * 1000} ms")
print(f"Proof generation time: {(proof_end - proof_start) * 1000} ms")