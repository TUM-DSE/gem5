/*
 * Copyright (c) 2024 Purdue,UCSD I guess
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "cpu/o3/userinterrupt.hh"
#include "arch/x86/regs/misc.hh"
#include "arch/x86/interrupts.hh"
#include "arch/x86/pcstate.hh"
#include "cpu/o3/cpu.hh"
#include "debug/UserInterrupt.hh"
#include "userinterrupt.hh"

namespace gem5
{

    namespace o3
    {
        bool UserInterruptProcessor::checkFlags() const
        {
            X86ISA::RFLAGS rflags = cpu->readMiscRegNoEffect(X86ISA::misc_reg::Rflags, 0);
            X86ISA::UintrMisc uintrMisc = cpu->readMiscRegNoEffect(X86ISA::misc_reg::UintrMisc, 0);
            return rflags.intf && uintrMisc.uif;
        }

        bool UserInterruptProcessor::checkIfOngoing() const
        {
            return ongoing;
        }

        void UserInterruptProcessor::setInterrupt(Fault userInt, uint64_t vector, Addr pc)
        {
            DPRINTF(UserInterrupt, "[userinterrupt] Setting UINTR\n");
            cpu->userIntAtLeastOnce = true;
            cpu->userInterruptInfoUpdate(userInt);
            arrival++;
            // reinterpret_cast<X86ISA::Interrupts *>(cpu->getInterruptController(0))->setReg(X86ISA::APIC_EOI, 1, 0);

            this->returnPC = pc;
            this->vector = vector;
            this->userInt = userInt;
            issue = waitingForBoundary = false;
        }

        void UserInterruptProcessor::setInterrupt(Addr pc)
        {
            DPRINTF(UserInterrupt, "[userinterrupt] User Interrupt updates only pc, RIP:%#x\n", pc);
            cpu->userIntAtLeastOnce = true;

            returnPC = pc;
        }

        void UserInterruptProcessor::resetInterrupt()
        {
            DPRINTF(UserInterrupt, "[userinterrupt] User Interrupt can retire in peace; vector: %d, RIP:%#x\n", vector, returnPC);
            cpu->setMiscRegNoEffect(X86ISA::misc_reg::UintrOngoing, 0, 0);
            cpu->userIntAtLeastOnce = true;
            retire++;
            assert(retire == arrival);
            issue = false;
            ongoing = false;
            userInt = NoFault;
            pcUsable = false;
        }
        std::unique_ptr<PCStateBase> UserInterruptProcessor::processInterrupt()
        {
            DPRINTF(UserInterrupt, "[userinterrupt] Start processing User Interrupt\n");
            cpu->userIntAtLeastOnce = true;

            std::unique_ptr<PCStateBase> oldpc(cpu->pcState(0).clone());
            // std::unique_ptr<PCStateBase> tempPC(cpu->pcState(0).clone());
            bool squashSituation = cpu->thread[0]->noSquashFromTC;
            cpu->thread[0]->noSquashFromTC = true;
            cpu->setMiscRegNoEffect(X86ISA::misc_reg::UintrOngoing, 1, 0);
            cpu->setMiscRegNoEffect(X86ISA::misc_reg::UintrVec, vector, 0);
            cpu->setMiscRegNoEffect(X86ISA::misc_reg::UintrPC, returnPC, 0);

            // tempPC->as<X86ISA::PCState>().pc(returnPC);
            // cpu->pcState(*tempPC, 0);

            DPRINTF(UserInterrupt, "[userinterrupt] User Interrupt fetch starts with vector: %d, RIP:%#x\n", vector, returnPC);
            // cpu->processInterrupts(userInt);
            // reinterpret_cast<X86ISA::Interrupts *>(cpu->getInterruptController(0))->setReg(X86ISA::APIC_EOI, 1, 0);
            DPRINTF(UserInterrupt, "[userinterrupt] userInt dynamic type: %s\n", typeid(userInt).name());

            //cpu->setMiscReg(X86ISA::misc_reg::UintrTemp, 0, 0);
            //cpu->setMiscReg(X86ISA::misc_reg::UintrScratch, 0, 0);
            /*for (int i = 0; i < cpu->numThreads; ++i) {
                auto* tc = cpu->getContext(i);
                tc->setMiscRegNoEffect(X86ISA::misc_reg::UintrTemp, 0);
                tc->setMiscRegNoEffect(X86ISA::misc_reg::UintrScratch, 0);
                DPRINTF(UserInterrupt, "[userinterrupts] Reset MSRs on core %d\n", cpu->cpuId());
            }*/

            cpu->trap(userInt, 0, nullptr);

            /*for (int i = 0; i < cpu->numThreads; ++i) {
                auto* tc = cpu->getContext(i);
                uint64_t tmp = tc->readMiscRegNoEffect(X86ISA::misc_reg::UintrTemp);
                uint64_t scrtch = tc->readMiscRegNoEffect(X86ISA::misc_reg::UintrScratch);
                DPRINTF(UserInterrupt, "[userinterrupt] [tid %d, core %d] UintrTemp: %#016llx, UintrScratch: %#016llx\n", i, cpu->cpuId(), tmp, scrtch);
                DPRINTF(UserInterrupt, "[userinterrupt] Uintr Frame: PC=0x%016lx, RFLAGS=0x%016lx, RSP=0x%016lx\n",
                    tc->readMiscReg(X86ISA::misc_reg::UintrPciPC),
                    tc->readMiscReg(X86ISA::misc_reg::UintrPciRFLAGS),
                    tc->readMiscReg(X86ISA::misc_reg::UintrPciRSP));
            }            
            */
            cpu->thread[0]->noSquashFromTC = squashSituation;
            issue = false;
            ongoing = true;
            return oldpc;
        }

        void UserInterruptProcessor::setWait()
        {
            cpu->userIntAtLeastOnce = true;

            ongoing = false;
            waitingForBoundary = true;
        }

        void UserInterruptProcessor::resetWait()
        {
            waitingForBoundary = false;
        }
        void UserInterruptProcessor::freeze()
        {
            DPRINTF(UserInterrupt, "[userinterrupt] Freezing user interrupt, there is another interrupt and we have not committed.\n");
            // we need to be waiting for boundary if we have already sent the interrupt we cannot
            // freeze.

            cpu->setMiscRegNoEffect(X86ISA::misc_reg::UintrOngoing, 0, 0);
            frozen = true;
            if (checkInterrupt())
            {
                setWait();
            }
            // there is still one case what if we committed something from rom and another interrupt comes,
            //  wouldn't happen with others but we basically consume a user interrupt right away.
        }

        void UserInterruptProcessor::unFreeze()
        {
            DPRINTF(UserInterrupt, "[userinterrupt] Unfreezing user interrupt, waiting for next instruction.\n");
            // an unfreeze cannot happen for an ongoing interrupt that is big boo-boo

            frozen = false;
            if (userInt != NoFault)
                waitingForBoundary = true;
        }

        UserInterruptProcessor::UserInterruptProcessor(CPU *cpu)
        {
            this->cpu = cpu;
            userInt = NoFault;
        }
    } // namespace o3
} // namespace gem5
