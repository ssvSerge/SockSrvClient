#ifndef __SOCKTYPES_H__
#define __SOCKTYPES_H__

#include <stdint.h>

#include <vector>
#include <list>
#include <future>
#include <thread>
#include <map>
#include <mutex>
#include <chrono>


typedef std::chrono::high_resolution_clock      sock_time_t;
typedef std::chrono::steady_clock::time_point   sock_timepoint_t;
typedef std::chrono::time_point<std::chrono::system_clock>  expiration_t;
typedef std::chrono::time_point<std::chrono::steady_clock>  exp_t;

typedef std::chrono::milliseconds               sock_duration_ms_t;
typedef uint64_t                                sock_transaction_id_t;
typedef std::list<int>                          sock_clients_list_t;
typedef std::vector<uint8_t>                    sock_frame_t;
typedef std::future<bool>                       sock_thread_t;
typedef std::list<sock_thread_t>                sock_conn_list_t;




typedef void (*ev_handler_t) ( sock_frame_t& in_data, sock_frame_t& out_data, expiration_t expiration_time );


#endif

