#ifndef CONCURRENCPP_IO_CONSTS_H
#define CONCURRENCPP_IO_CONSTS_H

namespace concurrencpp::details::consts {
    /*
        io_engine
    */

    inline const char* k_io_engine_shutdown_err_msg = "concurrencpp::io_engine - io engine has been shut down.";

    /*
        memory_stream
    */

    inline const char* k_memory_stream_read_nullptr_dst =
        "concurrencpp::memory_stream::read - count is bigger than 0 but dst is null.";

    inline const char* k_memory_stream_write_nullptr_src =
        "concurrencpp::memory_stream::write - count is bigger than 0 but src is null.";

    /*
        file_stream
    */
    inline const char* k_file_stream_constructor_null_io_engine_msg =
        "concurrencpp::file_stream::file_stream() - given io engine is null.";

    inline const char* k_file_stream_constructor_empty_file_path_msg =
        "concurrencpp::file_stream::file_stream() - given file path is empty.";

    inline const char* k_file_stream_empty_get_eof_err_msg = "concurrencpp::file_stream::eof_reached() - file stream is empty.";

    inline const char* k_file_stream_get_eof_null_executor_err_msg =
        "concurrencpp::file_stream::eof_reached() - given resume executor is null.";

    inline const char* k_file_stream_empty_get_path_err_msg = "concurrencpp::file_stream::get_path() - file stream is empty.";

    inline const char* k_file_stream_empty_get_mode_err_msg = "concurrencpp::file_stream::get_mode() - file stream is empty.";

    inline const char* k_file_stream_empty_seek_read_pos_err_msg =
        "concurrencpp::file_stream::seek_read_pos() - file stream is empty.";

    inline const char* k_file_stream_seek_read_pos_null_resume_executor_err_msg =
        "concurrencpp::file_stream::seek_read_pos() - given resume executor is null.";

    inline const char* k_file_stream_seek_read_pos_new_pos_negative_err_msg =
        "concurrencpp::file_stream::seek_read_pos() - new read position will become negative";

    inline const char* k_file_stream_seek_read_pos_new_pos_transcend_err_msg =
        "concurrencpp::file_stream::seek_read_pos() - new read position will transcend file size.";

    inline const char* k_file_stream_empty_get_read_pos_err_msg = "concurrencpp::file_stream::get_read_pos() - file stream is empty.";

    inline const char* k_file_stream_get_read_pos_null_resume_executor_err_msg =
        "concurrencpp::file_stream::get_read_pos() - given resume executor is null.";

    inline const char* k_file_stream_empty_seek_write_pos_err_msg =
        "concurrencpp::file_stream::seek_write_pos() - file stream is empty.";

    inline const char* k_file_stream_seek_write_pos_null_resume_executor_err_msg =
        "concurrencpp::file_stream::seek_write_pos() - given resume executor is null.";

    inline const char* k_file_stream_seek_write_pos_new_write_pos_negative_err_msg =
        "concurrencpp::file_stream::seek_write_pos() - new write position will become negative";

    inline const char* k_file_stream_empty_get_write_pos_err_msg =
        "concurrencpp::file_stream::get_write_pos() - file stream is empty.";

    inline const char* k_file_stream_get_write_pos_null_resume_executor_err_msg =
        "concurrencpp::file_stream::get_write_pos() - given resume executor is null.";

    inline const char* k_file_stream_empty_read_err_msg = "concurrencpp::file_stream::read() - file stream is empty.";

    inline const char* k_file_stream_read_null_resume_executor_err_msg =
        "concurrencpp::file_stream::read() - given resume executor is null.";

    inline const char* k_file_stream_read_invalid_buffer_err_msg =
        "concurrencpp::file_stream::read() - given buffer is null while count != 0.";

    inline const char* k_file_stream_read_invalid_stop_token_err_msg =
        "concurrencpp::file_stream::read() - given stop_token is invalid.";

    inline const char* k_file_stream_read_io_engine_shutdown_err_msg =
        "concurrencpp::file_stream::read() - io engine has been shut down";

    inline const char* k_file_stream_read_io_cancelled_err_msg = "concurrencpp::file_stream::read() - io operation was cancelled";

    inline const char* k_file_stream_empty_write_err_msg = "concurrencpp::file_stream::write() - file stream is empty.";

    inline const char* k_file_stream_write_null_resume_executor_err_msg =
        "concurrencpp::file_stream::write() - given resume executor is null.";

    inline const char* k_file_stream_write_invalid_buffer_err_msg =
        "concurrencpp::file_stream::write() - given buffer is null while count != 0.";

    inline const char* k_file_stream_write_invalid_stop_token_err_msg =
        "concurrencpp::file_stream::write() - given stop_token is invalid.";

    inline const char* k_file_stream_write_engine_shutdown_err_msg = "concurrencpp::file_stream_::write() - engine has been shut down";

    inline const char* k_file_stream_write_io_cancelled_err_msg = "concurrencpp::file_stream::write() - io was cancelled.";

