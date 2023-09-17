#ifndef CONCURRENCPP_IO_CONSTS_H
#define CONCURRENCPP_IO_CONSTS_H

namespace concurrencpp::details::consts {
    /*
        io_engine
    */

    inline const char* k_io_engine_shutdown_err_msg =
        "concurrencpp::io_engine - io engine has been shut down.";
    
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

}  // namespace concurrencpp::details::consts

#endif