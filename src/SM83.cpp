#include "SM83.h"
#include "utils.h"
#include <fmt/core.h>

namespace emulator
{
	int SM83::tick()
	{
		if (_registers.PC > 0xFF)
		{
			exit(0);
		}

		_cycle_count = 0;
		auto opcode = read_byte();

		uint8_t x, y, z, p, q;
		uint32_t nnnn = 0;
		uint16_t nn = 0;
		uint8_t n = 0;
		int8_t d = 0;
		uint8_t* ptr = nullptr;

		x = (opcode & 0xC0) >> 6;
		y = (opcode & 0x38) >> 3;
		z = (opcode & 0x07);
		p = (opcode & 0x30) >> 4;
		q = (opcode & 0x08) >> 3;

		if (opcode == 0xCB)
		{
			opcode = read_byte();

			x = (opcode & 0xC0) >> 6;
			y = (opcode & 0x38) >> 3;
			z = (opcode & 0x07);

			switch (x)
			{
			case 0:
				// Roll/shift register or memory location
				// rot[y] r[z]
				_rot[y](*this, R(z));
				break;
			case 1:
				// Test bit : set the zero flag if bit is *not* set.
				// BIT y, r[z]
				_registers.flags.z = !isSetBit(R(z), y);
				_registers.flags.n = false;
				_registers.flags.h = true;
				break;
			case 2:
				// Reset bit
				// RES y, r[z]
				clearBit(R(z), y);
				break;
			case 3:
				// Set bit
				// SET y, r[z]
				setBit(R(z), y);
				break;
			}

			return _cycle_count;
		}

		switch (x)
		{
		case 0:
			switch (z)
			{
			case 0:
				// Relative jumps and assorted ops
				switch (y)
				{
				case 0:
					// NOP
					break;
				case 1:
					// LD (nn), SP
					nn = read_byte();
					write(nn, _registers.SP);
					break;
				case 2:
					// STOP
					break;
				case 3:
					// JR d
					d = read_byte();
					JUMP(_registers.PC + d);
					break;
					// 4..7
				case 4:
				case 5:
				case 6:
				case 7:
					// JR cc[y - 4], d
					d = read_byte();
					if (CC(y))
					{
						JUMP(_registers.PC + d);
					}
					break;
				}
				break;
			case 1:
				// 16-bit load immediate/add
				if (q == 0)
				{
					// LD rp[p], nn
					nn = read_word();
					*(_rp[p]) = nn;
				}
				else
				{
					// ADD HL, rp[p]
					nn = _registers.HL + *(_rp[p]);
					_registers.flags.c = nn > 0xFF;
					set_half_carry_flag_add8(_registers.HL, *(_rp[p]));
					_registers.flags.n = false;
					_registers.HL = nn;
				}
				break;
			case 2:
				// Indirect loading
				if (q == 0)
				{
					switch (p)
					{
					case 0:
						// LD (BC),A
						write(_registers.BC, _registers.A);
						break;
					case 1:
						// LD (DE),A
						write(_registers.DE, _registers.A);
						break;
					case 2:
						// LD (HL+),A
						write(_registers.HL++, _registers.A);
						break;
					case 3:
						// LD (HL-),A
						write(_registers.HL--, _registers.A);
						break;
					}
				}
				else
				{
					switch (p)
					{
					case 0:
						// LD A, (BC)
						_registers.A = read(_registers.BC);
						break;
					case 1:
						// LD A, (DE)
						_registers.A = read(_registers.DE);
						break;
					case 2:
						// LD A, (HL+)
						_registers.A = read(_registers.HL++);
						break;
					case 3:
						// LD A, (HL-)
						_registers.A = read(_registers.HL--);
						break;
					}
				}
				break;
			case 3:
				// 16-bit INC/DEC
				if (q == 0)
				{
					// INC rp[p]
					// TO FIX
					++(*(_rp[p]));
				}
				else
				{
					// DEC rp[p]
					// TO FIX
					--(*(_rp[p]));
				}
				break;
			case 4:
				// 8-bit INC
				// INC r[y]
				n = R(y) + 1;
				_registers.flags.z = n == 0;
				_registers.flags.n = false;
				set_half_carry_flag_add8(R(y), 1);
				R(y) = n;
				break;
			case 5:
				// 8-bit DEC
				// DEC r[y]
				n = R(y);
				--(R(y));
				_registers.flags.z = R(y) == 0;
				set_half_carry_flag_sub(n, R(y));
				_registers.flags.n = true;
				break;
			case 6:
				// 8-bit load immediate
				// LD r[y], n
				n = read_byte();
				R(y) = n;
				break;
			case 7:
				// Assorted operations on accumulator/flags
				switch (y)
				{
				case 0:
					// RLCA
					RLC(_registers.A);
					_registers.flags.z = false;
					break;
				case 1:
					// RRCA
					RRC(_registers.A);
					_registers.flags.z = false;
					break;
				case 2:
					// RLA
					RL(_registers.A);
					_registers.flags.z = false;
					break;
				case 3:
					// RRA
					RR(_registers.A);
					_registers.flags.z = false;
					break;
				case 4:
				{ // DAA
					n = _registers.A;
					uint8_t correction = 0;
					bool prevAdd = !_registers.flags.n; // was previous instr an addition ?

					// (+) Add 6 to each digit greater than 9, or if it carried
					// (-) Subtract 6 from each digit greater than 9, or if it carried
					if (_registers.flags.h || (prevAdd && (n & 0x0F) > 9))
					{
						correction |= 0x06;
					}
					if (_registers.flags.c || (prevAdd && (n > 0x99)))
					{
						correction |= 0x60;
						_registers.flags.c = true;
					}

					n += _registers.flags.n ? -correction : correction;
					_registers.flags.z = n == 0;
					_registers.A = n;
				}
				break;
				case 5:
					// CPL
					_registers.flags.h = true;
					_registers.flags.n = true;
					_registers.A ^= 0xFF;
					break;
				case 6:
					// SCF
					_registers.flags.h = false;
					_registers.flags.n = false;
					_registers.flags.c = true;
					break;
				case 7:
					// CCF
					_registers.flags.h = false;
					_registers.flags.n = false;
					_registers.flags.c ^= 0x01;
					break;
				}
				break;
			}
			break;
		case 1:
			if (z == 6 && y == 6)
			{
				// Exception (replaces LD (HL), (HL))
				// HALT
			}
			else
			{
				// 8-bit loading
				// LD r[y], r[z]
				R(y) = R(z);
			}
			break;
		case 2:
			// Operate on accumulator and register/memory location
			// alu[y] r[z]
			ALU(y, R(z));
			break;
		case 3:
			switch (z)
			{
			case 0:
				// Conditional return, mem-mapped register loads and stack operations
				switch (y)
				{
				case 0:
				case 1:
				case 2:
				case 3:
					// RET cc[y]
					if (CC(y))
					{
						RET();
					}
					break;
				case 4:
					// LD (0xFF00 + n), A
					n = read_byte();
					write(0xFF00 + n, _registers.A);
					break;
				case 5:
					// ADD SP, d
					d = read_byte();
					nnnn = _registers.SP + d;

					_registers.flags.z = false;
					_registers.flags.n = false;
					set_half_carry_flag_add8(_registers.SP, d);
					_registers.flags.c = ((_registers.SP & 0xFF) + d) >= 0x100;

					_registers.SP = (uint16_t)nnnn;
					break;
				case 6:
					// LD A, (0xFF00 + n)
					n = read_byte();
					_registers.A = read(0xFF00 + n);
					break;
				case 7:
					// LD HL, SP + d
					d = read_byte();
					nnnn = _registers.SP + d;

					_registers.flags.z = false;
					_registers.flags.n = false;
					set_half_carry_flag_add8(_registers.SP, d);
					_registers.flags.c = ((_registers.SP & 0xFF) + d) >= 0x100;

					_registers.HL = (uint16_t)nnnn;
					break;
				}
				break;
			case 1:
				// POP & various ops
				if (q == 0)
				{
					// POP rp2[p]
					nn = read(_registers.SP++);
					*(_rp2[p]) = combine(nn, read(_registers.SP++));
				}
				else
				{
					switch (p)
					{
					case 0:
						// RET
						RET();
						break;
					case 1:
						// RETI
						RET();
						_ime = true;
						break;
					case 2:
						// JP HL
						JUMP(_registers.HL);
						break;
					case 3:
						// LD SP, HL
						_registers.HL = _registers.SP;
						break;
					}
				}
				break;
			case 2:
				// Conditional jump
				switch (y)
				{
				case 0:
				case 1:
				case 2:
				case 3:
					// JP cc[y], nn
					nn = read_word();
					if (CC(y))
					{
						JUMP(nn);
					}
				case 4:
					// LD (0xFF00+C), A
					write(0xFF00 + _registers.C, _registers.A);
					break;
				case 5:
					// LD (nn), A
					nn = read_word();
					write(nn, _registers.A);
					break;
				case 6:
					// LD A, (0xFF00+C)
					_registers.A = read(0xFF00 + _registers.C);
					break;
				case 7:
					// LD A, (nn)
					nn = read_word();
					_registers.A = read(nn);
					break;
				}
				break;
			case 3:
				// assorted operations
				switch (y)
				{
				case 0:
					// JP nn
					nn = read_word();
					JUMP(nn);
					break;
				case 6:
					// DI
					_ime = false;
					break;
				case 7:
					// EI
					_ime = true;
					break;
				}
				break;
			case 4:
				// Conditional call
				// CALL cc[y], nn
				nn = read_word();
				if (CC(y))
				{
					CALL(nn);
				}
				break;
			case 5:
				// PUSH & various ops
				if (q == 0)
				{
					// PUSH rp2[p]
					nn = *(_rp2[p]);
					_registers.SP -= 2;
					write(_registers.SP, nn);
				}
				else
				{
					if (p == 0)
					{
						// CALL nn
						nn = read_word();
						CALL(nn);
					}
				}
				break;
			case 6:
				// alu[y] n
				n = read_byte();
				ALU(y, n);
				break;
			case 7:
				// Restart
				// RST y*8
				nn = y * 8;
				CALL(nn);
				break;
			}
			break;
		}

		return _cycle_count;
	}