    inline const char* k_file_stream_empty_flush_err_msg = "concurrencpp::file_stream::flush() - file stream is empty.";

    inline const char* k_file_stream_flush_null_resume_executor_err_msg =
        "concurrencpp::file_stream::flush() - given resume executor is null.";

    inline const char* k_file_stream_flush_engine_shutdown_err_msg = "concurrencpp::file_stream_::flush() - engine has been shut down";

    /*
        socket
    */

    inline const char* k_socket_read_empty_socket_err_msg = "concurrencpp::socket::read() - socket is empty.";

    inline const char* k_socket_read_null_resume_executor_err_msg = "concurrencpp::socket::read() - given resume_executor is null.";

    inline const char* k_socket_read_bad_stop_token_err_msg = "concurrencpp::socket::read() - bad stop_token.";

    inline const char* k_socket_read_engine_shutdown_err_msg = "concurrencpp::socket::read() - io_engine has been shut down.";

    inline const char* k_socket_read_invalid_buffer_err_msg = "concurrencpp::socket::read() - given buffer is null while count != 0.";

    inline const char* k_socket_write_empty_socket_err_msg = "concurrencpp::socket::write() - socket is empty.";

    inline const char* k_socket_write_null_resume_executor_err_msg = "concurrencpp::socket::write() - resume_executor is null.";

    inline const char* k_socket_write_bad_stop_token_error_msg = "concurrencpp::socket::write() - bad stop_token.";

    inline const char* k_socket_write_engine_shutdown_err_msg = "concurrencpp::socket::write() - io_engine has been shut down.";

    inline const char* k_socket_write_invalid_buffer_err_msg =
        "concurrencpp::socket::write() - given buffer is null while count != 0.";

    inline const char* k_socket_get_address_family_empty_socket_err_msg =
        "concurrencpp::socket::get_address_family() - socket is empty.";

    inline const char* k_socket_get_socket_type_empty_socket_err_msg = "concurrencpp::socket::get_socket_type() - socket is empty.";

    inline const char* k_socket_get_protocol_type_empty_socket_err_msg =
        "concurrencpp::socket::get_protocol_type() - socket is empty.";

    inline const char* k_socket_eof_reached_empty_socket_err_msg = "concurrencpp::socket::eof_reached() - socket is empty.";

    inline const char* k_socket_eof_reached_null_resume_executor_err_msg =
        "concurrencpp::socket::eof_reached() - resume_executor is null.";

    inline const char* k_socket_connect_empty_socket_err_msg = "concurrencpp::socket::connect() - socket is empty.";

    inline const char* k_socket_connect_null_resume_executor_err_msg =
        "concurrencpp::socket::connect() - given resume_executor is null.";

    inline const char* k_socket_connect_bad_stop_token_err_msg = "concurrencpp::socket::connect() - bad stop_token.";

    inline const char* k_socket_connect_engine_shutdown_err_msg = "concurrencpp::socket::connect() - io_engine has been shut down.";

    inline const char* k_socket_listen_empty_socket_err_msg = "concurrencpp::socket::listen() - socket is empty.";

    inline const char* k_socket_listen_null_resume_executor_err_msg = "concurrencpp::socket::listen() - resume_executor is null.";

    inline const char* k_socket_accept_empty_socket_err_msg = "concurrencpp::socket::accept() - socket is empty.";

    inline const char* k_socket_accept_null_resume_executor_err_msg =
        "concurrencpp::socket::accept() - given resume_executor is null.";

    inline const char* k_socket_accept_bad_stop_token_err_msg = "concurrencpp::socket::accept() - bad stop_token.";

    inline const char* k_socket_accept_engine_shutdown_err_msg = "concurrencpp::socket::accept() - io engine has been shut down.";

    inline const char* k_socket_bind_empty_socket_err_msg = "concurrencpp::socket::bind() - socket is empty.";

    inline const char* k_socket_bind_null_resume_executor_err_msg = "concurrencpp::socket::bind() - given resume_executor is null.";

    inline const char* k_socket_local_endpoint_empty_socket_err_msg = "concurrencpp::socket::local_endpoint() - socket is empty.";

    inline const char* k_socket_local_endpoint_null_resume_executor_err_msg =
        "concurrencpp::socket::local_endpoint() - given resume_executor is null.";

    inline const char* k_socket_remote_endpoint_empty_socket_err_msg = "concurrencpp::socket::remote_endpoint() - socket is empty.";

    inline const char* k_socket_remote_endpoint_null_resume_executor_err_msg =
        "concurrencpp::socket::remote_endpoint() - given resume_executor is null.";

    inline const char* k_socket_available_bytes_empty_socket_err_msg = "concurrencpp::socket::available_bytes() - socket is empty.";

    inline const char* k_socket_available_bytes_null_resume_executor_err_msg =
        "concurrencpp::socket::available_bytes() - given resume_executor is null.";

    inline const char* k_socket_keep_alive_empty_socket_err_msg = "concurrencpp::socket::keep_alive() - socket is empty.";

