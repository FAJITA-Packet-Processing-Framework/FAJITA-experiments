#include <click/config.h>
#include "dyssectcontroller.hh"
#include <click/args.hh>
#include <click/router.hh>
#include <click/standard/scheduleinfo.hh>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_pci.h>
#include <rte_errno.h>
#include <rte_version.h>

CLICK_DECLS

uint64_t tsc_hz;

uint32_t DyssectController::iqueues = 0;
rte_ring** DyssectController::rings = 0;
rte_ring** DyssectController::queues = 0;
ShardInfo* DyssectController::shards = 0;
rte_atomic32_t* DyssectController::epoch = 0;
rte_atomic32_t** DyssectController::had_changes = 0;
rte_atomic32_t** DyssectController::transfer_r = 0;
rte_atomic32_t** DyssectController::transfer_shard = 0;
rte_atomic32_t** DyssectController::transfer_offloading = 0;
rte_atomic32_t** DyssectController::controller_signal = 0;
rte_atomic32_t** DyssectController::offloading_from_working = 0;
rte_atomic32_t** DyssectController::new_offloading_from_working = 0;
uint32_t DyssectController::sfc_length = 2;
uint32_t DyssectController::total_shards = 128;
rte_mempool *DyssectController::pktmbuf_pool = 0;
rte_ring** DyssectController::toAddQueue = 0;
rte_ring** DyssectController::toRemoveQueue = 0;

inline uint64_t tsc_to_us(uint64_t cycles) 
{   
    return cycles * 1000000.0 / tsc_hz;
}

bool sortByVal(DyssectState* a, DyssectState* b)
{
    return (a->prob2 > b->prob2);
}

inline bool is_equal_than(double a, double b)
{
    return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * EPSILON);
}

static inline
double processingtime_r(uint32_t, uint32_t)
{
    /* TO COMPLETE FOR YOU OWN PURPOSES */
    return 1 * 1e-6;
}

static inline
double processingtime_p(uint32_t, uint32_t)
{
    /* TO COMPLETE FOR YOU OWN PURPOSES */
    return 1 * 1e-6;
}

static inline int from_pipe(int fd, uint8_t* addr, int len)
{
    int n;
    int i = 0;

    while(i != len)
    {
        n = read(fd, addr + i, len - i);
        if(n <= 0)
        {
            return i;
        }

        i += n;
    }

    return i;
}

static inline
int to_pipe(int fd, uint8_t* addr, int len)
{
    int n;
    int i = 0;

    while(i != len)
    {
        n = write(fd, addr + i, len - i);
        if(n <= 0)
        {
            return i;
        }

        i += n;
    }

    return i;
}

DyssectController::DyssectController()
    : portid(0), _task(this)
{
    next_short = 0;
    total_flows = 0;
    total_bytes = 0;
    total_packets = 0;
    last_totalpackets = 0;
    tsc_hz = rte_get_timer_hz();
}

DyssectController::~DyssectController()
{
    
}

