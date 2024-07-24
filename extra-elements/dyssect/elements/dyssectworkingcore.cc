#include <click/config.h>
#include "dyssectworkingcore.hh"
#include <click/args.hh>
#include <click/router.hh>
#include <click/standard/scheduleinfo.hh>
#include <click/straccum.hh>
#include <clicknet/ip.h>
#include <clicknet/tcp.h>
#include <clicknet/ether.h>

CLICK_DECLS

inline bool is_priority(const click_ip *ip)
{
    return ip->ip_id == htons(1);
}

inline bool is_less_than(double a, double b)
{
    return (b - a) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * EPSILON );
}

DyssectWorkingCore::DyssectWorkingCore()
    : _task(this)
{
    in_batch_mode = BATCH_MODE_YES;
}

DyssectWorkingCore::~DyssectWorkingCore()
{
}

int DyssectWorkingCore::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if(Args(conf, this, errh)
        .read("PORT", portid)
        .read("QUEUE", queueid)
        .read_or_set("BURST", _burst, 32)
        .read_or_set("SOLVER", _solver, 1)
        .complete() < 0)
        return -1;
    
    off_idx = 0;
    qsize = 256;
    myown = false;
    q = DyssectController::queues[queueid];
    myring = DyssectController::rings[queueid];
    offloading_idx = rte_atomic32_read(DyssectController::offloading_from_working[queueid]);
    
    return 0;
}

int DyssectWorkingCore::initialize(ErrorHandler *errh)
{
    ScheduleInfo::initialize_task(this, &_task, true, errh);
    return 0;
}


inline DyssectState* DyssectWorkingCore::extract_state(Packet *p) 
{
    const click_ip *ip = reinterpret_cast<const click_ip *>(p->data() + sizeof(click_ether));
    const click_tcp *tcp = reinterpret_cast<const click_tcp *>(p->data() + sizeof(click_ether) + ip->ip_hl*4);

    DyssectFlow f = {
        .src_port = tcp->th_sport,
        .dst_port = tcp->th_dport,
        .src_addr = IPAddress(ip->ip_src),
        .dst_addr = IPAddress(ip->ip_dst),
        .hash_value = AGGREGATE_ANNO(p)
    };

    if(unlikely(DyssectController::iqueues == 1)) {
        f.hash_value = rte_hash_crc(&f, FLOW_SIZE, 0);
    }

    DyssectState *state = 0;
    bool priority = is_priority(ip);
//    bool priority = true;
    uint32_t s = f.hash_value % DyssectController::total_shards;

//    auto *dpdktable = 	reinterpret_cast<rte_hash*>(DyssectController::shards[s]._table);
//    void* data = nullptr;
//    int ret = rte_hash_lookup_data(dpdktable,&f, &data);
    auto item = DyssectController::shards[s].table->find(f);
    if(item != DyssectController::shards[s].table->end())
//    if (ret >= 0)
    {
        state = item->second;
//        state = reinterpret_cast<DyssectState*> (data);

        if(!priority && _solver)
        {
            uint32_t local_epoch = rte_atomic32_read(DyssectController::epoch);

            if(state->epoch != local_epoch)
            {
                state->prob = 0;
                if(DyssectController::shards[s].old_bytes && state->epoch == local_epoch-1)
                {
                    state->prob = ((double) state->bytes)/DyssectController::shards[s].old_bytes;
                }
                state->bytes = 0;
                state->epoch = local_epoch;
            }

            if(DyssectController::shards[s].use_2)
            {
                rte_atomic32_inc(&DyssectController::shards[s].ref_count_2);
                DyssectController::shards[s].flows3_2->operator[](state->flow) = state;
                rte_atomic32_dec(&DyssectController::shards[s].ref_count_2);
            } else
            {
                rte_atomic32_inc(&DyssectController::shards[s].ref_count_1);
                DyssectController::shards[s].flows3->operator[](state->flow) = state;
                rte_atomic32_dec(&DyssectController::shards[s].ref_count_1);
            }
        }

    } else {
        state = (DyssectState*) rte_zmalloc(NULL, sizeof(DyssectState), 64);
        void **global_state = (void**) rte_zmalloc(NULL, DyssectController::sfc_length * sizeof(void*), 64);

        state->global_state = global_state;
//        rte_hash_add_key_data(dpdktable, &f, state);
        DyssectController::shards[s].table->operator[](f) = state;
        state->prob = 0;
        state->shard = s;
        state->priority = priority;
        if (_solver)
            state->epoch = rte_atomic32_read(DyssectController::epoch);

        rte_memcpy(&(state->flow), &f, sizeof(DyssectFlow));
    }

    uint32_t iplen = ntohs(ip->ip_len);

    state->bytes += iplen;
    if (_solver){
        rte_atomic32_add(&DyssectController::shards[s].bytes, iplen);
        rte_atomic32_inc(&DyssectController::shards[s].packets);
    }

    p->set_network_header((const unsigned char*) ip);
    p->set_transport_header((const unsigned char*) tcp);
    SET_PERFCTR_ANNO(p, (uint64_t)(state->global_state));

    return state;
}

