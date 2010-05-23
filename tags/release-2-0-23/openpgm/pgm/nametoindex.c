/* vim:ts=8:sts=8:sw=4:noai:noexpandtab
 *
 * Windows interface name to interface index function.
 *
 * Copyright (c) 2006-2010 Miru Limited.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <glib.h>

#ifdef G_OS_WIN32
#	include <ws2tcpip.h>
#	include <iphlpapi.h>
#endif

#include "pgm/nametoindex.h"



//#define NAMETOINDEX_DEBUG

#ifndef NAMETOINDEX_DEBUG
#define g_trace(...)		while (0)
#else
#define g_trace(...)		g_debug(__VA_ARGS__)
#endif


#ifdef G_OS_WIN32

/* Retrieve interface index for a specified adapter name.
 *
 * On Windows is IPv4 specific, IPv6 indexes are separate (IP_ADAPTER_ADDRESSES::Ipv6IfIndex)
 *
 * On error returns zero, no errors are defined.
 */

int
pgm_if_nametoindex (
	const int		iffamily,
	const char*		ifname
        )
{
	g_return_val_if_fail (NULL != ifname, 0);

#ifdef CONFIG_TARGET_WINE
	g_assert (AF_INET6 != iffamily);

	ULONG ifIndex = 0;
	DWORD dwSize, dwRet;
	MIB_IFTABLE *pIfTable;
	MIB_IFROW *pIfRow;

	dwRet = GetAdapterIndex ((const LPWSTR)ifname, &ifIndex);
	if (NO_ERROR == dwRet)
		return ifIndex;

	pIfTable = (MIB_IFTABLE *) malloc(sizeof (MIB_IFTABLE));
	if (NULL == pIfTable) {
		perror("malloc");
		return 0;
	}
	dwSize = sizeof (MIB_IFTABLE);
	dwRet = GetIfTable(pIfTable, &dwSize, FALSE);
	if (ERROR_INSUFFICIENT_BUFFER == dwRet) {
		free(pIfTable);
		pIfTable = (MIB_IFTABLE *) malloc(dwSize);
		if (NULL == pIfTable) {
			perror("malloc");
			return 0;
		}
	}
	dwRet = GetIfTable(pIfTable, &dwSize, FALSE);
	if (NO_ERROR != dwRet) {
		perror("GetIfTable did not return NO_ERROR");
		return 0;
	}
	for (int i = 0; i < (int) pIfTable->dwNumEntries; i++)
	{
		pIfRow = (MIB_IFROW *) & pIfTable->table[i];
		if (0 == strncmp (ifname, pIfRow->bDescr, pIfRow->dwDescrLen)) {
			ifIndex = pIfRow->dwIndex;
			free(pIfTable);
			return ifIndex;
		}
	}
	free(pIfTable);

#else /* !CONFIG_TARGET_WINE */
	ULONG ifIndex;
	DWORD dwSize, dwRet;
	IP_ADAPTER_ADDRESSES *pAdapterAddresses, *adapter;

	dwRet = GetAdapterIndex ((const LPWSTR)ifname, &ifIndex);
	if (NO_ERROR == dwRet)
		return ifIndex;

/* fallback to finding index via iterating adapter list */
	dwRet = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_MULTICAST, NULL, NULL, &dwSize);
	if (ERROR_BUFFER_OVERFLOW != dwRet) {
		perror("GetAdaptersAddresses");
		return 0;
	}
	pAdapterAddresses = (IP_ADAPTER_ADDRESSES*)malloc (dwSize);
	if (NULL == pAdapterAddresses) {
		perror("malloc");
		return 0;
	}
	dwRet = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_FRIENDLY_NAME | GAA_FLAG_SKIP_MULTICAST, NULL, pAdapterAddresses, &dwSize);
	if (ERROR_SUCCESS != dwRet) {
		perror("GetAdaptersAddresses(2)");
		free(pAdapterAddresses);
		return 0;
	}

	for (adapter = pAdapterAddresses;
		adapter;
		adapter = adapter->Next)
	{
		if (0 == strcmp (ifname, adapter->AdapterName)) {
			ifIndex = AF_INET6 == iffamily ? adapter->Ipv6IfIndex : adapter->IfIndex;
			free (pAdapterAddresses);
			return ifIndex;
		}
	}

	free (pAdapterAddresses);
#endif
	return 0;
}
#endif /* G_OS_WIN32 */

/* eof */