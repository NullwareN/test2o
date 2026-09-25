#!/bin/bash
# Per-bot firejail network namespace via a local bridge, NATed out the host NordVPN
# interface. Isolates Steam abstract unix sockets without bypassing the VPN.

set -u

bridge="${CAT_BOT_BRIDGE:-catbotbr}"
bridge_addr="${CAT_BOT_BRIDGE_ADDR:-10.249.0.1/24}"
bridge_net="${CAT_BOT_BRIDGE_NET:-10.249.0.0/24}"
vpn_if="${CATHOOK_NET_INTERFACE:-}"
if [ -z "$vpn_if" ] || ! ip link show "$vpn_if" >/dev/null 2>&1; then
	vpn_if="$(ip -4 route show default 2>/dev/null | awk '{for(i=1;i<=NF;i++) if($i=="dev") print $(i+1)}' | head -1)"
fi

if [ -z "$vpn_if" ] || ! ip link show "$vpn_if" >/dev/null 2>&1; then
	echo "[net] missing network interface; not creating $bridge"
	exit 0
fi

if ! ip link show "$bridge" >/dev/null 2>&1; then
	ip link add "$bridge" type bridge
fi
if ! ip addr show dev "$bridge" | grep -q " ${bridge_addr%%/*}/"; then
	ip addr add "$bridge_addr" dev "$bridge" 2>/dev/null || true
fi
ip link set "$bridge" up
sysctl -w net.ipv4.ip_forward=1 >/dev/null

iptables -t nat -C POSTROUTING -s "$bridge_net" -o "$vpn_if" -j MASQUERADE 2>/dev/null ||
	iptables -t nat -A POSTROUTING -s "$bridge_net" -o "$vpn_if" -j MASQUERADE
iptables -C FORWARD -i "$bridge" -o "$vpn_if" -j ACCEPT 2>/dev/null ||
	iptables -I FORWARD -i "$bridge" -o "$vpn_if" -j ACCEPT
iptables -C FORWARD -i "$vpn_if" -o "$bridge" -m state --state RELATED,ESTABLISHED -j ACCEPT 2>/dev/null ||
	iptables -I FORWARD -i "$vpn_if" -o "$bridge" -m state --state RELATED,ESTABLISHED -j ACCEPT

echo "[net] $bridge $bridge_addr -> $vpn_if"