inline int DyssectWorkingCore::run(Packet *p)
{
    DyssectState *state = extract_state(p);
    if(state->priority)
    {
        return 0;
    }

    uint32_t s = state->shard;
    if(is_less_than(state->cdf, DyssectController::shards[s].r))
    {
        off_pkts[off_idx++] = p->mb();
        return -1;
    }
    
    return 0;
}

inline PacketBatch *DyssectWorkingCore::process()
{
    struct rte_mbuf *pkts[_burst];

    if(unlikely(rte_atomic32_read(DyssectController::had_changes[queueid]) == 1))
    {
        for(uint32_t s = 0; s < DyssectController::total_shards; s++)
        {
            if(DyssectController::shards[s].owner_new == queueid) 
            {
                if(rte_atomic32_read(&DyssectController::shards[s].pause) == 0)
                {
                    myown = false;
                    while(rte_ring_count(&DyssectController::shards[s].local_queue) != 0) 
                    {
                        uint32_t cnt = rte_ring_dequeue_burst(&DyssectController::shards[s].local_queue, (void**) pkts, _burst, 0);

                        rte_ring_enqueue_burst(myring, (void**) pkts, cnt, 0);
                    }
                }
            }
        }

        rte_atomic32_clear(DyssectController::had_changes[queueid]);
    }
    if(unlikely(myown))
    {
        return 0;
    }

    int cnt = rte_eth_rx_burst(portid, queueid, pkts, _burst);
    if(cnt == 0)
    {
        return 0;
    }

    PacketBatch *head = 0;
    WritablePacket *last;
    for(int i = 0; i < cnt; i++)
    {
        WritablePacket *p = static_cast<WritablePacket *>(Packet::make(pkts[i], false));
        p->set_packet_type_anno(Packet::HOST);
        SET_AGGREGATE_ANNO(p, pkts[i]->hash.rss);
        SET_FLOW_ID_ANNO(p, queueid);

        if (head == NULL)
            head = PacketBatch::start_head(p);
        else
            last->set_next(p);
        last = p;
    }

    head->make_tail(last, cnt);

    return head;
}

void DyssectWorkingCore::transfer_r()
{
    if(rte_ring_count(q) != 0)
    {
        rte_ring_enqueue(DyssectController::toRemoveQueue[offloading_idx], q);

        myown = true;
        rte_ring *aux = q;
        q = myring;
        DyssectController::queues[queueid] = myring;
        myring = aux;
        DyssectController::rings[queueid] = aux;

        rte_ring_enqueue(DyssectController::toAddQueue[offloading_idx], q);
    }

    for(uint32_t s = 0; s < DyssectController::total_shards; s++)
    {
        if(DyssectController::shards[s].owner == queueid)
        {
            DyssectController::shards[s].r = DyssectController::shards[s].r_new;
        }
    }
    
    rte_atomic32_clear(DyssectController::transfer_r[queueid]);
}

void DyssectWorkingCore::transfer_shard()
{
    struct rte_mbuf *pkts[_burst];
    if(offloading_idx != UINT32_MAX)
        rte_ring_enqueue(DyssectController::toRemoveQueue[offloading_idx], q);

    uint32_t count = MIN(rte_eth_rx_queue_count(portid, queueid), qsize);

    while(count > 0)
    {
        myown = true;

        int cnt = rte_eth_rx_burst(portid, queueid, pkts, _burst);
        if(cnt == 0)
        {
            break;
        }

        rte_ring_enqueue_burst(myring, (void**) pkts, cnt, 0);

        count -= cnt;
    }

    while(rte_ring_count(q) != 0)
    {
        myown = true;

        int cnt = rte_ring_dequeue_burst(q, (void**) pkts, _burst, NULL);
        rte_ring_enqueue_burst(myring, (void**) pkts, cnt, NULL);
    }

    if(offloading_idx != UINT32_MAX)
        rte_ring_enqueue(DyssectController::toAddQueue[offloading_idx], q);

    if(!myown)
    {
        for(uint32_t s = 0; s < DyssectController::total_shards; s++)
        {
            if(DyssectController::shards[s].owner == queueid && DyssectController::shards[s].owner_new != UINT32_MAX)
            {
                rte_atomic32_clear(&DyssectController::shards[s].pause);
                uint32_t owner_new = DyssectController::shards[s].owner_new;
                rte_atomic32_set(DyssectController::had_changes[owner_new], 1);
            }
        }
    }

    rte_atomic32_clear(DyssectController::transfer_shard[queueid]);
}

