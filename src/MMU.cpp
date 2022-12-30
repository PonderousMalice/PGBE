#include "MMU.h"

namespace emulator
{
	uint8_t MMU::read(uint16_t adr)
	{
		return get_host_adr(adr);
	}

	void MMU::write(uint16_t adr, uint8_t v)
	{
		uint8_t& p = get_host_adr(adr);

		if (&p != &_null)
		{
			p = v;
		}
	}

	uint8_t& MMU::get_host_adr(uint16_t gb_adr)
	{
		if (gb_adr <= 0x0100 && boot_rom)
		{
			return boot_rom->at(gb_adr);
		}
		else if (gb_adr >= 0x0104 && gb_adr <= 0x133)
		{
			return _logo.at(gb_adr - 0x104);
		}
		else if (gb_adr > 0x0100 && gb_adr <= 0x3FFF && _rom_bank_00)
		{
			return _rom_bank_00->at(gb_adr);
		}
		else if (gb_adr > 0x3FFF && gb_adr <= 0x7FFF && _rom_bank_01)
		{
			return _rom_bank_01->at(gb_adr - 0x4000);
		}
		else if (gb_adr > 0x7FFF && gb_adr <= 0x9FFF)
		{
			return _vram->at(gb_adr - 0x8000);
		}
		else if (gb_adr > 0x9FFF && gb_adr <= 0xBFFF && _external_ram)
		{
			return _external_ram->at(gb_adr - 0xA000);
		}
		else if (gb_adr > 0xBFFF && gb_adr <= 0xDFFF)
		{
			return _wram->at(gb_adr - 0xC000);
		}
		else if (gb_adr > 0xDFFF && gb_adr <= 0xFDFF)
		{
			return _wram->at(gb_adr - 0xE000);
		}
		else if (gb_adr > 0xFDFF && gb_adr <= 0xFE9F)
		{
			return OAM->at(gb_adr - 0xFE00);
		}
		else if (gb_adr > 0xFEFF && gb_adr <= 0xFF7F)
		{
			return _io_reg->at(gb_adr - 0xFF00);
		}
		else if (gb_adr > 0xFF7F && gb_adr <= 0xFFFE)
		{
			return _hram->at(gb_adr - 0xFF80);
		}
		else if (gb_adr == 0xFFFF)
		{
			return _ie_reg;
		}

		return _null;
	}
}