    inline const char* k_socket_keep_alive_null_resume_executor_err_msg =
        "concurrencpp::socket::keep_alive() - given resume_executor is null.";

    inline const char* k_socket_broadcast_enabled_empty_socket_err_msg =
        "concurrencpp::socket::broadcast_enabled() - socket is empty.";

    inline const char* k_socket_broadcast_enabled_null_resume_executor_err_msg =
        "concurrencpp::socket::broadcast_enabled() - given resume_executor is null.";

    inline const char* k_socket_reuse_port_empty_socket_err_msg = "concurrencpp::socket::reuse_port() - socket is empty.";

    inline const char* k_socket_reuse_port_null_resume_executor_err_msg =
        "concurrencpp::socket::reuse_port() - given resume_executor is null.";

    inline const char* k_socket_linger_mode_empty_socket_err_msg = "concurrencpp::socket::linger_mode() - socket is empty.";

    inline const char* k_socket_linger_mode_null_resume_executor_err_msg =
        "concurrencpp::socket::linger_mode() - given resume_executor is null.";

    inline const char* k_socket_no_delay_empty_socket_err_msg = "concurrencpp::socket::no_delay() - socket is empty.";

    inline const char* k_socket_no_delay_null_resume_executor_err_msg =
        "concurrencpp::socket::no_delay() - given resume_executor is null.";

    inline const char* k_socket_receive_buffer_size_empty_socket_err_msg =
        "concurrencpp::socket::receive_buffer_size() - socket is empty.";

    inline const char* k_socket_receive_buffer_size_null_resume_executor_err_msg =
        "concurrencpp::socket::receive_buffer_size() - given resume_executor is null.";

    inline const char* k_socket_send_buffer_size_empty_socket_err_msg = "concurrencpp::socket::send_buffer_size() - socket is empty.";

    inline const char* k_socket_send_buffer_size_null_resume_executor_err_msg =
        "concurrencpp::socket::send_buffer_size() - given resume_executor is null.";

    inline const char* k_socket_receive_timeout_empty_socket_err_msg = "concurrencpp::socket::receive_timeout() - socket is empty.";

    inline const char* k_socket_receive_timeout_null_resume_executor_err_msg =
        "concurrencpp::socket::receive_timeout() - given resume_executor is null.";

    inline const char* k_socket_send_timeout_empty_socket_err_msg = "concurrencpp::socket::send_timeout() - socket is empty.";

    inline const char* k_socket_send_timeout_null_resume_executor_err_msg =
        "concurrencpp::socket::send_timeout() - given resume_executor is null.";

    inline const char* k_socket_dont_fragment_empty_socket_err_msg = "concurrencpp::socket::dont_fragment() - socket is empty.";

    inline const char* k_socket_dont_fragment_null_resume_executor_err_msg =
        "concurrencpp::socket::dont_fragment() - given resume_executor is null.";
   
    inline const char* k_socket_ttl_empty_socket_err_msg = "concurrencpp::socket::ttl() - socket is empty.";

    inline const char* k_socket_ttl_null_resume_executor_err_msg =
        "concurrencpp::socket::ttl() - given resume_executor is null.";

    inline const char* k_socket_multicast_loopback_empty_socket_err_msg = "concurrencpp::socket::multicast_loopback() - socket is empty.";

    inline const char* k_socket_multicast_loopback_null_resume_executor_err_msg =
        "concurrencpp::socket::multicast_loopback() - given resume_executor is null.";

    inline const char* k_socket_receive_from_empty_socket_err_msg = "concurrencpp::socket::receive_from() - socket is empty.";

    inline const char* k_socket_receive_from_null_resume_executor_err_msg =
        "concurrencpp::socket::receive_from() - given resume_executor is null.";

    inline const char* k_socket_receive_from_bad_stop_token_err_msg = "concurrencpp::socket::receive_from() - bad stop_token.";

    inline const char* k_socket_receive_from_engine_shutdown_err_msg = "concurrencpp::socket::receive_from() - io_engine has been shut down.";

    inline const char* k_socket_receive_from_invalid_buffer_err_msg =
        "concurrencpp::socket::receive_from() - given buffer is null while count != 0.";

    inline const char* k_socket_send_to_empty_socket_err_msg = "concurrencpp::socket::send_to() - socket is empty.";

    inline const char* k_socket_send_to_null_resume_executor_err_msg = "concurrencpp::socket::send_to() - resume_executor is null.";

    inline const char* k_socket_send_to_bad_stop_token_error_msg = "concurrencpp::socket::send_to() - bad stop_token.";

    inline const char* k_socket_send_to_engine_shutdown_err_msg = "concurrencpp::socket::send_to() - io_engine has been shut down.";

    inline const char* k_socket_send_to_invalid_buffer_err_msg =
        "concurrencpp::socket::send_to() - given buffer is null while count != 0.";

}  // namespace concurrencpp::details::consts

#endif