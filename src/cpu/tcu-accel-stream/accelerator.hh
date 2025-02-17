/*
 * Copyright (C) 2015-2018 Nils Asmussen <nils@os.inf.tu-dresden.de>
 * Copyright (C) 2019-2021 Nils Asmussen, Barkhausen Institut
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are
 * those of the authors and should not be interpreted as representing official
 * policies, either expressed or implied, of the FreeBSD Project.
 */

#ifndef __CPU_TCU_ACCEL_STREAM_ACCELERATOR_HH__
#define __CPU_TCU_ACCEL_STREAM_ACCELERATOR_HH__

#include "params/TcuAccelStream.hh"
#include "cpu/tcu-accel-stream/logic.hh"
#include "cpu/tcu-accel/accelerator.hh"
#include "cpu/tcu-accel/ctxswsm.hh"
#include "cpu/tcu-accel/syscallsm.hh"
#include "cpu/tcu-accel/yieldsm.hh"
#include "mem/tcu/connector/base.hh"
#include "mem/tcu/reg_file.hh"
#include "sim/system.hh"

class TcuAccelStream : public TcuAccel
{
  public:

    static const Addr MSG_ADDR          = 0x2000;
    static const Addr BUF_ADDR          = 0x8000;
    static const Addr RBUF_ADDR         = 0x3FFF00;

    static const unsigned EP_IN_SEND    = 16;
    static const unsigned EP_IN_MEM     = 17;
    static const unsigned EP_OUT_SEND   = 18;
    static const unsigned EP_OUT_MEM    = 19;
    static const unsigned EP_RECV       = 20;

    static const uint64_t LBL_IN_REQ    = 1;
    static const uint64_t LBL_IN_REPLY  = 2;
    static const uint64_t LBL_OUT_REQ   = 3;
    static const uint64_t LBL_OUT_REPLY = 4;

    static const size_t MSG_SIZE        = 64;

    static const uint64_t NO_COMMIT     = 0xFFFFFFFFFFFFFFFF;

  public:
    TcuAccelStream(const TcuAccelStreamParams &p);

    void wakeup() override;

    void interrupt() override;

    void reset() override;

    void logicFinished();

    Addr rbufAddr() const { return RBUF_ADDR + offset; }
    Addr sendMsgAddr() const override { return MSG_ADDR + offset; }
    Addr bufferAddr() const override { return BUF_ADDR + offset; }
    void setSwitched() override { ctx.flags |= Flags::STARTED; }

  private:

    /// main simulation loop
    void tick() override;

    void completeRequest(PacketPtr pkt) override;

    enum class State
    {
        IDLE,

        FETCH_MSG,
        FETCH_MSG_WAIT,
        READ_MSG_ADDR,
        READ_MSG,

        INOUT_START,
        INOUT_SEND,
        INOUT_SEND_WAIT,
        INOUT_ACK,
        INOUT_ACK_WAIT,

        READ_DATA,
        READ_DATA_WAIT,

        WRITE_DATA,
        WRITE_DATA_WAIT,

        REPLY_STORE,
        REPLY_SEND,
        REPLY_WAIT,

        CTXSW,

        SYSCALL,

        COMMIT_START,
        COMMIT_SEND,
        COMMIT_SEND_WAIT,

        EXIT_ACK,
        EXIT_ACK_WAIT,
        EXIT,
    };

    enum class Command
    {
        STAT,
        SEEK,
        NEXT_IN,
        NEXT_OUT,
        COMMIT,
    };

    std::string getStateName() const;

    bool irqPending;
    bool memPending;

    State state;
    State lastState;
    uint16_t lastFlags;

    enum Flags
    {
        OUTPUT      = 0x1,
        WAIT        = 0x2,
        SEEN_EOF    = 0x4,
        SEEN_COMMIT = 0x8,
        EXIT        = 0x10,
        COMP        = 0x20,
        COMPDONE    = 0x40,
        FETCHED     = 0x80,
        TRANSFER    = 0x100,
        INSYSC      = 0x200,
        STARTED     = 0x400,
    };

    struct M5_ATTR_PACKED
    {
        uint16_t bufOff;
        uint16_t flags;
        uint32_t inAvail : 1,
                 outAvail : 1,
                 : 30;
        uint64_t compTime;
        uint64_t msgAddr;
        uint64_t inReqAddr;
        uint64_t outReqAddr;
        uint64_t commitOff;
        uint64_t commitLen;
        uint64_t inOff;
        uint64_t inPos;
        uint64_t inLen;
        uint64_t outOff;
        uint64_t outPos;
        uint64_t outLen;
        uint64_t lastSize;
        uint64_t nextSysc;
        // padding
        uint64_t : 64;
    } ctx;

    struct M5_ATTR_PACKED
    {
        uint64_t cmd;
        uint64_t commit;
    } rdwr_msg;

    struct M5_ATTR_PACKED
    {
        uint64_t opcode;
        uint64_t vpe_sel;
        uint64_t op;
        uint64_t arg;
    } exit_msg;

    struct M5_ATTR_PACKED
    {
        uint64_t err;
        uint64_t off;
        uint64_t len;
    } reply;

    size_t bufSize;
    SyscallSM sysc;
    State syscNext;
    Addr replyAddr;
    State replyNext;
    YieldSM yield;
    AccelLogic *logic;
    AccelCtxSwSM ctxsw;
    bool ctxSwPerformed;
};

#endif // __CPU_TCU_ACCEL_STREAM_ACCELERATOR_HH__
