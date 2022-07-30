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

IPaddress* ip;
UDPsocket udpsocket;
UDPpacket* packet;
SDLNet_SocketSet set;
Uint16 port;
cvar_t hostname;
Uint32 available;
static int net_acceptsocket = -1;		// socket for fielding new connections
static int net_controlsocket;
static int net_broadcastsocket = 0;
int channel;

#define MAX_NAME 3000

#ifdef DEBUG_SDLNET_DEVELOPMENT
void SDL_UDP_PrintVersion(void);

void SDL_UDP_PrintVersion(void)
{
	
#if SDL_NET_MAJOR_VERSION == 1
	SDL_version version;
	SDL_version Linked;
	SDL_NET_VERSION(&version);
	SDLNet_Linked_Version();
	Con_Printf("compiled with SDL_net version: %d.%d.%d\n",
		version.major, version.minor, version.patch);
	Con_Printf("compiled with SDL_net link_version: %d.%d.%d\n",
		Linked.major, Linked.minor, Linked.patch);

#elif SDL_NET_MAJOR_VERSION == 2
	SDLNet_version version;
	const SDLNet_version* link_version = SDLNet_Linked_Version();
	SDL_NET_VERSION(&version);
	Con_Printf("compiled with SDL_net version: %d.%d.%d\n",
		version.major, version.minor, version.patch);
	Con_Printf("compiled with SDL_net link_version: %d.%d.%d\n",
		link_version->major, link_version->minor, link_version->patch);
#endif
}
#endif

int UDP_Init(void)
{
	char log[MAX_NAME];

	if (COM_CheckParm("-noudp"))
		return -1;

#ifdef DEBUG_SDLNET_DEVELOPMENT
	SDL_UDP_PrintVersion();
#endif
	
	int ret;
	ret = SDLNet_Init();
	if(ret < 0)
	{
		Con_Printf("SDLNet failed to start: %s", SDLNet_GetError());	
	}

	SDLNet_UDP_AddSocket(NULL, udpsocket);

	//Get Host
	SDLNet_ResolveHost(ip, log, ip->host);
	SDLNet_UDP_Bind(udpsocket, channel, ip);
	

	// if the quake hostname isn't set, set it to the machine name
	if(Q_strcmp(hostname.string, "UNNAMED") == 0)
	{
		log[15] = 0;
		Cvar_Set("hostname", log);
	}

	ret = SDLNet_UDP_Open(port);
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
	SDLNet_DelSocket(NULL, udpsocket);
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

	int ret = SDLNet_CheckSockets(NULL, available);

	if(ret == -1)
	{
		Con_Printf("No connections found: %d", SDLNet_GetError());
	}

	else
	{
		SDLNet_AllocSocketSet(available);
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


}

int UDP_StringToAddr(char* string, struct qsockaddr* addr)
{


}

int UDP_GetSocketAddr(int socket, struct qsockaddr* addr)
{
   
}

int UDP_GetNameFromAddr(struct qsockaddr* addr, char* name)
{


}

int UDP_GetAddrFromName(char* name, struct qsockaddr* addr)
{



}

int UDP_AddrCompare(struct qsockaddr* addr1, struct qsockaddr* addr2)
{

}

int UDP_GetSocketPort(struct qsockaddr* addr)
{
	if(addr)
	{
	//Wip..
	}
}

int UDP_SetSocketPort(struct qsockaddr* addr, int port)
{

}
