#include <string>
#include <vector>

#include <Serializer.h>

typedef struct tag_media_info {
    std::string     payload;
    int             payload_type = 0;
}   media_info_t;

typedef std::vector<media_info_t>   media_list_t;

class V100_command {

    public:
        void LoadParameters () {
            // Host side. Prepare parameters required at device side.
            request_param1 = 123;
            request_param2 = 456;
            request_param3 = 789;
        }

        void ExecCommand () {
            // Device side. Simulate command execution. Prepare response.
            for ( size_t i = 0; i < 5; i++ ) {
                media_info_t new_item;
                new_item.payload = "abc";
                new_item.payload_type = 3;
                response_media.emplace_back ( std::move (new_item) );
            }
        }

        bool Compare ( const V100_command& ref ) {

            // Stuff command. Not required in the real application.
            bool ret_val = true;
            
            if ( this->request_param1 != ref.request_param1 ) {
                ret_val = false;
            }
            if ( this->request_param2 != ref.request_param2 ) {
                ret_val = false;
            }
            if ( this->request_param3 != ref.request_param3 ) {
                ret_val = false;
            }

            if ( this->response_media.size () != ref.response_media.size () ) {
                ret_val = false;
            } else {
                for ( size_t i = 0; i < this->response_media.size (); i++ ) {
                    if( this->response_media [i].payload != ref.response_media [i].payload ) {
                        ret_val = false;
                    }
                    if( this->response_media [i].payload_type != ref.response_media [i].payload_type ) {
                        ret_val = false;
                    }
                }
            }

            return ret_val;
        }

    public:

        bool ParametersExport ( ::hid::types::storage_t& storage ) {
            // (Step 1) Host side. Serialize parameters to external storage.
            hid::Serializer hid_tool;
            hid_tool.StoreFix ( request_param1, storage );
            hid_tool.StoreFix ( request_param2, storage );
            hid_tool.StoreFix ( request_param3, storage );
            return hid_tool.ExportStatus ();
        }

        bool ParametersImport ( const ::hid::types::storage_t& storage ) {
            // (Step 2) Device side. Load parameters.
            hid::Serializer hid_tool;
            hid_tool.LoadFix (storage, request_param1);
            hid_tool.LoadFix (storage, request_param2);
            hid_tool.LoadFix (storage, request_param3);
            return hid_tool.ImportStatus (storage);

        }

        bool ResponseExport ( ::hid::types::storage_t& storage ) {
            // (Step 3) Device side. Serialize response to external storage.
            hid::Serializer hid_tool;
            hid_tool.StoreCnt ( response_media.size(), storage );
            for( size_t i = 0; i < response_media.size (); i++ ) {
                hid_tool.StoreVar ( response_media [i].payload, storage );
                hid_tool.StoreFix ( response_media [i].payload_type, storage );
            }
            return hid_tool.ExportStatus ();
        }

        bool ResponseImport ( const ::hid::types::storage_t& storage ) {
            // (Step 4) Host side. Load response.
            hid::Serializer hid_tool;
            size_t cnt;
            hid_tool.LoadCnt (storage, cnt);
            for ( size_t i = 0; i < cnt; i++ ) {
                media_info_t new_item;
                hid_tool.LoadVar ( storage, new_item.payload );
                hid_tool.LoadFix ( storage, new_item.payload_type );
                response_media.emplace_back ( std::move(new_item) );
            }
            return hid_tool.ImportStatus (storage);
        }

    protected:
        char            request_param1 = 0;
        int             request_param2 = 0;
        size_t          request_param3 = 0;

    protected:
        media_list_t    response_media;
};


int main () {

    V100_command                host_object;
    ::hid::types::storage_t     host_transport_transaction_out;
    ::hid::types::storage_t     host_transport_transaction_in;

    V100_command                device_object;
    ::hid::types::storage_t     device_transport_transaction_in;
    ::hid::types::storage_t     device_transport_transaction_out;


    // Host side.
    {   // Prepare input parameters.
        host_object.LoadParameters ();

        {   // Optional but strongly recommended.
            // Estimate the required size and reserve memory.
            host_transport_transaction_out.reserve ( 1024 );
        }

        // Serialize parameters out.
        host_object.ParametersExport (host_transport_transaction_out);
    }

    // Simulate data transfer (host -> device).
    {   // Transfer data from host to device.
        device_transport_transaction_in = host_transport_transaction_out;
    }

    // Device side.
    {   // Serialize parameters in.
        device_object.ParametersImport ( device_transport_transaction_in );

        // Execute command and prepare response.
        device_object.ExecCommand ();

        {   // Optional but strongly recommended.
            // Estimate the required size and reserve memory.
            device_transport_transaction_out.reserve ( 1024 );
        }

        // Export response. 
        device_object.ResponseExport (device_transport_transaction_out);
    }

    // Simulate data transfer. (device -> host).
    {   // Transfer from device to host.
        host_transport_transaction_in = device_transport_transaction_out;
    }

    // Host side.
    {   // Import response.
        host_object.ResponseImport (host_transport_transaction_in);
    }

    // Compare objects. Both must be the same.
    host_object.Compare (device_object);

    return 0;
}
