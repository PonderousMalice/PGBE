#include "SM83.h"
#include "utils.h"
#include <fmt/core.h>

namespace emulator
{
    int SM83::tick()
    {
        _cycle_count = 0;

        _cur_instr.name = "";
        _cur_instr.arg1 = "";
        _cur_instr.arg2 = "";

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
                _rot.at(y)(*this, r(z));

                _cur_instr.arg1 = r_str(z);
                break;
            case 1:
                // Test bit : set the zero flag if bit is *not* set.
                // BIT y, r[z]
                _cur_instr.name = "BIT";
                _cur_instr.arg1 = fmt::format("({:02X})", y);
                _cur_instr.arg2 = r_str(z);

                _registers.flags.z = !isSetBit(r(z), y);
                _registers.flags.n = false;
                _registers.flags.h = true;
                break;
            case 2:
                // Reset bit
                // RES y, r[z]
                _cur_instr.name = "RES";
                _cur_instr.arg1 = fmt::format("({:02X})", y);
                _cur_instr.arg2 = r_str(z);
               
                clearBit(r(z), y);
                break;
            case 3:
                // Set bit
                // SET y, r[z]
                _cur_instr.name = "SET";
                _cur_instr.arg1 = fmt::format("({:02X})", y);
                _cur_instr.arg2 = r_str(z);

                fmt::print("SET {}, {}\n", y, r_str(z));

                setBit(r(z), y);
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
                    _cur_instr.name = "NOP";
                    break;
                case 1:
                    // LD (nn), SP
                    nn = read_byte();
                    write(nn, _registers.SP);

                    _cur_instr.name = "LD";
                    _cur_instr.arg1 = fmt::format("({:04X})", nn);
                    _cur_instr.arg2 = "SP";
                    break;
                case 2:
                    // STOP
                    _cur_instr.name = "STOP";
                    break;
                case 3:
                    // JR d
                    d = read_byte();
                    JUMP(_registers.PC + d);

                    _cur_instr.name = "JR";
                    _cur_instr.arg1 = fmt::format("({:02X})", d);
                    break;
                    // 4..7
                case 4:
                case 5:
                case 6:
                case 7:
                    // JR cc[y - 4], d
                    d = read_byte();
                    if (CC(y - 4))
                    {
                        JUMP(_registers.PC + d);
                    }

                    _cur_instr.name = "JR";
                    _cur_instr.arg1 = CC_str(y - 4);
                    _cur_instr.arg2 = fmt::format("({:02X})", d);
                    break;
                }
                break;
            case 1:
                // 16-bit load immediate/add
                if (q == 0)
                {
                    // LD rp[p], nn
                    nn = read_word();
                    *(_rp.at(p)) = nn;

                    _cur_instr.name = "LD";
                    _cur_instr.arg1 = rp_str(p);
                    _cur_instr.arg2 = fmt::format("{:04X}", nn);
                }
                else
                {
                    // ADD HL, rp[p]
                    nnnn = _registers.HL + *(_rp.at(p));
                    _registers.flags.c = nnnn > 0xFF;
                    set_half_carry_flag_add8(_registers.HL, *(_rp.at(p)));
                    _registers.flags.n = false;
                    _registers.HL = (uint16_t)nnnn;

                    _cur_instr.name = "ADD";
                    _cur_instr.arg1 = "HL";
                    _cur_instr.arg2 = rp_str(p);
                }
                break;
            case 2:
                // Indirect loading
                if (q == 0)
                {
                    _cur_instr.name = "LD"; 
                    _cur_instr.arg2 = "A";

                    switch (p)
                    {
                    case 0:
                        // LD (BC),A
                        write(_registers.BC, _registers.A);
                        _cur_instr.arg1 = "(BC)";
                        break;
                    case 1:
                        // LD (DE),A
                        write(_registers.DE, _registers.A);
                        _cur_instr.arg1 = "(DE)";
                        break;
                    case 2:
                        // LD (HL+),A
                        write(_registers.HL++, _registers.A);
                        _cur_instr.arg1 = "(HL+)";
                        break;
                    case 3:
                        // LD (HL-),A
                        write(_registers.HL--, _registers.A);
                        _cur_instr.arg1 = "(HL-)";
                        break;
                    }
                }
                else
                {
                    _cur_instr.name = "LD";
                    _cur_instr.arg1 = "A";

                    switch (p)
                    {
                    case 0:
                        // LD A, (BC)
                        _registers.A = read(_registers.BC);
                        _cur_instr.arg2 = "(BC)";
                        break;
                    case 1:
                        // LD A, (DE)
                        _registers.A = read(_registers.DE);
                        _cur_instr.arg2 = "(DE)";
                        break;
                    case 2:
                        // LD A, (HL+)
                        _registers.A = read(_registers.HL++);
                        _cur_instr.arg2 = "(HL+)";
                        break;
                    case 3:
                        // LD A, (HL-)
                        _registers.A = read(_registers.HL--);
                        _cur_instr.arg2 = "(HL-)";
                        break;
                    }
                }
                break;
            case 3:
                // 16-bit INC/DEC
                _cur_instr.arg1 = rp_str(p);
                if (q == 0)
                {
                    // INC rp[p]
                    ++(*(_rp.at(p)));
                    _cur_instr.name = "INC";
                }
                else
                {
                    // DEC rp[p]
                    --(*(_rp.at(p)));
                    _cur_instr.name = "DEC";
                }
                break;
            case 4:
                // 8-bit INC
                // INC r[y]
                n = r(y) + 1;
                _registers.flags.z = n == 0;
                _registers.flags.n = false;
                set_half_carry_flag_add8(r(y), 1);
                r(y) = n;

