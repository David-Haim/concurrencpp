#ifndef ASYNCIO_SOCKET_CONSTANTS_H
#define ASYNCIO_SOCKET_CONSTANTS_H

namespace concurrencpp::io
{
	enum class address_family
	{
		ip_v4,
		ip_v6
	};

	enum class socket_type
	{
		raw,
		datagram,
		stream
	};

	enum class protocol_type
	{
		tcp,
		udp,
		icmp,
		icmp_v6
	};
}

#endif