package main

import (
	"sync"
)

type Peer struct {
	Hostname string
	Address  string
}

type PeerManager struct {
	peers map[string]Peer
}

func NewPeerManager() *PeerManager {
	return &PeerManager{
		peers: make(map[string]Peer),
	}
}

type SafePeerManager struct {
	pm *PeerManager
	mu sync.RWMutex
}

func NewSafePeerManager() *SafePeerManager {
	return &SafePeerManager{
		pm: NewPeerManager(),
	}
}

func (spm *SafePeerManager) AddPeer(hostname, address string) {
	spm.mu.Lock()
	defer spm.mu.Unlock()
	spm.pm.peers[hostname] = Peer{Hostname: hostname, Address: address}
}

func (spm *SafePeerManager) RemovePeer(hostname string) {
	spm.mu.Lock()
	defer spm.mu.Unlock()
	delete(spm.pm.peers, hostname)
}

func (spm *SafePeerManager) GetPeers() []Peer {
	spm.mu.RLock()
	defer spm.mu.RUnlock()
	peers := make([]Peer, 0, len(spm.pm.peers))
	for _, peer := range spm.pm.peers {
		peers = append(peers, peer)
	}
	return peers
}

func (spm *SafePeerManager) GetPeer(hostname string) (Peer, bool) {
	spm.mu.RLock()
	defer spm.mu.RUnlock()
	peer, ok := spm.pm.peers[hostname]
	return peer, ok
}
