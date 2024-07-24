#include <rte_lcore.h>
#include <click/config.h>
#include <click/glue.hh>
#include <click/args.hh>
#include "dyids.hh"


CLICK_DECLS

int DyIDS::lex(const uint8_t *YYCURSOR) {
    const uint8_t *YYMARKER;
first_group:
{
    uint8_t yych;
    unsigned int yyaccept = 0;
    yych = *YYCURSOR;
    switch (yych) {
    case 0x00:      goto yy2;
    case 0x01:      goto yy4;
    default:        goto yy6;
    }
yy2:
    ++YYCURSOR;
    { return 0; }
yy4:
    yyaccept = 0;
    yych = *(YYMARKER = ++YYCURSOR);
    switch (yych) {
    case 0x00:      goto yy5;
    case 0x01:      goto yy7;
    case 0x03:      goto yy10;
    default:        goto yy9;
    }
yy5:
    { goto first_group; }
yy6:
    yyaccept = 0;
    yych = *(YYMARKER = ++YYCURSOR);
    switch (yych) {
    case 0x00:      goto yy5;
    case 0x01:      goto yy7;
    default:        goto yy9;
    }
yy7:
    yych = *++YYCURSOR;
    switch (yych) {
    case 0x01:      goto yy12;
    case 0x03:      goto yy10;
    default:        goto yy8;
    }
yy8:
    YYCURSOR = YYMARKER;
    if (yyaccept == 0) {
        goto yy5;
    } else {
        goto yy15;
    }
yy9:
    yych = *++YYCURSOR;
    switch (yych) {
    case 0x01:      goto yy12;
    default:        goto yy8;
    }
yy10:
    yych = *++YYCURSOR;
    switch (yych) {
    case 0x00:      goto yy8;
    case 0x0b:      goto yy13;
    default:        goto yy10;
    }
yy12:
    yych = *++YYCURSOR;
    switch (yych) {
    case 0x03:      goto yy10;
    default:        goto yy8;
    }
yy13:
    yyaccept = 1;
    yych = *(YYMARKER = ++YYCURSOR);
    switch (yych) {
    case 0x00:      goto yy15;
    case 0x0b:      goto yy13;
    default:        goto yy10;
    }
yy15:
    { return 1; }
}
}

int DyIDS::initialize(ErrorHandler *errh) {
    return 0;
}

int DyIDS::configure(Vector<String> &conf, ErrorHandler *errh) {
    if(Args(conf, this, errh)
        .read("HANDLE", handle)
        .read("OFFSET", offset)
        .complete() < 0)
        return -1;
    return 0;
}

inline void DyIDS::UpdateState(Packet *p, IDSState *state)
{
    const click_tcp *tcp = p->tcp_header();
    uint8_t* payload = ((uint8_t*) tcp) + tcp->th_off*4;
    if(payload)
    {
        payload += this->offset;
        int ret = lex(payload);

        if(ret != 0)
        {
            state->matched[state->found++ % DYIDS_ENTRIES] = tcp->th_seq;
        } else 
        {
            state->unmatched[state->not_found++ % DYIDS_ENTRIES] = tcp->th_seq;
        }
    }
}

inline Packet *DyIDS::simple_action(Packet *pkt) {
    void** global_state = (void**) PERFCTR_ANNO(pkt);
    IDSState* state = (IDSState*) global_state[handle];

    if(!state)
    {
        state = (IDSState*) malloc(sizeof(IDSState));

        state->found = 0;
        state->not_found = 0;
        state->matched = (uint32_t*) malloc(DYIDS_ENTRIES * sizeof(uint32_t));
        state->unmatched = (uint32_t*) malloc(DYIDS_ENTRIES * sizeof(uint32_t));

        global_state[handle] = state;
    }

    UpdateState(pkt, state);

    return pkt;
}

void DyIDS::push_batch(int, PacketBatch *batch)
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

EXPORT_ELEMENT(DyIDS)
