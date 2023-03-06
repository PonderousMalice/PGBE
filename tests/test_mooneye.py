import subprocess

PGBE_path = "builddir/PGBE"
PGBE_args = "--headless --rom={my_rom}"

ROM_prefix = "tests/rom/mooneye"

def test_blarggs_cpu_instrs():
    rom_path = ROM_prefix + "aceptance/add_sp_e_timing.gb"
    
    output = subprocess.Popen([PGBE_path, PGBE_args.format(my_rom = rom_path)])
    assert False