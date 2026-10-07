import os, sys

os.chdir(os.path.dirname(os.path.abspath(__file__)))
sys.stdout.reconfigure(encoding='utf-8', errors='replace')

files = [
    'ShowMenuTask.c',
    'ShowTimeTask.c',
    'ShowFlashLigntTask.c',
    'ShowDHT11.c',
    'ShowClockTimeTask.c',
    'ShowSettingTask.c',
    'ShowWoodenFish.c',
]

for fname in files:
    with open(fname, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()
    garbled = []
    for i, line in enumerate(lines, 1):
        if '�' in line:
            garbled.append((i, line.rstrip()))
    print(f"\n=== {fname} ({len(garbled)} garbled lines) ===")
    for ln, txt in garbled[:15]:
        print(f"  L{ln}: {txt}")
    if len(garbled) > 15:
        print(f"  ... and {len(garbled)-15} more")
