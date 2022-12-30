#pragma once
#include "MMU.h"
#include <functional>

namespace emulator
{
	class SM83
	{
	public:
		SM83(MMU* m) :
			_registers()
		{
			_mmu = m;
			_cycle_count = 0;
			_ime = false;
		}

		int tick();
		void dump(std::FILE* f);
	private:
		MMU* _mmu;
		struct registers
		{
			registers()
			{
				AF = BC = DE = HL = SP = PC = 0;
			}

			union
			{
				struct
				{
					union
					{
						struct flag
						{
							uint8_t : 4; // padding
							uint8_t c : 1;
							uint8_t h : 1;
							uint8_t n : 1;
							uint8_t z : 1;
						} flags;
						uint8_t F;
					};
					uint8_t A;
				};
				uint16_t AF;
			};

			union
			{
				struct
				{
					uint8_t C;
					uint8_t B;
				};
				uint16_t BC;
			};

			union
			{
				struct
				{
					uint8_t E;
					uint8_t D;
				};
				uint16_t DE;
			};

			union
			{
				struct
				{
					uint8_t L;
					uint8_t H;
				};
				uint16_t HL;
			};

			uint16_t SP;
			uint16_t PC;
		} _registers;
		int _cycle_count;
		bool _ime;

		// RAM access
		uint8_t read(uint16_t adr);
		uint8_t read_byte();
		uint16_t read_word();
		void write(uint16_t adr, uint8_t v);
		void write(uint16_t adr, uint16_t v);

		void ALU(uint8_t y, uint8_t val);
		bool CC(uint8_t y);
		// Interrupt handler
		//void ISR();

		void set_half_carry_flag_sub(uint8_t old_value, uint8_t new_value);
		void set_half_carry_flag_add8(uint8_t val1, uint8_t val2);
		void set_half_carry_flag_add16(uint8_t val1, uint8_t val2);

		// Conditions
		bool NZ();
		bool Z();
		bool NC();
		bool C();

		// ALU
		void ADD(uint8_t v);
		void ADC(uint8_t v);
		void SUB(uint8_t v);
		void SBC(uint8_t v);
		void AND(uint8_t v);
		void XOR(uint8_t v);
		void OR(uint8_t v);
		void CP(uint8_t v);

		// JUMP
		void CALL(uint16_t adr);
		void JUMP(uint16_t adr);
		void RET();

		// Rotate and shi(f)t instructions
		void RLC(uint8_t& v);
		void RRC(uint8_t& v);
		void RL(uint8_t& v);
		void RR(uint8_t& v);
		void SLA(uint8_t& v);
		void SRA(uint8_t& v);
		void SWAP(uint8_t& v);
		void SRL(uint8_t& v);

		const std::array<std::function<bool(SM83&)>, 4> _cc
		{
			&SM83::NZ,
			&SM83::Z,
			&SM83::NC,
			&SM83::C
		};

		const std::array<std::function<void(SM83&, uint8_t)>, 8> _alu
		{
			&SM83::ADD,
			&SM83::ADC,
			&SM83::SUB,
			&SM83::SBC,
			&SM83::AND,
			&SM83::XOR,
			&SM83::OR,
			&SM83::CP
		};

		const std::array<std::function<void(SM83&, uint8_t&)>, 8> _rot
		{
			&SM83::RLC,
			&SM83::RRC,
			&SM83::RL,
			&SM83::RR,
			&SM83::SLA,
			&SM83::SRA,
			&SM83::SWAP,
			&SM83::SRL
		};

		uint8_t& R(int i)
		{
			switch (i)
			{
			case 0:
				return _registers.B;
			case 1:
				return _registers.C;
			case 2:
				return _registers.D;
			case 3:
				return _registers.E;
			case 4:
				return _registers.H;
			case 5:
				return _registers.L;
			case 6:
				return _mmu->get_host_adr(_registers.HL);
			case 7:
				return _registers.A;

			}
		}

		const std::array<uint16_t*, 4> _rp
		{
			&_registers.BC,
			&_registers.DE,
			&_registers.HL,
			&_registers.SP
		};

		const std::array<uint16_t*, 4> _rp2
		{
			&_registers.BC,
			&_registers.DE,
			&_registers.HL,
			&_registers.AF
		};
	};
}

