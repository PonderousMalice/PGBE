import subprocess

PGBE_path = "C:\\Users\\David\\Documents\\Dev\\PGBE\\builddir\\PGBE.exe"
PGBE_args = "--headless --rom={my_rom}"

def test_blarggs_cpu_instrs():
    rom_path = "test-roms\\blargg's-gb-tests\\cpu_instrs\\cpu_instrs.gb"
    
    output = subprocess.Popen([PGBE_path, PGBE_args.format(my_rom = rom_path)])
    assert False