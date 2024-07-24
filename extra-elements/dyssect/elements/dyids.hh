#ifndef CLICK_DYIDS_HH_
#define CLICK_DYIDS_HH_

#include <stdint.h>
#include <click/batchelement.hh>

#include "dyssectcontroller.hh"

CLICK_DECLS

#define DYIDS_ENTRIES 1024

struct IDSState {
    uint32_t found;
    uint32_t not_found;
    uint32_t *matched;
    uint32_t *unmatched;
};

class DyIDS : public BatchElement {
    private:
        uint32_t handle;
        uint32_t offset;

    public:
        DyIDS() { in_batch_mode = BATCH_MODE_YES; };
        ~DyIDS() { };

        const char *class_name() const { return "DyIDS"; }
        const char *port_count() const { return "1/1"; }
        const char *processing() const { return PUSH; }

        int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;
        int initialize(ErrorHandler *errh);

        int lex(const uint8_t *);
        void UpdateState(Packet *p, IDSState *state);
        Packet *simple_action(Packet *pkt);
        void push_batch(int, PacketBatch *);
};

CLICK_ENDDECLS;

#endif // CLICK_DYIDS_HH_