void DyssectWorkingCore::transfer_offloading()
{
    struct rte_mbuf *pkts[_burst];

    if(offloading_idx != UINT32_MAX)
        rte_ring_enqueue(DyssectController::toRemoveQueue[offloading_idx], q);

    while(rte_ring_count(q) != 0)
    {
        myown = true;

        int cnt = rte_ring_dequeue_burst(q, (void**) pkts, _burst, NULL);
        rte_ring_enqueue_burst(myring, (void**) pkts, cnt, NULL);
    }

    uint32_t new_offloading_idx = rte_atomic32_read(DyssectController::new_offloading_from_working[queueid]);
    if(new_offloading_idx != UINT32_MAX)
    {
        rte_ring_enqueue(DyssectController::toAddQueue[new_offloading_idx], q);
        offloading_idx = new_offloading_idx;
        rte_atomic32_set(DyssectController::new_offloading_from_working[queueid], UINT32_MAX);
    }

    rte_atomic32_clear(DyssectController::transfer_offloading[queueid]);
}

bool DyssectWorkingCore::run_task(Task *) {
    struct rte_mbuf *pkts[_burst];

    PacketBatch *head = 0;
    WritablePacket *last;

    if(unlikely(myown))
    {
        if(rte_ring_empty(myring))
        {
            myown = false;

            for(uint32_t s = 0; s < DyssectController::total_shards; s++)
            {
                if(rte_atomic32_read(&DyssectController::shards[s].pause) == 1 && DyssectController::shards[s].owner == queueid && DyssectController::shards[s].owner_new != UINT32_MAX)
                {
                    rte_atomic32_clear(&DyssectController::shards[s].pause);
                    uint32_t onwer_new = DyssectController::shards[s].owner_new;
                    rte_atomic32_set(DyssectController::had_changes[onwer_new], 1);
                }

                if(rte_atomic32_read(&DyssectController::shards[s].pause) == 0 && DyssectController::shards[s].owner_new == queueid)
                {
                    DyssectController::shards[s].owner_new = UINT32_MAX;
                    DyssectController::shards[s].owner = queueid;
                }
            }

            _task.fast_reschedule();
            return true;
        }

        uint32_t cnt = MIN(rte_ring_count(myring), _burst);
        if(cnt == 0)
        {
            _task.fast_reschedule();
            return false;
        }

        rte_ring_dequeue_burst(myring, (void**) pkts, cnt, NULL);

        for(uint32_t i = 0; i < cnt; i++) {
            WritablePacket *p = static_cast<WritablePacket *>(Packet::make(pkts[i], false));
            p->set_packet_type_anno(Packet::HOST);
            SET_AGGREGATE_ANNO(p, pkts[i]->hash.rss);
            SET_FLOW_ID_ANNO(p, queueid);

            extract_state(p);

            if (head == NULL)
                head = PacketBatch::start_head(p);
            else
                last->set_next(p);
            last = p;
        }

        if(head) 
        {
            head->make_tail(last, cnt);
            output_push_batch(0, head);
        }
        
        _task.fast_reschedule();
        
        if (head)
            return true;
        else
            return false;
    }

    if(unlikely(rte_atomic32_read(DyssectController::controller_signal[queueid]) == 1))
    {
        if(rte_atomic32_read(DyssectController::transfer_r[queueid]) == 1)
        {
            transfer_r();
        }
        if(rte_atomic32_read(DyssectController::transfer_offloading[queueid]) == 1)
        {
            transfer_offloading();   
        }
        if(rte_atomic32_read(DyssectController::transfer_shard[queueid]) == 1)
        {
            transfer_shard();
        }
        rte_atomic32_clear(DyssectController::controller_signal[queueid]);
    }

    PacketBatch *batch = process();
    if(batch) 
    {   
        uint64_t hitmast = 0;
        // batch_lookup(batch, &hitmast);
        CLASSIFY_EACH_PACKET_IGNORE(1, run, batch, output_push_batch);
        if(off_idx)
        {
            rte_ring_enqueue_burst(q, (void**) off_pkts, off_idx, NULL);
            off_idx = 0;
        }
    } 

    _task.fast_reschedule();
    if(batch)
        return true;
    else
        return false;
}

