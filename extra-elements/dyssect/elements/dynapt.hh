#ifndef CLICK_DYNAPT_HH_
#define CLICK_DYNAPT_HH_

#include <stdint.h>
#include <click/batchelement.hh>

#include "dyssectcontroller.hh"

CLICK_DECLS

struct NAPTState {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t src_addr;
    uint32_t dst_addr;
    uint32_t count;
};

class DyNAPT : public BatchElement {
    private:
        uint32_t next;
        uint32_t handle;
        std::vector<uint32_t> dst_addrs_;
        std::vector<std::vector<bool>> port_lists_;
        static const uint16_t kPortRange = (1 << 16) - 1;

        std::tuple<bool, uint32_t, uint16_t> GetEndpoint(uint32_t);
        void update_state(NAPTState *state, Packet *p);
        NAPTState* new_flow(Packet*);

    public:
        DyNAPT() { };
        ~DyNAPT() { };

        const char *class_name() const { return "DyNAPT"; }
        const char *port_count() const { return "1/1"; }
        const char *processing() const { return PUSH; }

        int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
        int initialize(ErrorHandler *errh);

        Packet *simple_action(Packet *pkt);
        void push_batch(int, PacketBatch *);
};

CLICK_ENDDECLS;

#endif // CLICK_DYNAPT_HH_
