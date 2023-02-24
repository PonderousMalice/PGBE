#pragma once
#include "MMU.h"
#include "Timer.h"
#include <variant>

using std::placeholders::_1;

namespace emulator
{
    using reg_t = std::variant<u8*, u16*>;
    using reg_v = std::variant<u8, u16>;

    enum reg_name
    {
        None,
        A,
        AF,
        B,
        BC,
        C,
        D,
        DE,
        E,
        H,
        L,
        HL,
        HL_inc,
        HL_dec,
        SP,
        SP_d,
        n,
        nn,
    };

    struct reg_s
    {
        reg_name name;
        bool indirect;
    };

    struct OP
    {
        enum
        {
            INVALID,
            ALU,
            ADD_16,
            BIT,
            CALL,
            CALL_CC,
            CCF,
            CPL,
            DAA,
            DEC,
            DEC_16,
            DI,
            EI,
            HALT,
            INC,
            INC_16,
            JP,
            JP_CC,
            JP_HL,
            JR,
            JR_CC,
            LD,
            NOP,
            POP,
            PUSH,
            RES,
            RET,
            RET_CC,
            RETI,
            RLA,
            RLCA,
            ROT,
            RRA,
            RRCA,
            RST,
            SCF,
            SET,
            STOP,
        } name;
        std::array<reg_s, 2> args;

        u8 y;
    };

    class SM83
    {
    public:
        SM83(MMU* mmu, Timer* t);

        void run();
        std::string dump();
        std::string print_dis(OP op);
    private:
        MMU* m_mmu;
        Timer* m_timer;
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
                            u8 : 4; // padding
                            u8 c : 1;
                            u8 h : 1;
                            u8 n : 1;
                            u8 z : 1;
                        } flags;
                        u8 F;
                    };
                    u8 A;
                };
                u16 AF;
            };

            union
            {
                struct
                {
                    u8 C;
                    u8 B;
                };
                u16 BC;
            };

            union
            {
                struct
                {
                    u8 E;
                    u8 D;
                };
                u16 DE;
            };

            union
            {
                struct
                {
                    u8 L;
                    u8 H;
                };
                u16 HL;
            };

            u16 SP;
            u16 PC;
        } m_registers;
        bool m_ime, m_halted, m_halt_bug;
        u8& m_IF, & m_IE;

        OP m_prev_op;

        OP m_decode(u8 opcode);
        void m_execute(OP instr);
        void m_advance_cycle(int m_cycles = 1);

        // Memory access
        u8 m_read(u16 adr);
        u8 m_fetch();
        u16 m_fetch_word();
        void m_write(u16 adr, u8 v);

        reg_v m_get_reg(reg_s r);
        void m_set_reg(reg_s r, reg_v v);

        // Interrupt handler
        void m_isr();

        // ALU
        void m_add(u8 v);
        void m_add_16(reg_s lv, reg_s rv);
        void m_adc(u8 v);
        void m_sub(u8 v);
        void m_sbc(u8 v);
        void m_and(u8 v);
        void m_xor(u8 v);
        void m_or(u8 v);
        void m_cp(u8 v);

        void m_dec(reg_t v);
        void m_inc(reg_t v);

        void m_cpl();
        void m_scf();
        void m_ccf();
        void m_daa();

        void m_ld(reg_s lv, reg_s rv);

        void m_pop(u16& r);
        void m_push(u16 r);

        // Rotate and shi(f)t instructions
        void m_rlc(u8& v);
        void m_rrc(u8& v);
        void m_rl(u8& v);
        void m_rr(u8& v);
        void m_sla(u8& v);
        void m_sra(u8& v);
        void m_swap(u8& v);
        void m_srl(u8& v);
        void m_rla();
        void m_rlca();
        void m_rra();
        void m_rrca();

        // Conditions
        bool m_nz();
        bool m_z();
        bool m_nc();
        bool m_c();

        // Jump
        void m_call(u16 adr, std::function<bool(void)> cc = nullptr);
        void m_jp(std::function<bool(void)> cc = nullptr);
        void m_jr(std::function<bool(void)> cc = nullptr);
        void m_ret(std::function<bool(void)> cc = nullptr);
        void m_reti();

        // Bit op
        void m_bit(u8 v, u8 i);
        void m_res(u8& v, u8 i);
        void m_set(u8& v, u8 i);

        // Special
        void m_stop();
        void m_halt();

        const std::array<reg_s, 8> m_r
        {
            reg_s{ B, false },
            reg_s{ C, false },
            reg_s{ D, false },
            reg_s{ E, false },
            reg_s{ H, false },
            reg_s{ L, false },
            reg_s{ HL, true },
            reg_s{ A, false },
        };

        const std::array<reg_s, 4> m_rp
        {
            reg_s{ BC, false },
            reg_s{ DE, false },
            reg_s{ HL, false },
            reg_s{ SP, false },
        };

        const std::array<reg_s, 4> m_rp2
        {
            reg_s{ BC, false },
            reg_s{ DE, false },
            reg_s{ HL, false },
            reg_s{ AF, false },
        };

        const std::array<std::function<void(u8)>, 8> m_alu
        {
            std::bind(&SM83::m_add, this, _1),
            std::bind(&SM83::m_adc, this, _1),
            std::bind(&SM83::m_sub, this, _1),
            std::bind(&SM83::m_sbc, this, _1),
            std::bind(&SM83::m_and, this, _1),
            std::bind(&SM83::m_xor, this, _1),
            std::bind(&SM83::m_or, this, _1),
            std::bind(&SM83::m_cp, this, _1),
        };

        const std::array<std::function<void(u8&)>, 8> m_rot
        {
            std::bind(&SM83::m_rlc, this, _1),
            std::bind(&SM83::m_rrc, this, _1),
            std::bind(&SM83::m_rl, this, _1),
            std::bind(&SM83::m_rr, this, _1),
            std::bind(&SM83::m_sla, this, _1),
            std::bind(&SM83::m_sra, this, _1),
            std::bind(&SM83::m_swap, this, _1),
            std::bind(&SM83::m_srl, this, _1),
        };

        const std::array<std::function<bool(void)>, 4> m_cc
        {
            std::bind(&SM83::m_nz, this),
            std::bind(&SM83::m_z, this),
            std::bind(&SM83::m_nc, this),
            std::bind(&SM83::m_c, this),
        };
    };
}