	uint8_t SM83::read_byte()
	{
		return read(_registers.PC++);
	}

	uint16_t SM83::read_word()
	{
		auto lsb = read_byte();
		auto msb = read_byte();

		return combine(lsb, msb);
	}

	uint8_t SM83::read(uint16_t adr)
	{
		_cycle_count++;
		return _mmu->read(adr);
	}

	void SM83::write(uint16_t adr, uint8_t v)
	{
		_cycle_count++;
		_mmu->write(adr, v);
	}

	void SM83::write(uint16_t adr, uint16_t v)
	{
		auto lsb = LSB(v);
		auto msb = MSB(v);

		write(adr, lsb);
		write(adr + 1, msb);
	}

	void SM83::set_half_carry_flag_sub(uint8_t old_value, uint8_t new_value)
	{
		_registers.flags.h = ((old_value & 0x0F) - (new_value & 0x0F)) < 0;
	}

	void SM83::set_half_carry_flag_add8(uint8_t value1, uint8_t value2)
	{
		_registers.flags.h = ((value1 & 0x0F) + (value2 & 0x0F)) >= 0x10;
	}

	void SM83::set_half_carry_flag_add16(uint8_t value1, uint8_t value2)
	{
		_registers.flags.h = ((value1 & 0x0FFF) + (value2 & 0x0FFF)) >= 0x1000;
	}

