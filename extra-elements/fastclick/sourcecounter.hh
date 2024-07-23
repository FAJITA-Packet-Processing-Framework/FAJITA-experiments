#ifndef CLICK_SOURCECOUNTER_HH
#define CLICK_SOURCECOUNTER_HH
#include <click/batchelement.hh>
#include <click/vector.hh>
#include <click/multithread.hh>
#include <click/atomic.hh>
#include <rte_hash.h>

CLICK_DECLS


/*
=c

FlowCounter([CLOSECONNECTION])

=s flow

Counts all flows passing by, the number of active flows, and the number of 
packets per flow.

 */


class SourceCounter : public BatchElement
{
public:
    /** @brief Construct an FlowCounter element
     */
    SourceCounter() CLICK_COLD;

    const char *class_name() const override        { return "SourceCounter"; }
    const char *port_count() const override        { return PORTS_1_1; }
    const char *processing() const override        { return PUSH; }
    void add_handlers() override CLICK_COLD;

    int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;

    Packet *simple_action(Packet *);
    #if HAVE_BATCH
        PacketBatch *simple_action_batch(PacketBatch *);
    #endif

protected:

    struct local_flowID {
        uint32_t ip_src;
    };

    static String read_handler(Element *, void *) CLICK_COLD;
    static int write_handler(const String &, Element *, void *, ErrorHandler *) CLICK_COLD;

    uint32_t _capacity;
    uint8_t _is_src;
    uint8_t _cache;
    uint32_t _offset;
    atomic_uint32_t *_local_fcbs;
    atomic_uint32_t _insertions;

    void *_table;
};

CLICK_ENDDECLS
#endif
