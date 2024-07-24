#include <rte_lcore.h>
#include <click/config.h>
#include <click/glue.hh>
#include <click/args.hh>
#include "dynapt.hh"

CLICK_DECLS

std::vector<String> ext_addrs_ = {
    "10.0.0.1", "10.0.0.10", "10.0.0.100",
    "1.0.0.1", "1.0.0.10", "1.0.0.100",
    "10.0.1.1", "10.0.10.10", "10.0.100.100",
    "1.0.1.1", "1.0.10.10", "1.0.100.100",
    "10.1.1.1", "10.10.10.10", "10.100.100.100",
    "1.1.1.1", "1.10.10.10", "1.100.100.100",

    "172.16.0.1", "172.16.0.10", "172.16.0.100",
    "17.16.0.1", "17.16.0.10", "17.16.0.100",
    "172.16.1.1", "172.16.10.10", "172.16.100.100",
    "17.16.1.1", "17.16.10.10", "17.16.100.100",
    "172.160.0.1", "172.160.0.10", "172.160.0.100",
    "17.160.0.1", "17.160.0.10", "17.160.0.100",

    "192.0.10.1", "192.10.10.10", "192.100.10.100",
    "19.0.10.1", "19.10.10.10", "19.100.10.100",
    "192.168.10.1", "192.168.10.10", "192.168.10.100",
    "19.168.10.1", "19.168.10.10", "19.168.10.100",
    "192.168.100.1", "192.168.100.10", "192.168.100.100",
    "19.168.100.1", "19.168.100.10", "19.168.100.100",
};


int DyNAPT::initialize(ErrorHandler *errh) {
    return 0;
}

int DyNAPT::configure(Vector<String> &conf, ErrorHandler *errh) {
    if(Args(conf, this, errh)
        .read("HANDLE", handle)
        .complete() < 0)
        return -1;

    for(const String &e : ext_addrs_) {
        IPAddress ip(e);

        dst_addrs_.push_back(ip.addr());
        std::vector<bool> ports;
        for(uint32_t j = 0; j < kPortRange; j++)
            ports.push_back(true);
        port_lists_.push_back(ports);
    }

    return 0;
}

std::tuple<bool, uint32_t, uint16_t> DyNAPT::GetEndpoint(uint32_t idx) {
    uint16_t start_port = 1 + click_random(0, kPortRange);

    uint16_t port;
    for(uint32_t i = 0; i < kPortRange; i++) {
        port = (start_port + i) % kPortRange;
        if(port_lists_[idx][port]) {
            port_lists_[idx][port] = false;

            return std::make_tuple(true, dst_addrs_[idx], port);
        }
    }

    return std::make_tuple(false, 0, 0);
}

NAPTState* DyNAPT::new_flow(Packet* p) {
    NAPTState* state = (NAPTState*) malloc(sizeof(NAPTState));
/*
    bool valid;
    uint32_t addr;
    uint16_t port;
    uint32_t idx;

    for(uint32_t trials = 0; trials < dst_addrs_.size(); trials++) {
        idx = (next++ % dst_addrs_.size());

        std::tie(valid, addr, port) = GetEndpoint(idx);

        if(valid)
            break;
    }

    if(!valid)
        return 0;
*/
//    state->src_port = port;
//    state->dst_port = p->tcp_header()->th_dport;
//    state->src_addr = p->ip_header()->ip_src.s_addr;;
//    state->dst_addr = p->ip_header()->ip_dst.s_addr;
    state->count = 0;

    return state;
}

inline void DyNAPT::update_state(NAPTState *state, Packet *p) {
    state->count++;
/*
    uint16_t port = state->src_port;
    IPAddress ip(state->src_addr);
    WritablePacket* q = p->uniqueify();

    uint16_t *xip = reinterpret_cast<uint16_t *>(&q->ip_header()->ip_src);
    uint32_t old_hw = (uint32_t) xip[0] + xip[1];
    uint32_t t_old_hw = old_hw;
    old_hw += (old_hw >> 16);

    memcpy(&xip[0], &ip, 4);

    uint32_t new_hw = (uint32_t) xip[0] + xip[1];
    uint32_t t_new_hw = new_hw;
    new_hw += (new_hw >> 16);
    click_ip *iph = q->ip_header();
    click_update_in_cksum(&iph->ip_sum, old_hw, new_hw);

    uint16_t *xport = reinterpret_cast<uint16_t *>(&q->tcp_header()->th_sport);
    t_old_hw += (uint32_t) xport[0];
    t_old_hw += (t_old_hw >> 16);
    xport[0] = port;
    t_new_hw += (uint32_t) xport[0];
    t_new_hw += (t_new_hw >> 16);
    click_update_in_cksum(&q->tcp_header()->th_sum, t_old_hw, t_new_hw);
*/
}

inline Packet *DyNAPT::simple_action(Packet *p) {
    void** global_state = (void**) PERFCTR_ANNO(p);
    NAPTState* state = (NAPTState*) global_state[handle];

    if(!state)
    {
        state = new_flow(p);
        global_state[handle] = state;
    }

    update_state(state, p);

    return p;
}

void DyNAPT::push_batch(int, PacketBatch *batch)
{
    Packet *p = batch->first();
    Packet *next;

    while(p)
    {
        next = p->next();

        p = simple_action(p);

        p = next;
    }

    output_push_batch(0, batch);
}

CLICK_ENDDECLS

EXPORT_ELEMENT(DyNAPT)