	bool SM83::NZ()
	{
		return !_registers.flags.z;
	}

	bool SM83::Z()
	{
		return _registers.flags.z;
	}

	bool SM83::NC()
	{
		return !_registers.flags.c;
	}

	bool SM83::C()
	{
		return _registers.flags.c;
	}

	void SM83::ADD(uint8_t v)
	{
		uint16_t tmp = _registers.A + v;

		_registers.flags.z = tmp == 0;
		_registers.flags.n = false;
		set_half_carry_flag_add8(_registers.A, v);
		_registers.flags.c = tmp >= 0x100;

		_registers.A = (uint8_t)tmp;
	}

	void SM83::ADC(uint8_t v)
	{
		uint16_t tmp = _registers.A + v + _registers.flags.c;

		_registers.flags.z = tmp == 0;
		_registers.flags.n = false;
		set_half_carry_flag_add8(_registers.A, v + _registers.flags.c);
		_registers.flags.c = tmp >= 0x100;

		_registers.A = (uint8_t)tmp;
	}

	void SM83::SUB(uint8_t v)
	{
		int16_t tmp = _registers.A - v;

		_registers.flags.z = tmp == 0;
		_registers.flags.n = true;
		// _registers.flags.h = false;
		set_half_carry_flag_sub(_registers.A, tmp);
		_registers.flags.c = tmp < 0;

		_registers.A = (uint8_t)tmp;
	}

	void SM83::SBC(uint8_t v)
	{
		int16_t tmp = _registers.A - v - _registers.flags.c;

		_registers.flags.z = tmp == 0;
		_registers.flags.n = true;
		set_half_carry_flag_sub(_registers.A, tmp);
		_registers.flags.c = tmp < 0;

		_registers.A = (uint8_t)tmp;
	}

