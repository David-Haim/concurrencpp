#include "concurrencpp/io/ip.h"
#include "concurrencpp/platform_defs.h"
#include "concurrencpp/io/impl/win32/utils.h"

#include <stdexcept>

#include <cassert>

using concurrencpp::ip_v4;
using concurrencpp::ip_v6;

namespace concurrencpp::details {
    std::uint32_t ip_v4_from_str(std::string_view str);
    std::string ip_v4_to_str(std::uint32_t);

    static std::pair<std::array<std::uint8_t, 16>, std::uint32_t> ip_v6_from_str(std::string_view str);
    static std::string ip_v6_to_str(const std::uint8_t* address, std::uint32_t scope_id);
}

/*
    ip_v4
*/

const ip_v4 ip_v4::any(0);
const ip_v4 ip_v4::loop_back(ip_v4::make_address(127, 0, 0, 1));
const ip_v4 ip_v4::broadcast(ip_v4::make_address(255, 255, 255, 255));
const ip_v4 ip_v4::none = broadcast;

ip_v4::ip_v4(std::span<const std::byte, 4> bytes) noexcept :
    m_address(make_address(static_cast<uint8_t>(bytes[0]),
                           static_cast<uint8_t>(bytes[1]),
                           static_cast<uint8_t>(bytes[2]),
                           static_cast<uint8_t>(bytes[3]))) {}

ip_v4::ip_v4(uint32_t address) noexcept : m_address(address) {}

uint32_t ip_v4::make_address(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) noexcept {
    return static_cast<std::uint32_t>(b0) << 24 | static_cast<std::uint32_t>(b1) << 16 | static_cast<std::uint32_t>(b2) << 8 |
        static_cast<std::uint32_t>(b3);
}

ip_v4::ip_v4(std::string_view ip_string) {
    m_address = details::ip_v4_from_str(ip_string);
}

std::array<std::byte, 4> ip_v4::bytes() const noexcept {
    return std::array<std::byte, 4> {static_cast<std::byte>(m_address >> 24),
                                     static_cast<std::byte>(m_address >> 16),
                                     static_cast<std::byte>(m_address >> 8),
                                     static_cast<std::byte>(m_address)};
}

uint32_t ip_v4::address() const noexcept {
    return m_address;
}

bool ip_v4::is_loopback() const noexcept {
    const auto address_bytes = bytes();
    return static_cast<uint8_t>(address_bytes[0]) == 172;
}

bool ip_v4::is_broadcast() const noexcept {
    return m_address == broadcast.address();
}

bool ip_v4::is_multicast() const noexcept {
    const auto address_bytes = bytes();
    return (static_cast<std::uint8_t>(address_bytes[0]) & 0xF0) == 0XE0;
}

bool ip_v4::is_unspecified() const noexcept {
    return m_address == any.address();
}

std::string ip_v4::to_string() const noexcept {
    return details::ip_v4_to_str(m_address);
}

/*
    ip_v6
*/

