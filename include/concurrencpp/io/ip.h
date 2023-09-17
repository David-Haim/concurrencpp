#ifndef CONCURRENCPP_IP_H
#define CONCURRENCPP_IP_H

#include <array>
#include <span>
#include <variant>
#include <string_view>

namespace concurrencpp {
    class ip_v4 {

       private:
        uint32_t m_address = 0;

        static uint32_t make_address(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) noexcept;

       public:
        ip_v4() noexcept = default;
        ip_v4(const ip_v4& rhs) noexcept = default;

        ip_v4& operator=(const ip_v4&) noexcept = default;

        ip_v4(std::string_view ip_string);
        ip_v4(std::span<const std::byte, 4> bytes) noexcept;
        ip_v4(uint32_t address) noexcept;

        std::array<std::byte, 4> bytes() const noexcept;
        uint32_t address() const noexcept;

        bool is_loopback() const noexcept;
        bool is_broadcast() const noexcept;
        bool is_multicast() const noexcept;
        bool is_unspecified() const noexcept;

        //		ip_v6 map_to_v6() const;

        std::string to_string() const noexcept;

        bool operator==(const ip_v4&) const = default;
        bool operator!=(const ip_v4&) const = default;

       public:
        static const ip_v4 any;
        static const ip_v4 loop_back;
        static const ip_v4 broadcast;
        static const ip_v4 none;
    };

    class ip_v6 {

       private:
        std::array<std::uint8_t, 16> m_bytes;
        std::uint32_t m_scope_id = 0;

        static const ip_v6 s_ipv4_mapped_loopback;

        ip_v6(const std::array<std::uint8_t, 16>& bytes, std::uint32_t scope_id = 0) noexcept;

        std::uint16_t short_at(size_t index) const noexcept;

       public:
        ip_v6() noexcept = default;
        ip_v6(const ip_v6& rhs) noexcept = default;

        ip_v6& operator=(const ip_v6&) noexcept = default;

        ip_v6(std::span<const std::byte, 16> bytes, std::uint32_t scope_id = 0) noexcept;
        ip_v6(std::string_view ip_string);

        std::array<std::byte, 16> bytes() const noexcept;
        std::uint32_t scope_id() const noexcept;

        bool is_loopback() const noexcept;
        bool is_multicast() const noexcept;
        bool is_link_local() const noexcept;
        bool is_site_local() const noexcept;
        bool is_unspecified() const noexcept;
        bool is_v4_mapped() const noexcept;
        bool is_v4_compatible() const noexcept;

        ip_v4 map_to_v4() const;

        std::string to_string() const noexcept;

        bool operator==(const ip_v6&) const;
        bool operator!=(const ip_v6&) const;

        static const ip_v6 any;
        static const ip_v6 loop_back;
        static const ip_v6 none;
    };

    struct ip_endpoint {
        std::variant<ip_v4, ip_v6> address;
        uint16_t port;
    };
}  // namespace concurrencpp

#endif