	void SM83::AND(uint8_t v)
	{
		_registers.A &= v;

		_registers.flags.z = _registers.A == 0;
		_registers.flags.n = false;
		_registers.flags.h = true;
		_registers.flags.c = false;
	}

	void SM83::XOR(uint8_t v)
	{
		_registers.A ^= v;

		_registers.flags.z = _registers.A == 0;
		_registers.flags.n = false;
		_registers.flags.h = false;
		_registers.flags.c = false;
	}

	void SM83::OR(uint8_t v)
	{
		_registers.A |= v;

		_registers.flags.z = _registers.A == 0;
		_registers.flags.n = false;
		_registers.flags.h = false;
		_registers.flags.c = false;
	}

	void SM83::CP(uint8_t v)
	{
		int16_t tmp = _registers.A - v;

		_registers.flags.z = tmp == 0;
		_registers.flags.n = true;
		set_half_carry_flag_sub(_registers.A, tmp);
		_registers.flags.c = tmp < 0;
	}

	void SM83::CALL(uint16_t adr)
	{
		_registers.SP -= 2;
		write(_registers.SP, _registers.PC);
		JUMP(adr);
	}

	void SM83::JUMP(uint16_t adr)
	{
		_registers.PC = adr;
	}

	void SM83::RET()
	{
		JUMP(read(_registers.SP));
		_registers.SP += 2;
	}

	void SM83::RLC(uint8_t& v)
	{
		_registers.flags.h = false;
		_registers.flags.n = false;
		_registers.flags.c = v & 0x80;

		v <<= 1;
		v |= _registers.flags.c;

		_registers.flags.z = v == 0;
	}

	void SM83::RRC(uint8_t& v)
	{
		_registers.flags.h = false;
		_registers.flags.n = false;
		_registers.flags.c = v & 0x01;

		v >>= 1;
		v |= (_registers.flags.c << 8);

		_registers.flags.z = v == 0;
	}

	void SM83::RL(uint8_t& v)
	{
		uint8_t n = _registers.flags.c;
		_registers.flags.h = false;
		_registers.flags.n = false;
		_registers.flags.c = (v & 0x80) == 0x80;

		v <<= 1;
		v |= n;

		_registers.flags.z = (v == 0);
	}

	void SM83::RR(uint8_t& v)
	{
		uint8_t n = _registers.flags.c;
		_registers.flags.h = false;
		_registers.flags.n = false;
		_registers.flags.c = v & 0x01;

		v >>= 1;
		v |= (n << 8);
		_registers.flags.z = v == 0;
	}

	void SM83::SLA(uint8_t& v)
	{
		_registers.flags.h = false;
		_registers.flags.n = false;
		// TO FIX
		_registers.flags.c = v & 0x80;
		v <<= 1;
		_registers.flags.z = v == 0;
	}

	void SM83::SRA(uint8_t& v)
	{
		uint8_t b7 = v & 0x80;
		_registers.flags.h = false;
		_registers.flags.n = false;
		_registers.flags.c = v & 0x01;
		v >>= 1;
		v &= b7;
		_registers.flags.z = (int8_t)v == 0;
	}

	void SM83::SWAP(uint8_t& v)
	{
		auto lsb = (v & 0x0F);
		auto msb = (v & 0xF0);
		v = lsb | msb;

		_registers.flags.h = false;
		_registers.flags.n = false;
		_registers.flags.c = false;
		_registers.flags.z = v == 0;
	}

	void SM83::SRL(uint8_t& v)
	{
		_registers.flags.h = false;
		_registers.flags.n = false;
		_registers.flags.c = v & 0x01;
		v >>= 1;
		_registers.flags.z = v == 0;
	}

	void SM83::dump(std::FILE* f)
	{
		fmt::print(f, "A: {:02X} F: {:02X} B: {:02X} C: {:02X} D: {:02X} E: {:02X} H: {:02X} L: {:02X} SP: {:04X} PC: 00:{:04X}",
			_registers.A, _registers.F, _registers.B, _registers.C, _registers.D, _registers.E,
			_registers.H, _registers.L, _registers.SP, _registers.PC);

		for (int i = 0; i < 4; ++i)
		{
			if (i == 0)
			{
				fmt::print(f, " (");
			}
			else
			{
				fmt::print(f, " ");
			}
			fmt::print(f, "{:02X}", read(_registers.PC + i));
		}

		fmt::print(f, ")\n");
	}

	void SM83::ALU(uint8_t y, uint8_t val)
	{
		_alu[y](*this, val);
	}

	bool SM83::CC(uint8_t y)
	{
		return _cc[y - 4](*this);
	}
}