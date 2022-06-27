#include <HidTransportInt.h>

namespace hid {
namespace transport {

//---------------------------------------------------------------------------//

transaction_t::transaction_t () {
    
    inp_cmd  = 0;
    inp_code = 0;
    out_cmd  = 0;
    out_code = 0;
}

void transaction_t::start ( duration_ms_t expiration_default_ms ) {

    inp_hdr.clear ();
    inp_pay.clear ();
    out_hdr.clear ();
    out_pay.clear ();

    tv_rcv_hdr  = checkpoint_t ( duration_ms_t ( 0 ) );
    tv_rcv_pay  = checkpoint_t ( duration_ms_t ( 0 ) );
    tv_exec     = checkpoint_t ( duration_ms_t ( 0 ) );
    tv_snt_hdr  = checkpoint_t ( duration_ms_t ( 0 ) );
    tv_snt_pay  = checkpoint_t ( duration_ms_t ( 0 ) );

    tv_start    = time_source_t::now ();
    tv_expiration = tv_start + expiration_default_ms;
}

void transaction_t::checkpoint_set ( checkpoint_id_t point_type ) {

    checkpoint_t ref = time_source_t::now();

    switch ( point_type ) {
        case checkpoint_id_t::CHECKPOINT_START:
            tv_start = ref;
            break;
        case checkpoint_id_t::CHECKPOINT_RX_HDR:
            tv_rcv_hdr = ref;
            break;
        case checkpoint_id_t::CHECKPOINT_RX_PAYLOAD:
            tv_rcv_pay = ref;
            break;
        case checkpoint_id_t::CHECKPOINT_EXEC:
            tv_exec = ref;
            break;
        case checkpoint_id_t::CHECKPOINT_TX_HDR:
            tv_snt_hdr = ref;
            break;
        case checkpoint_id_t::CHECKPOINT_TX_PAYLOAD:
            tv_snt_pay = ref;
            break;
        case checkpoint_id_t::CHECKPOINT_UNKNOWN:
        default:
            break;
    }
}

void transaction_t::expiration_set ( duration_ms_t expiration_ms ) {

    checkpoint_t   expiration_tp;
    expiration_tp = tv_start + expiration_ms;
    tv_expiration = expiration_tp;
}

void transaction_t::reset ( void ) {

    checkpoint_t zero_val = {};

    inp_hdr.clear ();
    inp_pay.clear ();
    out_hdr.clear ();
    out_pay.clear ();

    inp_cmd       = 0;

    tv_start      = zero_val;
    tv_rcv_hdr    = zero_val;
    tv_rcv_pay    = zero_val;
    tv_exec       = zero_val;
    tv_snt_hdr    = zero_val;
    tv_snt_pay    = zero_val;
    tv_expiration = zero_val;
}

//---------------------------------------------------------------------------//

void TransportServer::SetHandler ( ev_handler_t handler ) {
    m_access_controller.lock ();
        m_ev_handler = handler;
    m_access_controller.unlock ();
}

bool TransportServer::Start ( const char* const port, conn_type_t conn_type ) {
    bool ret_val = false;
    m_access_controller.lock ();
        ret_val = StartMe ( port, conn_type );
    m_access_controller.unlock ();
    return ret_val;
}

void TransportServer::Stop ( void ) {
    m_access_controller.lock ();
        StopMe();
    m_access_controller.unlock ();
}

//---------------------------------------------------------------------------//

}
}