const ip_v6 ip_v6::any(std::array<std::uint8_t, 16> {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0);
const ip_v6 ip_v6::loop_back(std::array<std::uint8_t, 16> {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1}, 0);
const ip_v6 ip_v6::none(std::array<std::uint8_t, 16> {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, 0);
const ip_v6 ip_v6::s_ipv4_mapped_loopback(std::array<std::uint8_t, 16> {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 255, 127, 0, 0, 1}, 0);

ip_v6::ip_v6(const std::array<std::uint8_t, 16>& bytes, std::uint32_t scope_id) noexcept : m_bytes(bytes), m_scope_id(scope_id) {}

std::uint16_t ip_v6::short_at(size_t index) const noexcept {
    assert(index < std::size(m_bytes) / 2);
    return static_cast<std::uint16_t>(m_bytes[index * 2]) * 256 + static_cast<std::uint16_t>(m_bytes[index * 2 + 1]);
}

ip_v6::ip_v6(std::span<const std::byte, 16> bytes, std::uint32_t scope_id) noexcept : m_scope_id(scope_id) {
    for (size_t i = 0; i < std::size(m_bytes); i++) {
        m_bytes[i] = static_cast<std::uint8_t>(bytes[i]);
    }
}

ip_v6::ip_v6(std::string_view ip_string) {
    std::tie(m_bytes, m_scope_id) = details::ip_v6_from_str(ip_string);
}

std::array<std::byte, 16> ip_v6::bytes() const noexcept {
    std::array<std::byte, 16> bytes = {};
    for (size_t i = 0; i < std::size(m_bytes); i++) {
        bytes[i] = static_cast<std::byte>(m_bytes[i]);
    }

    return bytes;
}

std::uint32_t ip_v6::scope_id() const noexcept {
    return m_scope_id;
}

bool ip_v6::is_loopback() const noexcept {
    return *this == loop_back || *this == s_ipv4_mapped_loopback;
}

bool ip_v6::is_multicast() const noexcept {
    const auto s = short_at(0);
    return (s & 0xFF00) == 0xFF00;
}

bool ip_v6::is_link_local() const noexcept {
    const auto s = short_at(0);
    return (s & 0xFFC0) == 0xFE80;
}

bool ip_v6::is_site_local() const noexcept {
    const auto s = short_at(0);
    return (s & 0xFFC0) == 0xFEC0;
}

bool ip_v6::is_unspecified() const noexcept {
    for (size_t i = 0; i < 16; i++) {
        if (m_bytes[i] != 0) {
            return false;
        }
    }

    return true;
}

bool ip_v6::is_v4_mapped() const noexcept {
    for (size_t i = 0; i < 10; i++) {
        if (m_bytes[i] != 0) {
            return false;
        }
    }

    return m_bytes[10] == 0xFF && m_bytes[11] == 0xFF;
}

bool ip_v6::is_v4_compatible() const noexcept {

    for (size_t i = 0; i < 12; i++) {
        if (m_bytes[i] != 0) {
            return false;
        }
    }

    return true;
}

ip_v4 ip_v6::map_to_v4() const {
    if (!is_v4_mapped() && !is_v4_compatible()) {
        throw std::runtime_error("cannot map to ipv4");
    }

    std::byte bytes[4] {static_cast<std::byte>(m_bytes[12]),
                        static_cast<std::byte>(m_bytes[13]),
                        static_cast<std::byte>(m_bytes[14]),
                        static_cast<std::byte>(m_bytes[15])};

    return ip_v4(bytes);
}

std::string ip_v6::to_string() const noexcept {
    return details::ip_v6_to_str(m_bytes.data(), m_scope_id);
}

bool ip_v6::operator==(const ip_v6& rhs) const {
    return m_bytes == rhs.m_bytes && m_scope_id == rhs.m_scope_id;
}

bool ip_v6::operator!=(const ip_v6& rhs) const {
    return !(*this == rhs);
}

#ifdef CRCPP_WIN_OS

#    include <WinSock2.h>
#    include <ws2tcpip.h>
#    include <Mstcpip.h>
#    include <ntstatus.h>

#    pragma comment(lib, "Ntdll.lib")

std::uint32_t concurrencpp::details::ip_v4_from_str(std::string_view ip_string) {
    ::IN_ADDR address = {};
    const auto res = ::inet_pton(AF_INET, ip_string.data(), &address);

    if (res == 0) {
        throw std::invalid_argument("ip_v4::ip_v4(str) - given string is not a valid ip(v4) address.");
    }

    if (res == -1) {
        throw_system_error(::WSAGetLastError());
    }

    assert(res == 1);
    return address.S_un.S_addr;
}

std::string concurrencpp::details::ip_v4_to_str(std::uint32_t ip_address) {
    ::IN_ADDR address = {};
    address.S_un.S_addr = ip_address;
    char saddr[INET_ADDRSTRLEN];
    const auto res = ::inet_ntop(AF_INET, &address, saddr, INET_ADDRSTRLEN);
    assert(res != nullptr);
    return saddr;
}

std::pair<std::array<std::uint8_t, 16>, std::uint32_t> concurrencpp::details::ip_v6_from_str(std::string_view ip_string) {
    std::array<std::uint8_t, 16> bytes = {};
    ::in6_addr address = {};
    ULONG scope_id = 0;
    USHORT port = 0;

    const auto res = ::RtlIpv6StringToAddressExA(ip_string.data(), &address, &scope_id, &port);
    if (res == STATUS_SUCCESS) {
        for (size_t i = 0; i < bytes.size(); i++) {
            bytes[i] = address.u.Byte[i];
        }

        return {bytes, scope_id};
    }

    if (res == STATUS_INVALID_PARAMETER) {
        throw std::invalid_argument("ip_v6::ip_v6(str) - given string is not a valid ip(v6) address.");
    }

    throw_system_error(res);
}

std::string concurrencpp::details::ip_v6_to_str(const std::uint8_t* address, std::uint32_t scope_id) {
    char str_res[INET6_ADDRSTRLEN + 1];
    ULONG length = std::size(str_res);
    ::in6_addr in_address = {};

    for (size_t i = 0; i < 16; i++) {
        in_address.u.Byte[i] = static_cast<UCHAR>(address[i]);
    }

    const auto res = ::RtlIpv6AddressToStringExA(&in_address, scope_id, 0, str_res, &length);
    assert(res == STATUS_SUCCESS);
    return str_res;
}

#endif