                _cur_instr.name = "INC";
                _cur_instr.arg1 = r_str(y);
                break;
            case 5:
                // 8-bit DEC
                // DEC r[y]
                n = r(y);
                --(r(y));
                _registers.flags.z = r(y) == 0;
                set_half_carry_flag_sub(n, r(y));
                _registers.flags.n = true;

                _cur_instr.name = "DEC";
                _cur_instr.arg1 = r_str(y);
                break;
            case 6:
                // 8-bit load immediate
                // LD r[y], n
                n = read_byte();
                r(y) = n;

                _cur_instr.name = "LD";
                _cur_instr.arg1 = r_str(y);
                _cur_instr.arg2 = fmt::format("{:02X}", n);
                break;
            case 7:
                // Assorted operations on accumulator/flags
                switch (y)
                {
                case 0:
                    // RLCA
                    RLC(_registers.A);
                    _registers.flags.z = false;

                    _cur_instr.name = "RLCA";
                    break;
                case 1:
                    // RRCA
                    RRC(_registers.A);
                    _registers.flags.z = false;

                    _cur_instr.name = "RRC";
                    break;
                case 2:
                    // RLA
                    RL(_registers.A);
                    _registers.flags.z = false;

                    _cur_instr.name = "RLA";
                    break;
                case 3:
                    // RRA
                    RR(_registers.A);
                    _registers.flags.z = false;

                    _cur_instr.name = "RRA";
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

                    _cur_instr.name = "DAA";
                }
                break;
                case 5:
                    // CPL
                    _registers.flags.h = true;
                    _registers.flags.n = true;
                    _registers.A ^= 0xFF;

                    _cur_instr.name = "CPL";
                    break;
                case 6:
                    // SCF
                    _registers.flags.h = false;
                    _registers.flags.n = false;
                    _registers.flags.c = true;

                    _cur_instr.name = "SCF";
                    break;
                case 7:
                    // CCF
                    _registers.flags.h = false;
                    _registers.flags.n = false;
                    _registers.flags.c ^= 0x01;