enum {
    h_vendor, h_driver, h_carrier, h_duplex, h_autoneg, h_speed, h_type,
    h_ipackets, h_ibytes, h_imissed, h_ierrors, h_nombufs,
    h_stats_packets, h_stats_bytes,
    h_active, h_safe_active,
    h_xstats, h_queue_count,
    h_nb_rx_queues, h_nb_tx_queues, h_nb_vf_pools,
    h_rss, h_rss_reta, h_rss_reta_size,
    h_mac, h_add_mac, h_remove_mac, h_vf_mac,
    h_mtu,
    h_device, h_isolate,
#if HAVE_FLOW_API
    h_rule_add, h_rules_del, h_rules_flush,
    h_rules_list, h_rules_list_with_hits, h_rules_ids_global, h_rules_ids_internal,
    h_rules_count, h_rules_count_with_hits, h_rule_packet_hits, h_rule_byte_count,
    h_rules_aggr_stats
#endif
};

int DyssectWorkingCore::xstats_handler(
        int operation, String &input, Element *e,
        const Handler *handler, ErrorHandler *errh) {
    DyssectWorkingCore *fd = static_cast<DyssectWorkingCore *>(e);

    int op = (intptr_t)handler->read_user_data();
    switch (op) {
        case h_xstats: {
            struct rte_eth_xstat_name *names;
        #if RTE_VERSION >= RTE_VERSION_NUM(16,07,0,0)
            int len = rte_eth_xstats_get_names(fd->portid, 0, 0);
            names = static_cast<struct rte_eth_xstat_name *>(
                malloc(sizeof(struct rte_eth_xstat_name) * len)
            );
            rte_eth_xstats_get_names(fd->portid, names, len);
            struct rte_eth_xstat *xstats;
            xstats = static_cast<struct rte_eth_xstat *>(malloc(
                sizeof(struct rte_eth_xstat) * len)
            );
            rte_eth_xstats_get(fd->portid,xstats,len);
            if (input == "") {
                StringAccum acc;
                for (int i = 0; i < len; i++) {
                    acc << names[i].name << "[" <<
                           xstats[i].id << "] = " <<
                           xstats[i].value << "\n";
                }

                input = acc.take_string();
            } else {
                for (int i = 0; i < len; i++) {
                    if (strcmp(names[i].name,input.c_str()) == 0) {
                        input = String(xstats[i].value);
                        return 0;
                    }
                }
                return -1;
            }
            return 0;
        #else
            input = "unsupported with DPDK < 16.07";
            return -1;
        #endif
        }
        case h_queue_count:
            if (input == "") {
                StringAccum acc;
                for (uint16_t i = 0; i < 1; i++) {
                    int v = rte_eth_rx_queue_count(fd->portid, i);
                    acc << "Queue " << i << ": " << v << "\n";
                }
                input = acc.take_string();
            } else {
                int v = rte_eth_rx_queue_count(fd->portid, atoi(input.c_str()));
                input = String(v);
            }
            return 0;
        case h_stats_packets:
        case h_stats_bytes: {
            struct rte_eth_stats stats;
            if (rte_eth_stats_get(fd->portid, &stats))
                return -1;

            int id = atoi(input.c_str());
            if (id < 0 || id > RTE_ETHDEV_QUEUE_STAT_CNTRS)
                return -EINVAL;
            uint64_t v;
            if (op == (int) h_stats_packets)
                 v = stats.q_ipackets[id];
            else
                 v = stats.q_ibytes[id];
            input = String(v);
            return 0;
        }
        default:
            return -1;
    }
}

void DyssectWorkingCore::add_handlers()
{
    set_handler("xstats", Handler::f_read | Handler::f_read_param, xstats_handler, h_xstats);
    set_handler("queue_count", Handler::f_read | Handler::f_read_param, xstats_handler, h_queue_count);
    set_handler("queue_packets", Handler::f_read | Handler::f_read_param, xstats_handler, h_stats_packets);
    set_handler("queue_bytes", Handler::f_read | Handler::f_read_param, xstats_handler, h_stats_bytes);
#if HAVE_FLOW_API
    set_handler(FlowRuleManager::FLOW_RULE_PACKET_HITS, Handler::f_read | Handler::f_read_param, xstats_handler, h_rule_packet_hits);
    set_handler(FlowRuleManager::FLOW_RULE_BYTE_COUNT,  Handler::f_read | Handler::f_read_param, xstats_handler, h_rule_byte_count);
    set_handler(FlowRuleManager::FLOW_RULE_AGGR_STATS,  Handler::f_read | Handler::f_read_param, xstats_handler, h_rules_aggr_stats);
#endif

}

CLICK_ENDDECLS
EXPORT_ELEMENT(DyssectWorkingCore)