void DyssectController::init_dpdk()
{
    struct rte_eth_conf dev_conf;
    struct rte_eth_dev_info dev_info;
    memset(&dev_conf, 0, sizeof(dev_conf));
    rte_eth_dev_info_get(portid, &dev_info);

    reta_size = dev_info.reta_size;

    char name[128];
    sprintf(name, "mbuf_pool");
    pktmbuf_pool = rte_pktmbuf_pool_create(name, PKTMBUF_POOL_ELEMENTS, MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if(pktmbuf_pool == NULL)
    {
        rte_exit(EXIT_FAILURE, "Cannot init mbuf pool on socket %d\n", rte_socket_id());
    }

    if(iqueues > 1)
    {
        dev_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_RSS;
        dev_conf.rx_adv_conf.rss_conf.rss_key = NULL;
        dev_conf.rx_adv_conf.rss_conf.rss_hf = RTE_ETH_RSS_IP | RTE_ETH_RSS_UDP | RTE_ETH_RSS_TCP;
        dev_conf.rx_adv_conf.rss_conf.rss_hf &= dev_info.flow_type_rss_offloads;

        {
            uint8_t rss_key[40] = {
                0x7c,0x9c,0x37,0xde,0x18,0xdc,0x43,0x86,0xd9,0x27,
                0x0f,0x6f,0x26,0x03,0x74,0xb8,0xbf,0xd0,0x40,0x4b,
                0x78,0x72,0xe2,0x24,0xdc,0x1b,0x91,0xbb,0x01,0x1b,
                0xa7,0xa6,0x37,0x6c,0xc8,0x7e,0xd6,0xe3,0x14,0x17,
            };
            
            dev_conf.rx_adv_conf.rss_conf.rss_key = new uint8_t[40];
            memcpy(dev_conf.rx_adv_conf.rss_conf.rss_key, rss_key, 40);
            dev_conf.rx_adv_conf.rss_conf.rss_key_len = 40;
        }

    }

    int ret;
    if((ret = rte_eth_dev_configure(portid, iqueues, oqueues, &dev_conf)) < 0)
        rte_exit(EXIT_FAILURE, "Cannot initialize DPDK port %u with %u RX and %u TX queues\nError %d : %s", portid, iqueues, oqueues, ret, strerror(ret));
    
    struct rte_eth_rxconf rx_conf;
    memcpy(&rx_conf, &dev_info.default_rxconf, sizeof(rx_conf));

    for(int i = 0; i < iqueues; i++) 
    {
        if(rte_eth_rx_queue_setup(portid, i, ndesc_in, 0, &rx_conf, pktmbuf_pool) != 0)
            rte_exit(EXIT_FAILURE, "Cannot initialize RX queue %u of port %u on node %u : %s", i, portid, 0, rte_strerror(rte_errno));
    }

    struct rte_eth_txconf tx_conf;
    tx_conf = dev_info.default_txconf;
    memcpy(&tx_conf, &dev_info.default_txconf, sizeof(tx_conf));

    for(int i = 0; i < oqueues; i++) 
    {
        if(rte_eth_tx_queue_setup(portid, i, ndesc_out, 0, nullptr) != 0)
            rte_exit(EXIT_FAILURE, "Cannot initialize TX queue %u of port %u on node %u : %s",
                i, portid, 0, rte_strerror(rte_errno));
    }

    int err = rte_eth_dev_start(portid);
    if(err < 0)
        rte_exit(EXIT_FAILURE, "Cannot start DPDK port %u: error %d", portid, err);

    memset(reta_conf, 0, sizeof(reta_conf));
    for (int i = 0; i < reta_size; i++)
        reta_conf[i / RTE_ETH_RETA_GROUP_SIZE].mask = UINT64_MAX;

    A = (uint32_t*) rte_zmalloc(NULL, total_shards * total_cores * sizeof(uint32_t), 64);
    newA = (uint32_t*) rte_zmalloc(NULL, total_shards * total_cores * sizeof(uint32_t), 64);
    for(uint32_t s = 0; s < total_shards; s++)
    {
        for(uint32_t j = s; j < reta_size; j += total_shards)
        {
            reta_conf[j / RTE_ETH_RETA_GROUP_SIZE].reta[j % RTE_ETH_RETA_GROUP_SIZE] = s % W;
        }
        A[s*total_cores + (s % W)] = 1;
    }

    O = (uint32_t*) rte_zmalloc(NULL, total_cores * total_cores * sizeof(uint32_t), 64);
    newO = (uint32_t*) rte_zmalloc(NULL, total_cores * total_cores * sizeof(uint32_t), 64);
    for(uint32_t j = 0; j < W; j++)
    {
        if(E)
        {
            O[(j % E)*total_cores + j] = 1;
            rte_atomic32_set(offloading_from_working[j], j % E);
        } else 
        {
            rte_atomic32_set(offloading_from_working[j], UINT32_MAX);
        }

        rte_atomic32_set(new_offloading_from_working[j], UINT32_MAX);
    }

    ret = rte_eth_dev_rss_reta_update(portid, reta_conf, reta_size);

    rte_memcpy(newA, A, total_shards * total_cores * sizeof(uint32_t));
    rte_memcpy(newO, O, total_cores  * total_cores * sizeof(uint32_t));

    rte_eth_promiscuous_enable(portid);
}

void DyssectController::init_shards()
{
    shards = (ShardInfo*) rte_zmalloc(NULL, total_shards * sizeof(ShardInfo), 64);

    DyssectFlow *flow_empty = (DyssectFlow*) rte_zmalloc(NULL, sizeof(DyssectFlow), 64);
    DyssectFlow *flow_deleted = (DyssectFlow*) rte_zmalloc(NULL, sizeof(DyssectFlow), 64);
    flow_empty->src_port = flow_deleted->src_port = 0;
    flow_empty->dst_port = flow_deleted->dst_port = 0;
    flow_empty->src_addr = flow_deleted->src_addr = 0;
    flow_empty->dst_addr = flow_deleted->dst_addr = 0;

    for(uint32_t i = 0; i < total_shards; i++) {
        struct rte_hash_parameters hash_params = {0};
        char buf[64];
        sprintf(buf, "%i-%s", i, name().c_str());
        hash_params.name = buf;
        hash_params.entries = HASH_TABLE_SIZE;

        hash_params.key_len = sizeof(DyssectFlow);
        hash_params.hash_func = ipv4_hash_crc2;
        hash_params.hash_func_init_val = 0;
        hash_params.extra_flag = 0;

//        shards[i]._table = rte_hash_create(&hash_params);
        shards[i].table = new DyssectHashTable(HASH_TABLE_SIZE);
        shards[i].table->set_empty_key(*flow_empty);
        shards[i].flows3 = new ((DyssectHashTable*) rte_zmalloc(NULL, sizeof(DyssectHashTable), 64)) DyssectHashTable();
        shards[i].flows3->set_empty_key(*flow_empty);
        shards[i].flows3_2 = new ((DyssectHashTable*) rte_zmalloc(NULL, sizeof(DyssectHashTable), 64)) DyssectHashTable();
        shards[i].flows3_2->set_empty_key(*flow_empty);

        shards[i].use_2 = false;

        shards[i].ordered_flows = new ((std::vector<DyssectState*>*) rte_zmalloc(NULL, sizeof(std::vector<DyssectState*>), 64)) std::vector<DyssectState*>();

        rte_atomic32_init(&shards[i].pause);
        shards[i].owner = i % W;

        rte_atomic32_init(&shards[i].bytes);
        rte_atomic32_init(&shards[i].packets);

        size_t queue_size = 2*1024;
        rte_ring_init(&shards[i].local_queue, NULL, queue_size, RING_F_SP_ENQ|RING_F_SC_DEQ);
    }
}

void DyssectController::init_queues() 
{
    rings = (rte_ring**) rte_zmalloc(NULL, W * sizeof(rte_ring*), 64);
    queues = (rte_ring**) rte_zmalloc(NULL, W * sizeof(rte_ring*), 64);
    toAddQueue = (rte_ring**) rte_zmalloc(NULL, E * sizeof(rte_ring*), 64);
    toRemoveQueue = (rte_ring**) rte_zmalloc(NULL, E * sizeof(rte_ring*), 64);

    for(uint32_t i = 0; i < E; i++)
    {
        char name[128];
        sprintf(name, "toAdd%d", i);
        toAddQueue[i] = rte_ring_create(name, 4096, 0, RING_F_SP_ENQ|RING_F_SC_DEQ);
        sprintf(name, "toRemove%d", i);
        toRemoveQueue[i] = rte_ring_create(name, 4096, 0, RING_F_SP_ENQ|RING_F_SC_DEQ);
    }

    for(uint32_t i = 0; i < W; i++)
    {
        char name[128];
        sprintf(name, "ring%d", i);
        rings[i] = rte_ring_create(name, 4096, 0, RING_F_SP_ENQ|RING_F_SC_DEQ);
        sprintf(name, "queue%d", i);
        queues[i] = rte_ring_create(name, 4096, 0, RING_F_SP_ENQ|RING_F_SC_DEQ);

        if(E != 0)
        {
            rte_ring_enqueue(toAddQueue[i % E], (void*) queues[i]);
        }
    }
}

int DyssectController::initialize(ErrorHandler *errh)
{
    ScheduleInfo::initialize_task(this, &_task, true, errh);

    return 0;
}

int DyssectController::configure(Vector<String> &conf, ErrorHandler *errh)
{
    if(Args(conf, this, errh)
        .read_or_set("PORT", portid, 0)
        .read("NDESC_IN", ndesc_in)
        .read("NDESC_OUT", ndesc_out)
        .read_or_set("SHARDS", DyssectController::total_shards, 128)
        .read("SFC_LENGTH", DyssectController::sfc_length)
        .read("W", W)
        .read("E", E)
        .read("SLOr", SLOr)
        .read("SLOp", SLOp)
        .read_or_set("SOLVER", solver, false)
        .complete() < 0)
        return -1;

    iqueues = W;
    oqueues = W + E;
    total_cores = W + E;

    Tr = Tp = 1 * 1e-6;
    SLOp = SLOp * 1e-9;
    SLOr = SLOr * 1e-9;
    Cap = 0.0914;
    Car = 0.0966;
    Csp = 0.1166;
    Csr = 0.1202;

    solver_IN  = (char*) malloc(1024);
    solver_OUT = (char*) malloc(1024);
    memset(solver_IN, 0, 1024);
    memset(solver_OUT, 0, 1024);
    strcpy(solver_IN, "./solver_IN");
    strcpy(solver_OUT, "./solver_OUT");

    int status = mkfifo(solver_OUT, 0755);
    if(status < 0)
    {
        unlink(solver_OUT);
        status = mkfifo(solver_OUT, 0755);
    }

    char buff[128];
    int __attribute__((unused)) ret = sprintf(buff, "chmod 777 %s 1>/dev/null 2>/dev/null", solver_OUT);
    ret = system(buff);

    epoch = (rte_atomic32_t*) rte_zmalloc(NULL, sizeof(rte_atomic32_t), 64);
    rte_atomic32_init(epoch);

    had_changes = (rte_atomic32_t**) rte_zmalloc(NULL, total_cores * sizeof(rte_atomic32_t*), 64);
    transfer_r = (rte_atomic32_t**) rte_zmalloc(NULL, total_cores * sizeof(rte_atomic32_t*), 64);
    transfer_shard = (rte_atomic32_t**) rte_zmalloc(NULL, total_cores * sizeof(rte_atomic32_t*), 64);
    transfer_offloading = (rte_atomic32_t**) rte_zmalloc(NULL, total_cores * sizeof(rte_atomic32_t*), 64);
    controller_signal = (rte_atomic32_t**) rte_zmalloc(NULL, total_cores * sizeof(rte_atomic32_t*), 64);
    offloading_from_working = (rte_atomic32_t**) rte_zmalloc(NULL, total_cores * sizeof(rte_atomic32_t*), 64);
    new_offloading_from_working = (rte_atomic32_t**) rte_zmalloc(NULL, total_cores * sizeof(rte_atomic32_t*), 64);

    for(uint32_t i = 0; i < total_cores; i++)
    {
        had_changes[i] = (rte_atomic32_t*) rte_zmalloc(NULL, sizeof(rte_atomic32_t), 64);
        transfer_r[i] = (rte_atomic32_t*) rte_zmalloc(NULL, sizeof(rte_atomic32_t), 64);
        transfer_shard[i] = (rte_atomic32_t*) rte_zmalloc(NULL, sizeof(rte_atomic32_t), 64);
        transfer_offloading[i] = (rte_atomic32_t*) rte_zmalloc(NULL, sizeof(rte_atomic32_t), 64);
        controller_signal[i] = (rte_atomic32_t*) rte_zmalloc(NULL, sizeof(rte_atomic32_t), 64);
        offloading_from_working[i] = (rte_atomic32_t*) rte_zmalloc(NULL, sizeof(rte_atomic32_t), 64);
        new_offloading_from_working[i] = (rte_atomic32_t*) rte_zmalloc(NULL, sizeof(rte_atomic32_t), 64);
        rte_atomic32_init(had_changes[i]);
        rte_atomic32_init(transfer_r[i]);
        rte_atomic32_init(transfer_shard[i]);
        rte_atomic32_init(transfer_offloading[i]);
        rte_atomic32_init(controller_signal[i]);
        rte_atomic32_init(offloading_from_working[i]);
        rte_atomic32_init(new_offloading_from_working[i]);
    }

    init_dpdk();
    click_chatter("initializing shards!");
    init_shards();
    click_chatter("initializing queues!");
    init_queues();
    click_chatter("controller initialized!");

    return 0;
}

bool DyssectController::update_ratio()
{
    bool changed = false;

    for(uint32_t s = 0; s < total_shards; s++)
    {
        if(!is_equal_than(shards[s].r_new, 0.0))
        {
            double accum = 0.0;
            std::vector<DyssectState*> *vec = shards[s].ordered_flows;

            for(auto it = vec->begin(); it != vec->end(); it++)
            {
                accum += (*it)->prob2;
                if(accum >= shards[s].r_new)
                {
                    if(accum < shards[s].r_new * SCALE)
                    {
                        shards[s].r_new = accum + EPSILON;
                    }
                    break;
                }
            }
        }

        if(!is_equal_than(shards[s].r, shards[s].r_new))
        {
            uint32_t owner = shards[s].owner;
            rte_atomic32_set(transfer_r[owner], 1);

            changed = true;
        }
    }
    return changed;
}

bool DyssectController::update_relationship()
{
    if(memcmp(newO, O, total_cores * total_cores * sizeof(uint32_t)) == 0)
    {
        return false;
    }

    for(uint32_t w = 0; w < total_cores; w++)
    {
        for(uint32_t e = 0; e < total_cores; e++)
        {
            if(O[e*total_cores + w] == 0 && newO[e*total_cores + w] == 1)
            {
                uint32_t working = w;
                uint32_t new_offloading_idx = e;

                if(rte_atomic32_read(offloading_from_working[working]) != UINT32_MAX)
                {
                    uint32_t offloading_idx = rte_atomic32_read(offloading_from_working[working]);
                    if(offloading_idx == new_offloading_idx)
                    {
                        continue;
                    }
                    
                    rte_atomic32_set(new_offloading_from_working[working], new_offloading_idx);
                    rte_atomic32_set(transfer_offloading[working], 1);
                }
            }

            if(O[e*total_cores + w] == 1 && newO[e*total_cores + w] == 0)
            {
                uint32_t working = w;
                uint32_t offloading_idx = e;

                rte_ring *q = queues[working];
                rte_ring_enqueue(toRemoveQueue[offloading_idx], q);
            }
        }
    }

    rte_memcpy(O, newO, total_cores * total_cores * sizeof(uint32_t));

    return true;
}

void DyssectController::migration_shard(uint32_t s, uint32_t w, bool send_signal)
{
    if(shards[s].owner == w)
    {
        return;
    }

    if(!send_signal)
    {
        shards[s].owner_new = w;
        rte_atomic32_set(&shards[s].pause, 1);
    } else
    {
        rte_atomic32_set(transfer_shard[shards[s].owner], 1);
    }
}

#define RETA_CONF_SIZE     (RTE_ETH_RSS_RETA_SIZE_512 / RTE_ETH_RETA_GROUP_SIZE)
void DyssectController::update_reta()
{
    uint32_t transfers = 0;
    for(uint32_t s = 0; s < total_shards; s++)
    {
        for(uint32_t w = 0; w < total_cores; w++)
        {
            if(w >= W)
            {
                continue;
            }

            if(A[s*total_cores + w] != newA[s*total_cores + w])
            {
                transfers++;
            }

            if(newA[s*total_cores + w] == 1)
            {
                for(uint32_t k = s; k < reta_size; k += total_shards)
                {
                    reta_conf[k / RTE_ETH_RETA_GROUP_SIZE].reta[k % RTE_ETH_RETA_GROUP_SIZE] = w;
                }
            }
        }
    }

    transfers /= 2;

    if(transfers != 0)
    {
        click_chatter("MIGRATIONS=%d", transfers);
        int ret = rte_eth_dev_rss_reta_update(portid, reta_conf, reta_size);
    }
}


bool DyssectController::update_shards()
{
    if(memcmp(A, newA, total_shards * total_cores * sizeof(uint32_t)) == 0)
    {
        return false;
    }

    for(uint32_t s = 0; s < total_shards; s++)
    {
        for(uint32_t w = 0; w < total_cores; w++)
        {
            if(newA[s*total_cores + w] == 1 && A[s*total_cores + w] == 0)
            {
                migration_shard(s, w, false);
            }
        }
    }

    update_reta();

    for(uint32_t s = 0; s < total_shards; s++)
    {
        for(uint32_t w = 0; w < total_cores; w++)
        {
            if(newA[s*total_cores + w] == 1 && A[s*total_cores + w] == 0)
            {
                migration_shard(s, w, true);
            }
        }
    }

    rte_memcpy(A, newA, total_shards * total_cores * sizeof(uint32_t));

    return true;
}

bool DyssectController::run_short_solver()
{
    int fd = open((const char*) solver_IN, O_CREAT|O_WRONLY, 0777);

    if(fd == -1)
    {
        return false;
    }

    uint32_t mode = SHORT;
    int n;

    n = write(fd, &mode, sizeof(uint32_t));
    n = write(fd, &W, sizeof(uint32_t));
    n = write(fd, &E, sizeof(uint32_t));
    n = write(fd, &total_cores, sizeof(uint32_t));
    n = write(fd, &total_shards, sizeof(uint32_t));

    n = write(fd, &Cap,  sizeof(double));
    n = write(fd, &Csp,  sizeof(double));
    n = write(fd, &SLOp, sizeof(double));
    n = write(fd, &Car,  sizeof(double));
    n = write(fd, &Csr,  sizeof(double));
    n = write(fd, &SLOr, sizeof(double));

    n = write(fd, &Tr, sizeof(double));
    n = write(fd, &Tp, sizeof(double));

    for(uint32_t s = 0; s < total_shards; s++)
    {
        n = write(fd, &shards[s].V, sizeof(double));
    }

    for(uint32_t s = 0; s < total_shards; s++)
    {
        n = write(fd, &shards[s].r, sizeof(double));
    }

    n = to_pipe(fd, (uint8_t*) A, total_shards * total_cores * sizeof(uint32_t));
    n = to_pipe(fd, (uint8_t*) O, total_cores  * total_cores * sizeof(uint32_t));

    close(fd);

    fd = open((const char*) solver_OUT, O_RDONLY);

    if(fd == -1)
    {
        return false;
    }

    int value;
    n = read(fd, &value, sizeof(int));

    if(value == 1)
    {
        for(uint32_t s = 0; s < total_shards; s++)
        {
            n = read(fd, &shards[s].r_new, sizeof(double));
        }

        n = from_pipe(fd, (uint8_t*) newA, total_shards * total_cores * sizeof(uint32_t));
        n = from_pipe(fd, (uint8_t*) newO, total_cores  * total_cores * sizeof(uint32_t));
    }

    close(fd);
    
    return value == 1;
}

void DyssectController::swap_shards()
{
    for(uint32_t s = 0; s < total_shards; s++)
    {
        shards[s].use_2 ^= true;
    }

    last_totalpackets = total_packets;

    total_bytes = 0;
    total_packets = 0;

    for(uint32_t s = 0; s < total_shards; s++)
    {
        shards[s].old_bytes = rte_atomic32_read(&shards[s].bytes);
        shards[s].old_packets = rte_atomic32_read(&shards[s].packets);
        rte_atomic32_clear(&shards[s].bytes);
        rte_atomic32_clear(&shards[s].packets);

        total_bytes += shards[s].old_bytes;
        total_packets += shards[s].old_packets;
    }
}

void DyssectController::volume_shards()
{
    if(!last_totalpackets)
    {
        return;
    }

    if(total_packets)
    {
        Tr = processingtime_r(total_flows, W);
        Tp = processingtime_p(total_flows, W);

        for(uint32_t s = 0; s < total_shards; s++)
        {
            shards[s].V = (double)shards[s].old_packets * Tp * 1e6/SHORT_TIME;
        }
    } else
    {
        for(uint32_t s = 0; s < total_shards; s++)
        {
            shards[s].V = 0;
        }
    }
}

void DyssectController::order_shards()
{
    total_flows = 0;

    for(uint32_t s = 0; s < total_shards; s++)
    {
        DyssectHashTable *pflow;
        std::vector<DyssectState*> *vec = shards[s].ordered_flows;

        if(shards[s].use_2)
        {
            while(rte_atomic32_read(&shards[s].ref_count_1) != 0)
            {
            }
            pflow = shards[s].flows3;
        } else
        {
            while(rte_atomic32_read(&shards[s].ref_count_2) != 0)
            {
            }
            pflow = shards[s].flows3_2;
        }

        vec->clear();
        for(auto it = pflow->begin(); it != pflow->end(); it++)
        {
            it->second->prob2 = it->second->prob;
            vec->push_back(it->second);
        }

        total_flows += pflow->size();
        std::sort(vec->begin(), vec->end(), sortByVal);

        double accum = 0.0;
        for(auto it = vec->begin(); it != vec->end(); it++)
        {
            accum += (*it)->prob2;
            (*it)->cdf = accum;
        }
    }
}

void DyssectController::clear_flows()
{
    for(uint32_t s = 0; s < total_shards; s++)
    {
        if(shards[s].use_2)
        {
            while(rte_atomic32_read(&shards[s].ref_count_1) != 0)
            {
            }
            shards[s].flows3->clear();
        } else
        {
            while(rte_atomic32_read(&shards[s].ref_count_2) != 0)
            {
            }
            shards[s].flows3_2->clear();
        }
    }
}

void DyssectController::update_short_epoch(bool solver)
{
    swap_shards();
    volume_shards();
    if(solver)
    {   
        int ret = run_short_solver();

        if(ret) 
        {
            bool changed_r = update_ratio();
            bool changed_relationship = update_relationship();
            bool changed_shards = update_shards();
            if(changed_r || changed_relationship || changed_shards)
            {   
                for(uint32_t i = 0; i < W; i++)
                {
                    rte_atomic32_set(controller_signal[i], 1);
                }
            }
        }
    }
    rte_atomic32_inc(epoch);

    order_shards();
    clear_flows();
}

bool DyssectController::run_task(Task *)
{
    char buffer[1024];
    memset(buffer, 0, 1024);
    uint64_t now = tsc_to_us(rte_rdtsc());
    if(now > next_short) 
    {
        update_short_epoch(solver);
        next_short = now + SHORT_TIME;
        _task.fast_reschedule();
        return true;
    }

    _task.fast_reschedule();
    return false;

}

void DyssectController::add_handlers()
{
    add_task_handlers(&_task, 0, TASKHANDLER_WRITE_ALL);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(DyssectController)
