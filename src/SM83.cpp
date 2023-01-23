#include "SM83.h"
#include "utils.h"
#include <fmt/core.h>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace emulator
{
    template<class... Ts> struct overload : Ts... { using Ts::operator()...; };

    SM83::SM83(MMU* mmu, Timer* t, uint8_t& rIF, uint8_t& rIE) :
        m_registers(),
        m_ime(false),
        m_ime_new_value(false),
        m_halted(false),
        m_mmu(mmu),
        m_timer(t),
        m_IF(rIF),
        m_IE(rIE)
    {}

    void SM83::run()
    {
        if (m_halted)
        {
            m_advance_cycle();
            if ((m_IF & m_IE) != 0)
            {
                m_halted = false;
            }
            else
            {
                return;
            }
        }

        m_isr();

        auto opcode = m_fetch();
        auto instr = m_decode(opcode);
        m_execute(instr);
    }

    OP SM83::m_decode(uint8_t opcode)
    {
        uint8_t x = (opcode & 0xC0) >> 6,
            y = (opcode & 0x38) >> 3,
            z = (opcode & 0x07),
            p = (opcode & 0x30) >> 4,
            q = (opcode & 0x08) >> 3;

        OP res
        {
            .y = y
        };

        if (opcode == 0xCB)
        {
            opcode = m_fetch();

            x = (opcode & 0xC0) >> 6;
            y = (opcode & 0x38) >> 3;
            z = (opcode & 0x07);

            res.y = y;
            res.args[0] = m_r.at(z);

            switch (x)
            {
            case 0:
                // Roll/shift register or memory location
                // rot[y] r[z]
                res.name = OP::ROT;
                break;
            case 1:
                // Test bit : set the zero flag if bit is *not* set.
                // BIT y, r[z]
                res.name = OP::BIT;
                break;
            case 2:
                // Reset bit
                // RES y, r[z]
                res.name = OP::RES;
                break;
            case 3:
                // Set bit
                // SET y, r[z]
                res.name = OP::SET;
                break;
            }
        }
        else
        {
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
                        res.name = OP::NOP;
                        break;
                    case 1:
                        // LD (nn), SP
                        res.name = OP::LD;
                        res.args[0] = { nn, true };
                        res.args[1] = { SP, false };
                        break;
                    case 2:
                        // STOP
                        res.name = OP::STOP;
                        break;
                    case 3:
                        // JR d
                        res.name = OP::JR;
                        break;
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                        res.name = OP::JR_CC;
                        break;
                    }
                    break;
                case 1:
                    // 16-bit load immediate/add
                    if (q)
                    {
                        // ADD HL, rp[p]
                        res.name = OP::ADD_16;
                        res.args[0] = { HL, false };
                        res.args[1] = m_rp.at(p);
                    }
                    else
                    {
                        // LD rp[p], nn
                        res.name = OP::LD;
                        res.args[0] = m_rp.at(p);
                        res.args[1] = { nn, false };
                    }
                    break;
                case 2:
                    // Indirect loading
                    res.name = OP::LD;
                    if (q)
                    {
                        res.args[0] = { A, false };
                        switch (p)
                        {
                        case 0:
                            // LD A, (BC)
                            res.args[1] = { BC, true };
                            break;
                        case 1:
                            // LD A, (DE)
                            res.args[1] = { DE, true };
                            break;
                        case 2:
                            // LD A, (HL+)
                            res.args[1] = { HL_inc, true };
                            break;
                        case 3:
                            // LD A, (HL-)
                            res.args[1] = { HL_dec, true };
                            break;
                        }
                    }
                    else
                    {
                        res.args[1] = { A, false };
                        switch (p)
                        {
                        case 0:
                            // LD (BC),A
                            res.args[0] = { BC, true };
                            break;
                        case 1:
                            // LD (DE),A
                            res.args[0] = { DE, true };
                            break;
                        case 2:
                            // LD (HL+),A
                            res.args[0] = { HL_inc, true };
                            break;
                        case 3:
                            // LD (HL-),A
                            res.args[0] = { HL_dec, true };
                            break;
                        }
                    }
                    break;
                case 3:
                    // 16-bit INC/DEC
                    // INC rp[p]
                    // DEC rp[p]
                    res.name = q ? OP::DEC_16 : OP::INC_16;
                    res.args[0] = m_rp.at(p);
                    break;
                case 4:
                    // 8-bit INC
                    // INC r[y]
                    res.name = OP::INC;
                    res.args[0] = m_r.at(y);
                    break;
                case 5:
                    // 8-bit DEC
                    // DEC r[y]
                    res.name = OP::DEC;
                    res.args[0] = m_r.at(y);
                    break;
                case 6:
                    // 8-bit load immediate
                    // LD r[y], n
                    res.name = OP::LD;
                    res.args[0] = m_r.at(y);
                    res.args[1] = { n, false };
                    break;
                case 7:
                    // Assorted operations on accumulator/flags
                    res.args[0] = { A, false };
                    switch (y)
                    {
                    case 0:
                        // RLCA
                        res.name = OP::RLCA;
                        break;
                    case 1:
                        // RRCA
                        res.name = OP::RRCA;
                        break;
                    case 2:
                        // RLA
                        res.name = OP::RLA;
                        break;
                    case 3:
                        // RRA
                        res.name = OP::RRA;
                        break;
                    case 4:
                        res.name = OP::DAA;
                        break;
                    case 5:
                        res.name = OP::CPL;
                        break;
                    case 6:
                        res.name = OP::SCF;
                        break;
                    case 7:
                        res.name = OP::CCF;
                        break;
                    }
                    break;
                }
                break;
            case 1:
                if (z == 6 && y == 6)
                {
                    res.name = OP::HALT;
                }
                else
                {
                    // 8-bit loading
                    // LD r[y], r[z]
                    res.name = OP::LD;
                    res.args[0] = m_r.at(y);
                    res.args[1] = m_r.at(z);
                }
                break;
            case 2:
                // Operate on accumulator and register/memory location
                // alu[y] r[z]
                res.name = OP::ALU;
                res.args[0] = m_r.at(z);
                break;
            case 3:
                switch (z)
                {
                case 0:
                    // Conditional return, 
                    // mem-mapped register loads and stack operations
                    switch (y)
                    {
                    case 0:
                    case 1:
                    case 2:
                    case 3:
                        // RET cc[y]
                        res.name = OP::RET_CC;
                        break;
                    case 4:
                        // LD (0xFF00 + n), A
                        res.name = OP::LD;
                        res.args[0] = { n, true };
                        res.args[1] = { A, false };
                        break;
                    case 5:
                        // ADD SP, d
                        res.name = OP::ADD_16;
                        res.args[0] = { SP, false };
                        break;
                    case 6:
                        // LD A, (0xFF00 + n)
                        res.name = OP::LD;
                        res.args[0] = { A, false };
                        res.args[1] = { n, true };
                        break;
                    case 7:
                        // LD HL, SP + d
                        res.name = OP::LD;
                        res.args[0] = { HL, false };
                        res.args[1] = { SP_d, false };
                        break;
                    }
                    break;
                case 1:
                    // POP & various ops
                    if (q)
                    {
                        switch (p)
                        {
                        case 0:
                            // RET
                            res.name = OP::RET;
                            break;
                        case 1:
                            // RETI
                            res.name = OP::RETI;
                            break;
                        case 2:
                            // JP HL
                            res.name = OP::JP_HL;
                            break;
                        case 3:
                            // LD SP, HL
                            res.name = OP::LD;
                            res.args[0] = { SP, false };
                            res.args[1] = { HL, false };
                            break;
                        }
                    }
                    else
                    {
                        // POP rp2[p]
                        res.name = OP::POP;
                        res.args[0] = m_rp2.at(p);
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
                        res.name = OP::JP_CC;
                        res.args[0] = { nn, false };
                        break;
                    case 4:
                        // LD (0xFF00+C), A
                        res.name = OP::LD;
                        res.args[0] = { C, true };
                        res.args[1] = { A, false };
                        break;
                    case 5:
                        // LD (nn), A
                        res.name = OP::LD;
                        res.args[0] = { nn, true };
                        res.args[1] = { A, false };
                        break;
                    case 6:
                        // LD A, (0xFF00+C)
                        res.name = OP::LD;
                        res.args[0] = { A, false };
                        res.args[1] = { C, true };
                        break;
                    case 7:
                        // LD A, (nn)
                        res.name = OP::LD;
                        res.args[0] = { A, false };
                        res.args[1] = { nn, true };
                        break;
                    }
                    break;
                case 3:
                    // assorted operations
                    switch (y)
                    {
                    case 0:
                        // JP nn
                        res.name = OP::JP;
                        res.args[0] = { nn, false };
                        break;
                    case 6:
                        // DI
                        res.name = OP::DI;
                        break;
                    case 7:
                        // EI
                        res.name = OP::EI;
                        break;
                    default:
                        // Illegal OP
                        break;
                    }
                    break;
                case 4:
                    // Conditional call
                    // CALL cc[y], nn
                    res.name = OP::CALL_CC;
                    break;
                case 5:
                    // PUSH & various ops
                    if (q)
                    {
                        res.name = OP::CALL;
                    }
                    else
                    {
                        res.name = OP::PUSH;
                        res.args[0] = m_rp2.at(p);
                    }
                    break;
                case 6:
                    res.name = OP::ALU;
                    res.args[0] = { n, false };
                    break;
                case 7:
                    res.name = OP::RST;
                    break;
                }
                break;
            }
        }

        return res;
    }

    void SM83::m_execute(OP instr)
    {
        uint8_t y = instr.y;
        auto arg1 = instr.args[0];
        auto arg2 = instr.args[1];

        switch (instr.name)
        {
        case OP::ALU:
            m_alu.at(y)(std::get<uint8_t>(m_get_reg(arg1)));
            break;
        case OP::ADD_16:
            m_add_16(arg1, arg2);
            break;
        case OP::BIT:
            m_bit(std::get<uint8_t>(m_get_reg(arg1)), y);
            break;
        case OP::RES:
        {
            uint8_t tmp = std::get<uint8_t>(m_get_reg(arg1));
            m_res(tmp, y);
            m_set_reg(arg1, tmp);
        }
        break;
        case OP::SET:
        {
            uint8_t tmp = std::get<uint8_t>(m_get_reg(arg1));
            m_set(tmp, y);
            m_set_reg(arg1, tmp);
        }
        break;
        case OP::NOP:
            break;
        case OP::CALL:
            m_call(m_fetch_word());
            break;
        case OP::CALL_CC:
            m_call(m_fetch_word(), m_cc.at(y));
            break;
        case OP::DAA:
            m_daa();
            break;
        case OP::CPL:
            m_cpl();
            break;
        case OP::SCF:
            m_scf();
            break;
        case OP::CCF:
            m_ccf();
            break;
        case OP::ROT:
        {
            uint8_t tmp = std::get<uint8_t>(m_get_reg(arg1));
            m_rot.at(y)(tmp);
            m_set_reg(arg1, tmp);
        }
        break;
        case OP::DEC:
        {
            uint8_t tmp = std::get<uint8_t>(m_get_reg(arg1));
            m_dec(&tmp);
            m_set_reg(arg1, tmp);
        }
        break;
        case OP::DEC_16:
        {
            uint16_t tmp = std::get<uint16_t>(m_get_reg(arg1));
            m_dec(&tmp);
            m_set_reg(arg1, tmp);
        }
        break;
        case OP::INC:
        {
            uint8_t tmp = std::get<uint8_t>(m_get_reg(arg1));
            m_inc(&tmp);
            m_set_reg(arg1, tmp);
        }
        break;
        case OP::INC_16:
        {
            uint16_t tmp = std::get<uint16_t>(m_get_reg(arg1));
            m_inc(&tmp);
            m_set_reg(arg1, tmp);
        }
        break;
        case OP::DI:
            m_ime_new_value = false;
            break;
        case OP::EI:
            m_ime_new_value = true;
            break;
        case OP::HALT:
            m_halt();
            break;
        case OP::JP:
            m_jp();
            break;
        case OP::JP_CC:
            m_jp(m_cc.at(y));
            break;
        case OP::JP_HL:
            m_registers.PC = m_registers.HL;
            break;
        case OP::JR:
            m_jr();
            break;
        case OP::JR_CC:
            m_jr(m_cc.at(y - 4));
            break;
        case OP::LD:
            m_ld(arg1, arg2);
            break;
        case OP::POP:
        {
            uint16_t tmp = std::get<uint16_t>(m_get_reg(arg1));
            m_pop(tmp);
            m_set_reg(arg1, tmp);

            if (arg1.name == AF)
            {
                m_registers.F &= 0xF0;
            }
        }
        break;
        case OP::PUSH:
            m_push(std::get<uint16_t>(m_get_reg(arg1)));
            break;
        case OP::RET:
            m_ret();
            break;
        case OP::RET_CC:
            m_ret(m_cc.at(y));
            break;
        case OP::RETI:
            m_reti();
            break;
        case OP::RLA:
            m_rla();
            break;
        case OP::RLCA:
            m_rlca();
            break;
        case OP::RRA:
            m_rra();
            break;
        case OP::RRCA:
            m_rrca();
            break;
        case OP::RST:
            m_call(y * 8);
            break;
        case OP::STOP:
            m_stop();
            break;
        default:
            std::cout << "no op : " << instr.name;
            break;
        }
    }

    uint8_t SM83::m_fetch()
    {
        return m_read(m_registers.PC++);
    }

    uint16_t SM83::m_fetch_word()
    {
        auto lsb = m_fetch();
        auto msb = m_fetch();

        return combine(lsb, msb);
    }

    uint8_t SM83::m_read(uint16_t adr)
    {
        m_advance_cycle();
        if (adr == 0xFF44 && g_debug)
        {
            return 0x90;
        }

        return m_mmu->read(adr);
    }

    reg_v SM83::m_get_reg(reg_s r)
    {
        switch (r.name)
        {
        case A:
            return m_registers.A;
        case B:
            return m_registers.B;
        case C:
            if (r.indirect)
            {
                return m_read(0xFF00 + m_registers.C);
            }
            return m_registers.C;
        case D:
            return m_registers.D;
        case E:
            return m_registers.E;
        case H:
            return m_registers.H;
        case L:
            return m_registers.L;
        case AF:
            if (r.indirect)
            {
                return m_read(m_registers.AF);
            }
            return m_registers.AF;
        case BC:
            if (r.indirect)
            {
                return m_read(m_registers.BC);
            }
            return m_registers.BC;
        case DE:
            if (r.indirect)
            {
                return m_read(m_registers.DE);
            }
            return m_registers.DE;
        case HL:
            if (r.indirect)
            {
                return m_read(m_registers.HL);
            }
            return m_registers.HL;
        case HL_inc:
            if (r.indirect)
            {
                return m_read(m_registers.HL++);
            }
            return m_registers.HL++;
        case HL_dec:
            if (r.indirect)
            {
                return m_read(m_registers.HL--);
            }
            return m_registers.HL--;
        case SP:
            if (r.indirect)
            {
                return m_read(m_registers.SP);
            }
            return m_registers.SP;
        case SP_d:
            if (r.indirect)
            {
                return m_read(m_registers.SP + (int8_t)m_fetch());
            }
            return uint16_t(m_registers.SP + (int8_t)m_fetch());
        case n:
            if (r.indirect)
            {
                return m_read(0xFF00 + m_fetch());
            }
            return m_fetch();
        case nn:
            if (r.indirect)
            {
                return m_read(m_fetch_word());
            }
            return m_fetch_word();
            return m_fetch_word();
        default:
            throw "pute";
        }
    }

    void SM83::m_write(uint16_t adr, uint8_t v)
    {
        m_advance_cycle();
        m_mmu->write(adr, v);
    }

    void SM83::m_set_reg(reg_s r, reg_v v)
    {
        uint16_t adr = 0;

        switch (r.name)
        {
        case A:
            m_registers.A = std::get<uint8_t>(v);
            break;
        case B:
            m_registers.B = std::get<uint8_t>(v);
            break;
        case C:
            if (r.indirect)
            {
                adr = 0xFF00 + m_registers.C;
            }
            else
            {
                m_registers.C = std::get<uint8_t>(v);
            }
            break;
        case D:
            m_registers.D = std::get<uint8_t>(v);
            break;
        case E:
            m_registers.E = std::get<uint8_t>(v);
            break;
        case H:
            m_registers.H = std::get<uint8_t>(v);
            break;
        case L:
            m_registers.L = std::get<uint8_t>(v);
            break;
        case AF:
            if (r.indirect)
            {
                adr = m_registers.AF;
            }
            else
            {
                m_registers.AF = std::get<uint16_t>(v);
            }
            break;
        case BC:
            if (r.indirect)
            {
                adr = m_registers.BC;
            }
            else
            {
                m_registers.BC = std::get<uint16_t>(v);
            }
            break;
        case DE:
            if (r.indirect)
            {
                adr = m_registers.DE;
            }
            else
            {
                m_registers.DE = std::get<uint16_t>(v);
            }
            break;
        case HL:
            if (r.indirect)
            {
                adr = m_registers.HL;
            }
            else
            {
                m_registers.HL = std::get<uint16_t>(v);
            }
            break;
        case HL_inc:
            if (r.indirect)
            {
                adr = m_registers.HL;
            }
            else
            {
                m_registers.HL = std::get<uint16_t>(v);
            }
            m_registers.HL++;
            break;
        case HL_dec:
            if (r.indirect)
            {
                adr = m_registers.HL;
            }
            else
            {
                m_registers.HL = std::get<uint16_t>(v);
            }
            m_registers.HL--;
            break;
        case SP:
            if (r.indirect)
            {
                adr = m_registers.SP;
            }
            else
            {
                m_registers.SP = std::get<uint16_t>(v);
            }
            break;
        case n:
            if (r.indirect)
            {
                adr = 0xFF00 + m_fetch();
            }
            break;
        case nn:
            if (r.indirect)
            {
                adr = m_fetch_word();
            }
            break;
        default:
            throw "kekw";
        }

        if (r.indirect)
        {
            if (std::holds_alternative<uint16_t>(v))
            {
                uint16_t tmp = std::get<uint16_t>(v);
                m_write(adr, LSB(tmp));
                m_write(adr + 1, MSB(tmp));
            }
            else
            {
                m_write(adr, std::get<uint8_t>(v));
            }
        }
    }

    bool SM83::m_nz()
    {
        return !m_registers.flags.z;
    }

    bool SM83::m_z()
    {
        return m_registers.flags.z;
    }

    bool SM83::m_nc()
    {
        return !m_registers.flags.c;
    }

    bool SM83::m_c()
    {
        return m_registers.flags.c;
    }

    void SM83::m_ld(reg_s lv, reg_s rv)
    {
        auto v = m_get_reg(rv);
        m_set_reg(lv, v);

        if (lv.name == HL && rv.name == SP_d)
        {
            uint16_t spd_v = std::get<uint16_t>(v);
            int8_t d = spd_v - m_registers.SP;

            m_registers.flags.h = ((m_registers.SP & 0x0F) + (d & 0x0F)) > 0x0F;
            m_registers.flags.c = ((m_registers.SP & 0xFF) + (d & 0xFF)) > 0xFF;
            m_registers.flags.z = false;
            m_registers.flags.n = false;
        }
    }

    void SM83::m_dec(reg_t v)
    {
        std::visit(overload
            {
                [](uint16_t* arg) { --(*arg); },
                [this](uint8_t* arg)
                {
                    auto old = (*arg)--;
                    m_registers.flags.n = true;
                    m_registers.flags.z = (*arg == 0);
                    m_registers.flags.h = ((old & 0x0F) - (*arg & 0x0F)) < 0;
                },
            }, v);
    }

    void SM83::m_inc(reg_t v)
    {
        std::visit(overload
            {
                [](uint16_t* arg) { ++(*arg); },
                [this](uint8_t* arg)
                {
                    auto old = (*arg)++;
                    m_registers.flags.n = false;
                    m_registers.flags.z = (*arg == 0);
                    m_registers.flags.h = ((old & 0x0F) == 0x0F);
                },
            }, v);
    }

    void SM83::m_add(uint8_t v)
    {
        uint16_t tmp = m_registers.A + v;

        m_registers.flags.z = ((tmp & 0xFF) == 0);
        m_registers.flags.n = false;
        m_registers.flags.h = ((m_registers.A & 0x0F) + (v & 0x0F)) >= 0x10;
        m_registers.flags.c = (tmp > 0xFF);

        m_registers.A = (uint8_t)tmp;
    }

    void SM83::m_add_16(reg_s lv, reg_s rv)
    {
        uint32_t res = 0;
        m_registers.flags.h = false;

        if (lv.name == SP)
        {
            int8_t d = m_fetch();
            res = m_registers.SP + d;
            m_registers.flags.h = ((m_registers.SP & 0x0F) + (d & 0x0F)) >= 0x10;
            m_registers.flags.c = ((m_registers.SP & 0xFF) + (d & 0xFF)) >= 0x100;
            m_registers.SP = res;
        }
        else if (lv.name == HL)
        {
            auto v = std::get<uint16_t>(m_get_reg(rv));
            res = m_registers.HL + v;
            m_registers.flags.h = ((m_registers.HL & 0x0FFF) + (v & 0x0FFF)) >= 0x1000;

            m_registers.HL = res;
            m_registers.flags.c = (res > 0xFFFF);
        }

        if (lv.name == SP || rv.name == SP_d)
        {
            m_registers.flags.z = false;
        }

        m_registers.flags.n = false;
    }

    void SM83::m_adc(uint8_t v)
    {
        uint16_t tmp = m_registers.A + v + m_registers.flags.c;

        m_registers.flags.z = ((tmp & 0xFF) == 0);
        m_registers.flags.n = false;
        m_registers.flags.h = ((m_registers.A & 0x0F) + (v & 0x0F) + m_registers.flags.c) >= 0x10;
        m_registers.flags.c = (tmp > 0xFF);

        m_registers.A = (uint8_t)tmp;
    }

    void SM83::m_sub(uint8_t v)
    {
        int16_t tmp = m_registers.A - v;

        m_registers.flags.z = ((tmp & 0xFF) == 0);
        m_registers.flags.n = true;
        m_registers.flags.h = ((m_registers.A & 0x0F) - (tmp & 0x0F)) < 0;
        m_registers.flags.c = (tmp < 0);

        m_registers.A = (uint8_t)tmp;
    }

    void SM83::m_sbc(uint8_t v)
    {
        int16_t tmp = m_registers.A - v - m_registers.flags.c;

        m_registers.flags.z = ((tmp & 0xFF) == 0);
        m_registers.flags.n = true;
        m_registers.flags.h = ((m_registers.A & 0x0F) - (v & 0x0F) - m_registers.flags.c) < 0;
        m_registers.flags.c = (tmp < 0);

        m_registers.A = (uint8_t)tmp;
    }

    void SM83::m_and(uint8_t v)
    {
        m_registers.A &= v;

        m_registers.flags.z = (m_registers.A == 0);
        m_registers.flags.n = false;
        m_registers.flags.h = true;
        m_registers.flags.c = false;
    }

    void SM83::m_xor(uint8_t v)
    {
        m_registers.A ^= v;

        m_registers.flags.z = (m_registers.A == 0);
        m_registers.flags.n = false;
        m_registers.flags.h = false;
        m_registers.flags.c = false;
    }

    void SM83::m_or(uint8_t v)
    {
        m_registers.A |= v;

        m_registers.flags.z = (m_registers.A == 0);
        m_registers.flags.n = false;
        m_registers.flags.h = false;
        m_registers.flags.c = false;
    }

    void SM83::m_cp(uint8_t v)
    {
        int16_t tmp = m_registers.A - v;

        m_registers.flags.z = (tmp & (0x00FF)) == 0;
        m_registers.flags.n = true;
        m_registers.flags.h = ((m_registers.A & 0x0F) - (tmp & 0x0F)) < 0;
        m_registers.flags.c = (tmp < 0);
    }

    void SM83::m_cpl()
    {
        m_registers.flags.h = true;
        m_registers.flags.n = true;
        m_registers.A ^= 0xFF;
    }

    void SM83::m_scf()
    {
        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c = true;
    }

    void SM83::m_ccf()
    {
        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c ^= 0x01;
    }

    void SM83::m_daa()
    {
        uint8_t n = m_registers.A;

        if (m_registers.flags.n)
        {
            if (m_registers.flags.c)
            {
                n -= 0x60;
            }

            if (m_registers.flags.h)
            {
                n -= 0x06;
            }
        }
        else
        {
            if (m_registers.flags.c || n > 0x99)
            {
                n += 0x60;
                m_registers.flags.c = true;
            }

            if (m_registers.flags.h || (n & 0x0F) > 0x09)
            {
                n += 0x06;
            }
        }

        m_registers.flags.z = ((n & 0xFF) == 0);
        m_registers.flags.h = 0;

        m_registers.A = n;
    }

    void SM83::m_call(uint16_t nn, std::function<bool(void)> cond)
    {
        if (!cond || cond())
        {
            m_push(m_registers.PC);
            m_registers.PC = nn;
        }
    }

    void SM83::m_jp(std::function<bool(void)> cond)
    {
        uint16_t nn = m_fetch_word();
        if (!cond || cond())
        {
            m_registers.PC = nn;
        }
    }

    void SM83::m_jr(std::function<bool(void)> cond)
    {
        int8_t d = m_fetch();
        if (!cond || cond())
        {
            m_registers.PC += d;
        }
    }

    void SM83::m_ret(std::function<bool(void)> cond)
    {
        uint8_t lsb = m_read(m_registers.SP);
        uint8_t msb = m_read(m_registers.SP + 1);

        if (!cond || cond())
        {
            m_registers.PC = combine(lsb, msb);
            m_registers.SP += 2;
        }
    }

    void SM83::m_reti()
    {
        m_ret();
        m_ime = true;
    }

    void SM83::m_rlc(uint8_t& v)
    {
        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c = (v & 0x80) > 0;

        v <<= 1;
        if (m_registers.flags.c)
        {
            set_bit(v, 0);
        }

        m_registers.flags.z = (v == 0 && m_registers.flags.c == false);
    }

    void SM83::m_rrc(uint8_t& v)
    {
        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c = (v & 0x01) > 0;

        v >>= 1;
        if (m_registers.flags.c)
        {
            set_bit(v, 7);
        }

        m_registers.flags.z = (v == 0);
    }

    void SM83::m_rl(uint8_t& v)
    {
        uint8_t n = m_registers.flags.c;
        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c = ((v & 0x80) == 0x80);

        v <<= 1;
        v |= n;

        m_registers.flags.z = (v == 0);
    }

    void SM83::m_rr(uint8_t& v)
    {
        bool carry = m_registers.flags.c;
        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c = (v & 0x01);

        v >>= 1;
        if (carry)
        {
            set_bit(v, 7);
        }

        m_registers.flags.z = (v == 0);
    }

    void SM83::m_sla(uint8_t& v)
    {
        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c = (v & 0x80) > 0;
        
        v <<= 1;
        m_registers.flags.z = (v == 0);
    }

    void SM83::m_sra(uint8_t& v)
    {
        bool high_bit_was_set = (v & 0x80) > 0;
        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c = (v & 0x01) > 0;

        v >>= 1;
        if (high_bit_was_set)
        {
            set_bit(v, 7);
        }

        m_registers.flags.z = ((int8_t)v == 0);
    }

    void SM83::m_swap(uint8_t& v)
    {
        auto right = (v & 0x0F) << 4;
        auto left = (v & 0xF0) >> 4;

        v = (left | right);

        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c = false;
        m_registers.flags.z = (v == 0);
    }

    void SM83::m_srl(uint8_t& v)
    {
        m_registers.flags.h = false;
        m_registers.flags.n = false;
        m_registers.flags.c = (v & 0x01) > 0;
        v >>= 1;
        m_registers.flags.z = (v == 0);
    }

    void SM83::m_rla()
    {
        m_rl(m_registers.A);
        m_registers.flags.z = false;
        m_registers.flags.h = false;
        m_registers.flags.n = false;
    }

    void SM83::m_rlca()
    {
        m_rlc(m_registers.A);
        m_registers.flags.z = false;
        m_registers.flags.h = false;
        m_registers.flags.n = false;
    }

    void SM83::m_rra()
    {
        m_rr(m_registers.A);
        m_registers.flags.z = false;
        m_registers.flags.h = false;
        m_registers.flags.n = false;
    }

    void SM83::m_rrca()
    {
        m_rrc(m_registers.A);
        m_registers.flags.z = false;
        m_registers.flags.h = false;
        m_registers.flags.n = false;
    }

    void SM83::m_bit(uint8_t v, uint8_t i)
    {
        m_registers.flags.z = !is_set_bit(v, i);
        m_registers.flags.n = false;
        m_registers.flags.h = true;
    }

    void SM83::m_res(uint8_t& v, uint8_t i)
    {
        clear_bit(v, i);
    }

    void SM83::m_set(uint8_t& v, uint8_t i)
    {
        set_bit(v, i);
    }

    void SM83::m_pop(uint16_t& rp)
    {
        uint8_t n = m_read(m_registers.SP++);
        rp = combine(n, m_read(m_registers.SP++));
    }

    void SM83::m_push(uint16_t rp)
    {
        m_registers.SP -= 2;
        // pre-dec has to be done in its own cycle
        m_advance_cycle();
        m_write(m_registers.SP, LSB(rp));
        m_write(m_registers.SP + 1, MSB(rp));
    }

    std::string SM83::dump()
    {
        if (m_registers.PC < 0x100)
        {
            return "";
        }

        std::string res =
            fmt::format("A: {:02X} F: {:02X} B: {:02X} C: {:02X} D: {:02X} E: {:02X} H: {:02X} L: {:02X} SP: {:04X} PC: 00:{:04X}",
                m_registers.A, m_registers.F, m_registers.B, m_registers.C, m_registers.D, m_registers.E,
                m_registers.H, m_registers.L, m_registers.SP, m_registers.PC);

        for (int i = 0; i < 4; ++i)
        {
            if (i == 0)
            {
                res += fmt::format(" (");
            }
            else
            {
                res += fmt::format(" ");
            }
            res += fmt::format("{:02X}", m_read(m_registers.PC + i));
        }

        res += fmt::format(")\n");

        return res;
    }

    std::string SM83::print_dis(OP op)
    {
        switch (op.name)
        {
        default:
            break;
        }
        return "";
        // return fmt::format("{} {},{} \n", op.name, op.args[0], op.args[1]);
    }

    void SM83::m_isr()
    {
        for (int i = 0; i < 5; ++i)
        {
            if (m_ime && is_set_bit(m_IE, i) && is_set_bit(m_IF, i))
            {
                m_ime = false;
                m_advance_cycle(2);
                clear_bit(m_IF, i);
                m_call(0xFF00 + (0x40 + i * 8));
            }
        }

        if (m_ime_new_value != m_ime)
        {
            m_ime = m_ime_new_value;
        }
    }

    void SM83::m_stop()
    {

    }

    void SM83::m_halt()
    {
        if (m_ime == false && ((m_IF & m_IE) != 0))
        {
            // halt bug
        }
        else
        {
            m_halted = true;
        }
    }

    void SM83::m_advance_cycle(int x)
    {
        for (int i = 0; i < x; ++i)
        {
            m_timer->advance_cycle();
        }
    }
}