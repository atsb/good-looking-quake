/*
Copyright (C) 2022 André Guilherme 

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

NOTE:
 This file is heavily based on net_udp.c original 
 and some stuff should not work properly beacuse i not familiar with SDLNet stuff
*/

#include "quakedef.h"
#include <SDL_net.h>
#include <string.h>

IPaddress* ip;
UDPsocket udpsocket;
UDPpacket* packet;
SDLNet_SocketSet set;
Uint16 port;
cvar_t hostname;
Uint32 available;
Uint32 Available_sockets;
static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
int channel;

#define MAX_NAME 3000

int UDP_Init(void)
{
	char log[MAX_NAME];

	if (COM_CheckParm("-noudp"))
		return -1;
	
	int ret;
	ret = SDLNet_Init();
	if(ret < 0)
	{
		Con_Printf("SDLNet failed to start: %s", SDLNet_GetError());	
	}

	SDLNet_UDP_AddSocket(set, udpsocket);

	//Get Host and port
	SDLNet_ResolveHost(ip->port, log, ip->host);
	SDLNet_UDP_Bind(udpsocket, channel, ip->host);
	
	// if the quake hostname isn't set, set it to the machine name
	if(Q_strcmp(hostname.string, "UNNAMED") == 0)
	{
		log[15] = 0;
		Cvar_Set("hostname", log);
	}

	ret = UDP_OpenSocket(port);
	if(ret == -1)
	{
		Sys_Error("UDP_Init: Unable to open control socket\n");
	}

	Con_Printf("UDP Initialized");
	tcpipAvailable = true;
	
	return;
}

void UDP_Shutdown(void)
{
	SDLNet_Quit();
	SDLNet_FreePacket(packet);
	SDLNet_UDP_Unbind(udpsocket, channel);
	SDLNet_UDP_Close(udpsocket);
	SDLNet_DelSocket(set, udpsocket);
	SDLNet_FreeSocketSet(set);
}

void UDP_Listen(qboolean state)
{
	if (state)
	{
		if (net_acceptsocket != -1)
			return;
		if ((net_acceptsocket = UDP_OpenSocket(net_hostport) == -1))
			Sys_Error("UDP_Listen: Unable to open accept socket\n");
		return;
	}
	if (net_acceptsocket == -1)
		return;
	SDLNet_UDP_Close(udpsocket);
	udpsocket = NULL;
}

int UDP_OpenSocket(int port)
{
	return SDLNet_UDP_Open(port);
}

int UDP_CloseSocket(int socket)
{
	if (socket == net_broadcastsocket)
	{
		net_broadcastsocket = 0;
		SDLNet_UDP_Close(socket);
	}
	return socket;
}

void get_qsockaddr(struct qsockaddr *saddr, struct qsockaddr *qaddr)
{
	//adress and data
	qaddr->sa_family = saddr->sa_family;
	sizeof(qaddr->sa_family, saddr->sa_family);
	SDL_memcpy(&(qaddr->sa_data), &(saddr->sa_data), sizeof(saddr->sa_data, qaddr->sa_data));
}

int UDP_Connect(int socket, struct qsockaddr* addr)
{
	return 0;
}

int UDP_CheckNewConnections(void)
{
	if (net_acceptsocket == -1)
		return -1;

	int ret = SDLNet_CheckSockets(set, available);

	if(ret == -1)
	{
		Con_Printf("No connections found: %d", SDLNet_GetError());
	}

	else
	{
		SDLNet_AllocSocketSet(Available_sockets);
		SDLNet_SocketReady(udpsocket);
	}

	if (available)
		return net_acceptsocket;

	return -1;
}

int UDP_Read(int socket, byte* buf, int len, struct qsockaddr* addr)
{
	static struct qsockaddr *saddr;
	socket = saddr;
	unsigned int addrlen = sizeof(UDPsocket);
	int	ret;

	ret = SDLNet_UDP_Recv(udpsocket, packet);
	if (ret == -1 && (errno == EWOULDBLOCK || errno == ECONNREFUSED))
	{
		Con_Printf("Failed to recive the socket and packet");
		return 0;
	}
	get_qsockaddr(socket, addr);
	return ret;
}

int UDP_Write(int socket, byte* buf, int len, struct qsockaddr* addr)
{
	int ret;
	ret = SDLNet_UDP_Send(udpsocket, channel, packet);

	if (ret == -1 && errno == EWOULDBLOCK)
		return 0;
	return ret;
}

int UDP_Broadcast(int socket, byte* buf, int len)
{
	//WIP

}

char* UDP_AddrToString(struct qsockaddr* addr)
{
	//WIP SDLNet_GetLocalAddresses(ip, addr);

}

int UDP_StringToAddr(char* string, struct qsockaddr* addr)
{
	//WIP
}

int UDP_GetSocketAddr(int socket, struct qsockaddr* addr)
{
   //WIP
}

int UDP_GetNameFromAddr(struct qsockaddr* addr, char* name)
{
	//WIP
}

int UDP_GetAddrFromName(char* name, struct qsockaddr* addr)
{
	//WIP
}

int UDP_AddrCompare(struct qsockaddr* addr1, struct qsockaddr* addr2)
{
	if(addr1)
	{
		SDLNet_GetLocalAddresses(ip->host, available);
		SDLNet_UDP_GetPeerAddress(udpsocket, NULL);
		memcpy(udpsocket, addr1, sizeof(addr1));
	}
	else if(addr2)
	{
		SDLNet_GetLocalAddresses(ip->host, available);
		SDLNet_UDP_GetPeerAddress(udpsocket, NULL);
		memcpy(udpsocket, addr2, sizeof(addr2));
	}
	else
	{
		Con_Printf("Nothing to compare");
	}
	return;
}

int UDP_GetSocketPort(struct qsockaddr* addr)
{
	ip->port = addr;
	SDLNet_UDP_AddSocket(set, udpsocket);
	SDLNet_UDP_Open(addr);
}

int UDP_SetSocketPort(struct qsockaddr* addr, int port)
{
	UDP_GetSocketPort(port);
	SDLNet_UDP_Bind(udpsocket, available, ip->port);
		
	if (addr->sa_data) 
	{
		SDLNet_UDP_Recv(udpsocket, packet->data);	
	}
	else
	{
		SDLNet_UDP_Send(udpsocket, available, packet->status);
	}

	return ip->port;
}
