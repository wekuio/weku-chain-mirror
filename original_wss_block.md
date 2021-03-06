# File path - 
`./libraries/fc/src/rpc/websocket_api.cpp`

Copy paste the code replacing the contents of the file prior to build.
Thi is needed ATM because the repository in a include, and we are using an old version, in the future this might become a local file.

```
#include <fc/rpc/websocket_api.hpp>
#include <iostream>
#include <fstream>
#include <string>

namespace fc { namespace rpc {

websocket_api_connection::~websocket_api_connection()
{
}

websocket_api_connection::websocket_api_connection( fc::http::websocket_connection& c )
   : _connection(c)
{
   _rpc_state.add_method( "call", [this]( const variants& args ) -> variant
   {
      FC_ASSERT( args.size() == 3 && args[2].is_array() );
      api_id_type api_id;
      if( args[0].is_string() )
      {
         variants subargs;
         subargs.push_back( args[0] );
         variant subresult = this->receive_call( 1, "get_api_by_name", subargs );
         api_id = subresult.as_uint64();
      }
      else
         api_id = args[0].as_uint64();

      return this->receive_call(
         api_id,
         args[1].as_string(),
         args[2].get_array() );
   } );

   _rpc_state.add_method( "notice", [this]( const variants& args ) -> variant
   {
      FC_ASSERT( args.size() == 2 && args[1].is_array() );
      this->receive_notice( args[0].as_uint64(), args[1].get_array() );
      return variant();
   } );

   _rpc_state.add_method( "callback", [this]( const variants& args ) -> variant
   {
      FC_ASSERT( args.size() == 2 && args[1].is_array() );
      this->receive_callback( args[0].as_uint64(), args[1].get_array() );
      return variant();
   } );

   _rpc_state.on_unhandled( [&]( const std::string& method_name, const variants& args )
   {
      return this->receive_call( 0, method_name, args );
   } );

   _connection.on_message_handler( [&]( const std::string& msg ){ on_message(msg,true); } );
   _connection.on_http_handler( [&]( const std::string& msg ){ return on_message(msg,false); } );
   _connection.closed.connect( [this](){ closed(); } );
}

variant websocket_api_connection::send_call(
   api_id_type api_id,
   string method_name,
   variants args /* = variants() */ )
{
   auto request = _rpc_state.start_remote_call(  "call", {api_id, std::move(method_name), std::move(args) } );
   _connection.send_message( fc::json::to_string(request) );
   return _rpc_state.wait_for_response( *request.id );
}

variant websocket_api_connection::send_callback(
   uint64_t callback_id,
   variants args /* = variants() */ )
{
   auto request = _rpc_state.start_remote_call( "callback", {callback_id, std::move(args) } );
   _connection.send_message( fc::json::to_string(request) );
   return _rpc_state.wait_for_response( *request.id );
}

void websocket_api_connection::send_notice(
   uint64_t callback_id,
   variants args /* = variants() */ )
{
   fc::rpc::request req{ optional<uint64_t>(), "notice", {callback_id, std::move(args)}};
   _connection.send_message( fc::json::to_string(req) );
}

std::string websocket_api_connection::on_message(
   const std::string& message,
   bool send_message /* = true */ )
{
   wdump((message));
   try
   {
      auto var = fc::json::from_string(message);
      const auto& var_obj = var.get_object();
      if( var_obj.contains( "method" ) )
      {
          //TODO: need to convert to consensus version, since it's a temp workaround.
          bool is_transfer_weku = false;
          if ((message.find("\"amount\":") != std::string::npos)
                   && (message.find("\"from\":") != std::string::npos)
                   && (message.find("\"to\":") != std::string::npos))
              is_transfer_weku = true;

          bool is_transfer_power = false;
          if ((message.find("\"transfer_to_vesting\":") != std::string::npos)
                   && (message.find("\"from\":") != std::string::npos)
                   && (message.find("\"to\":") != std::string::npos))
              is_transfer_power = true;

          bool is_vote = false;
          if ((message.find("\"vote\":") != std::string::npos)
                   && (message.find("\"voter\":") != std::string::npos))
              is_vote = true;

          bool is_post = false;
          if ((message.find("\"comment\":") != std::string::npos)
                   && (message.find("\"author\":") != std::string::npos))
              is_post = true;

          // only read disk file while needed to improve the performance
          if(is_transfer_weku || is_transfer_power || is_vote) {
              std::vector<std::string> bad_guys;
              std::ifstream infile;
              try {
                  infile.open("./blacklist.txt");
                  string name;
                  while (getline(infile, name))
                      bad_guys.push_back(name);
              } catch (...) {} // if no blacklist file, ignore it.

              string blocked_message = "blocked account";

              if (is_transfer_weku || is_transfer_power) {
                  for (auto it = bad_guys.cbegin(); it != bad_guys.cend(); it++)
                      if (message.find("\"from\":\"" + *it + "\"") != std::string::npos)
                          return blocked_message;
              }

              if (is_vote) {
                  for (auto it = bad_guys.cbegin(); it != bad_guys.cend(); it++)
                      if (message.find("\"voter\":\"" + *it + "\"") != std::string::npos)
                          return blocked_message;
              }

              if (is_post) {
                  for (auto it = bad_guys.cbegin(); it != bad_guys.cend(); it++)
                      if (message.find("\"author\":\"" + *it + "\"") != std::string::npos)
                          return blocked_message;
              }
          }

         auto call = var.as<fc::rpc::request>();
         exception_ptr optexcept;
         try
         {
            try
            {
#ifdef LOG_LONG_API
               auto start = time_point::now();
#endif

               auto result = _rpc_state.local_call( call.method, call.params );

#ifdef LOG_LONG_API
               auto end = time_point::now();

               if( end - start > fc::milliseconds( LOG_LONG_API_MAX_MS ) )
                  elog( "API call execution time limit exceeded. method: ${m} params: ${p} time: ${t}", ("m",call.method)("p",call.params)("t", end - start) );
               else if( end - start > fc::milliseconds( LOG_LONG_API_WARN_MS ) )
                  wlog( "API call execution time nearing limit. method: ${m} params: ${p} time: ${t}", ("m",call.method)("p",call.params)("t", end - start) );
#endif

               if( call.id )
               {
                  auto reply = fc::json::to_string( response( *call.id, result ) );
                  if( send_message )
                     _connection.send_message( reply );
                  return reply;
               }
            }
            FC_CAPTURE_AND_RETHROW( (call.method)(call.params) )
         }
         catch ( const fc::exception& e )
         {
            if( call.id )
            {
               optexcept = e.dynamic_copy_exception();
            }
         }
         if( optexcept ) {

               auto reply = fc::json::to_string( response( *call.id,  error_object{ 1, optexcept->to_detail_string(), fc::variant(*optexcept)}  ) );
               if( send_message )
                  _connection.send_message( reply );

               return reply;
         }
      }
      else
      {
         auto reply = var.as<fc::rpc::response>();
         _rpc_state.handle_reply( reply );
      }
   }
   catch ( const fc::exception& e )
   {
      wdump((e.to_detail_string()));
      return e.to_detail_string();
   }
   return string();
}

} } // namespace fc::rpc
```