                    _cur_instr.name = "CCF";
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
                _cur_instr.name = "HALT";
            }
            else
            {
                // 8-bit loading
                // LD r[y], r[z]
                r(y) = r(z);

                _cur_instr.name = "LD";
                _cur_instr.arg1 = r_str(y);
                _cur_instr.arg2 = r_str(z);
            }
            break;
        case 2:
            // Operate on accumulator and register/memory location
            // alu[y] r[z]
            ALU(y, r(z));

            _cur_instr.arg2 = r_str(z);
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

                    _cur_instr.name = "RET";
                    _cur_instr.arg1 = CC_str(y);
                    break;
                case 4:
                    // LD (0xFF00 + n), A
                    n = read_byte();
                    write(0xFF00 + n, _registers.A);

                    _cur_instr.name = "LD";
                    _cur_instr.arg1 = fmt::format("(0xFF00 + {:02X})", n);
                    _cur_instr.arg2 = "A";
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

                    _cur_instr.name = "ADD";
                    _cur_instr.arg1 = "SP";
                    _cur_instr.arg2 = fmt::format("{:02X}", d);
                    break;
                case 6:
                    // LD A, (0xFF00 + n)
                    n = read_byte();
                    _registers.A = read(0xFF00 + n);

                    _cur_instr.name = "LD";
                    _cur_instr.arg1 = "A";
                    _cur_instr.arg2 = fmt::format("(0xFF00 + {:02X})", n);
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

                    _cur_instr.name = "LD";
                    _cur_instr.arg1 = "HL";
                    _cur_instr.arg2 = fmt::format("(SP + {:02X})", d);
                    break;
                }
                break;
            case 1:
                // POP & various ops
                if (q == 0)
                {
                    // POP rp2[p]
                    nn = read(_registers.SP++);
                    *(_rp2.at(p)) = combine(nn, read(_registers.SP++));

                    _cur_instr.name = "POP";
                    _cur_instr.arg1 = rp2_str(p);
                }
                else
                {
                    switch (p)
                    {
                    case 0:
                        // RET
                        RET();
                        _cur_instr.name = "RET";
                        break;
                    case 1:
                        // RETI
                        RET();
                        _cur_instr.name = "RETI";
                        _ime = true;
                        break;
                    case 2:
                        // JP HL
                        JUMP(_registers.HL);
                        _cur_instr.name = "JP";
                        _cur_instr.arg1 = "HL";
                        break;
                    case 3:
                        // LD SP, HL
                        _registers.HL = _registers.SP;
                        _cur_instr.name = "LD";
                        _cur_instr.arg1 = "SP";
                        _cur_instr.arg2 = "HL";
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
                    _cur_instr.name = "JP";
                    _cur_instr.arg1 = CC_str(y);
                    _cur_instr.arg2 = fmt::format("{:04X}", nn);
                case 4:
                    // LD (0xFF00+C), A
                    write(0xFF00 + _registers.C, _registers.A);
                    _cur_instr.name = "LD";
                    _cur_instr.arg1 = "(0xFF00+C)";
                    _cur_instr.arg2 = "A";
                    break;
                case 5:
                    // LD (nn), A
                    nn = read_word();
                    write(nn, _registers.A);
                    _cur_instr.name = "LD";
                    _cur_instr.arg1 = fmt::format("({:04X})", nn);
                    _cur_instr.arg2 = "A";
                    break;
                case 6:
                    // LD A, (0xFF00+C)
                    _registers.A = read(0xFF00 + _registers.C);
                    _cur_instr.name = "LD";
                    _cur_instr.arg2 = "(0xFF00+C)";
                    _cur_instr.arg1 = "A";
                    break;
                case 7:
                    // LD A, (nn)
                    nn = read_word();
                    _registers.A = read(nn);
                    _cur_instr.name = "LD";
                    _cur_instr.arg2 = fmt::format("({:04X})", nn);
                    _cur_instr.arg1 = "A";
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
                    _cur_instr.name = "JP";
                    _cur_instr.arg1 = fmt::format("({:04X})", nn);
                    break;
                case 6:
                    // DI
                    _ime = false;
                    _cur_instr.name = "DI";
                    break;
                case 7:
                    // EI
                    _ime = true;
                    _cur_instr.name = "EI";
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
                _cur_instr.name = "CALL";
                _cur_instr.arg1 = CC_str(y);
                _cur_instr.arg2 = fmt::format("({:04X})", nn);
                break;
            case 5:
                // PUSH & various ops
                if (q == 0)
                {
                    // PUSH rp2[p]
                    nn = *(_rp2.at(p));
                    _registers.SP -= 2;
                    write(_registers.SP, nn);

                    _cur_instr.name = "PUSH";
                    _cur_instr.arg1 = rp2_str(p);
                }
                else
                {
                    if (p == 0)
                    {
                        // CALL nn
                        nn = read_word();
                        CALL(nn);

                        _cur_instr.name = "CALL";
                        _cur_instr.arg1 = fmt::format("({:04X})", nn);
                    }
                }
                break;
            case 6:
                // alu[y] n
                n = read_byte();
                ALU(y, n);
                _cur_instr.arg1 = fmt::format("({:02X})", n);
                break;
            case 7:
                // Restart
                // RST y*8
                nn = y * 8;
                CALL(nn);
                _cur_instr.name = "RST";
                _cur_instr.arg1 = fmt::format("({:04X})", nn);
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

        _registers.flags.z = (tmp & (0x00FF)) == 0;
        _registers.flags.n = false;
        set_half_carry_flag_add8(_registers.A, v);
        _registers.flags.c = tmp >= 0x100;

        _registers.A = (uint8_t)tmp;

        _cur_instr.name = "ADD";
    }

    void SM83::ADC(uint8_t v)
    {
        uint16_t tmp = _registers.A + v + _registers.flags.c;

        _registers.flags.z = (tmp & (0x00FF)) == 0;
        _registers.flags.n = false;
        set_half_carry_flag_add8(_registers.A, v + _registers.flags.c);
        _registers.flags.c = tmp >= 0x100;

        _registers.A = (uint8_t)tmp;

        _cur_instr.name = "ADC";
    }

    void SM83::SUB(uint8_t v)
    {
        int16_t tmp = _registers.A - v;

        _registers.flags.z = (tmp & (0x00FF)) == 0;
        _registers.flags.n = true;
        set_half_carry_flag_sub(_registers.A, tmp);
        _registers.flags.c = tmp < 0;

        _registers.A = (uint8_t)tmp;

        _cur_instr.name = "SUB";
    }

    void SM83::SBC(uint8_t v)
    {
        int16_t tmp = _registers.A - v - _registers.flags.c;

        _registers.flags.z = (tmp & (0x00FF)) == 0;
        _registers.flags.n = true;
        set_half_carry_flag_sub(_registers.A, tmp);
        _registers.flags.c = tmp < 0;

        _registers.A = (uint8_t)tmp;

        _cur_instr.name = "SBC";
    }

    void SM83::AND(uint8_t v)
    {
        _registers.A &= v;

        _registers.flags.z = _registers.A == 0;
        _registers.flags.n = false;
        _registers.flags.h = true;
        _registers.flags.c = false;

        _cur_instr.name = "AND";
    }

    void SM83::XOR(uint8_t v)
    {
        _registers.A ^= v;

        _registers.flags.z = _registers.A == 0;
        _registers.flags.n = false;
        _registers.flags.h = false;
        _registers.flags.c = false;

        _cur_instr.name = "XOR";
    }

    void SM83::OR(uint8_t v)
    {
        _registers.A |= v;

        _registers.flags.z = _registers.A == 0;
        _registers.flags.n = false;
        _registers.flags.h = false;
        _registers.flags.c = false;

        _cur_instr.name = "OR";
    }

    void SM83::CP(uint8_t v)
    {
        int16_t tmp = _registers.A - v;

        _registers.flags.z = (tmp & (0x00FF)) == 0;
        _registers.flags.n = true;
        set_half_carry_flag_sub(_registers.A, tmp);
        _registers.flags.c = tmp < 0;

        _cur_instr.name = "CP";
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

        _cur_instr.name = "RLC";
    }

    void SM83::RRC(uint8_t& v)
    {
        _registers.flags.h = false;
        _registers.flags.n = false;
        _registers.flags.c = v & 0x01;

        v >>= 1;
        v |= (_registers.flags.c << 8);

        _registers.flags.z = v == 0;

        _cur_instr.name = "RRC";
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

        _cur_instr.name = "RL";
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

        _cur_instr.name = "RR";
    }

    void SM83::SLA(uint8_t& v)
    {
        _registers.flags.h = false;
        _registers.flags.n = false;
        // TO FIX
        _registers.flags.c = v & 0x80;
        v <<= 1;
        _registers.flags.z = v == 0;

        _cur_instr.name = "SLA";
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

        _cur_instr.name = "SRA";
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

        _cur_instr.name = "SWAP";
    }

    void SM83::SRL(uint8_t& v)
    {
        _registers.flags.h = false;
        _registers.flags.n = false;
        _registers.flags.c = v & 0x01;
        v >>= 1;
        _registers.flags.z = v == 0;

        _cur_instr.name = "SRL";
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

    void SM83::print_dis(std::FILE* f)
    {
        fmt::print(f, "{} {},{}\n", _cur_instr.name, _cur_instr.arg1, _cur_instr.arg2);
    }

    void SM83::ALU(uint8_t y, uint8_t val)
    {
        _alu.at(y)(*this, val);
    }

    bool SM83::CC(uint8_t y)
    {
        return _cc.at(y)(*this);
    }

    std::string SM83::CC_str(int y)
    {
        switch (y)
        {
        case 0:
            return "NZ";
        case 1:
            return "Z";
        case 2:
            return "NC";
        case 3:
            return "C";
        }
    }

    std::string SM83::rp_str(int i)
    {
        switch (i)
        {
        case 0:
            return "BC";
        case 1:
            return "DE";
        case 2:
            return "HL";
        case 3:
            return "SP";
        }
    }

    std::string SM83::rp2_str(int i)
    {
        switch (i)
        {
        case 0:
            return "BC";
        case 1:
            return "DE";
        case 2:
            return "HL";
        case 3:
            return "AF";
        }
    }
}