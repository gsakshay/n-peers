package main

import (
	"bufio"
	"context"
	"flag"
	"fmt"
	"log"
	"net"
	"os"
	"strings"
	"time"
)

type Config struct {
	UDPPort      int
	Message      string
	Timeout      time.Duration
	SendInterval time.Duration
	HostsFile    string
	Debug        bool
	Hostname     string
}

type HeartbeatMessage struct {
	From string
	To   string
}

func parseFlagsAndAssignConstants() Config {
	var config Config
	flag.StringVar(&config.HostsFile, "h", "", "Path to hosts file")
	flag.BoolVar(&config.Debug, "d", false, "Enable debugging mode")
	flag.Parse()

	config.UDPPort = 8000
	config.Message = "HEARTBEAT"
	config.Timeout = 1 * time.Minute
	config.SendInterval = 5 * time.Second

	hostname, err := os.Hostname()
	if err != nil {
		log.Fatalf("Error getting hostname: %v", err)
	}
	config.Hostname = hostname

	return config
}

func readHostsFile(filePath string) ([]string, error) {
	file, err := os.Open(filePath)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	var peers []string
	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line != "" && !strings.HasPrefix(line, "#") {
			peers = append(peers, line)
		}
	}

	if err := scanner.Err(); err != nil {
		return nil, err
	}

	return peers, nil
}

func initializePeerManager(config Config) *SafePeerManager {
	spm := NewSafePeerManager()
	peers, err := readHostsFile(config.HostsFile)
	if err != nil {
		log.Fatalf("Error reading hosts file: %v", err)
	}

	for _, peer := range peers {
		if peer != config.Hostname {
			spm.AddPeer(peer, fmt.Sprintf("%s:%d", peer, config.UDPPort))
		}
	}

	return spm
}

func logDebug(config Config, message string) {
	if config.Debug {
		log.Println(message)
	}
}

func sendHeartbeats(ctx context.Context, config Config, peerManager *SafePeerManager, sendCh chan<- HeartbeatMessage) {
	ticker := time.NewTicker(config.SendInterval)
	defer ticker.Stop()

	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			for _, peer := range peerManager.GetPeers() {
				sendCh <- HeartbeatMessage{From: config.Hostname, To: peer.Hostname}
			}
		}
	}
}

func processHeartbeats(ctx context.Context, config Config, peerManager *SafePeerManager, receiveCh <-chan HeartbeatMessage) {
	readyPeers := make(map[string]bool)
	readyPrinted := false
	for {
		select {
		case <-ctx.Done():
			return
		case msg := <-receiveCh:
			readyPeers[msg.From] = true
			logDebug(config, fmt.Sprintf("Received READY from %s", msg.From))
			if len(readyPeers) == len(peerManager.GetPeers()) && !readyPrinted {
				log.Println("READY")
				readyPrinted = true
			}
		}
	}
}

func listen(ctx context.Context, config Config, receiveCh chan<- HeartbeatMessage) {
	addr, err := net.ResolveUDPAddr("udp", fmt.Sprintf(":%d", config.UDPPort))
	if err != nil {
		log.Fatalf("Error resolving UDP address: %v", err)
	}

	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		log.Fatalf("Error listening: %v", err)
	}
	defer conn.Close()

	for {
		select {
		case <-ctx.Done():
			return
		default:
			buffer := make([]byte, 1024)
			n, remoteAddr, err := conn.ReadFromUDP(buffer)
			if err != nil {
				log.Printf("Error reading from UDP: %v", err)
				continue
			}

			if string(buffer[:n]) == config.Message {
				receiveCh <- HeartbeatMessage{From: remoteAddr.String(), To: config.Hostname}
			}
		}
	}
}

func sendMessage(config Config, peer Peer) {
	addr, err := net.ResolveUDPAddr("udp", peer.Address)
	if err != nil {
		log.Printf("Error resolving address for %s: %v", peer.Hostname, err)
		return
	}

	conn, err := net.DialUDP("udp", nil, addr)
	if err != nil {
		log.Printf("Error dialing %s: %v", peer.Hostname, err)
		return
	}
	defer conn.Close()

	_, err = conn.Write([]byte(config.Message))
	if err != nil {
		log.Printf("Error sending to %s: %v", peer.Hostname, err)
	} else {
		logDebug(config, fmt.Sprintf("Sent READY to %s", peer.Hostname))
	}
}

func main() {
	config := parseFlagsAndAssignConstants()
	peerManager := initializePeerManager(config)

	ctx, cancel := context.WithTimeout(context.Background(), config.Timeout)
	defer cancel()

	sendCh := make(chan HeartbeatMessage)
	receiveCh := make(chan HeartbeatMessage)

	go listen(ctx, config, receiveCh)
	go processHeartbeats(ctx, config, peerManager, receiveCh)

	go sendHeartbeats(ctx, config, peerManager, sendCh)

	for {
		select {
		case <-ctx.Done():
			log.Println("Shutting down...")
			return
		case msg := <-sendCh:
			if peer, ok := peerManager.GetPeer(msg.To); ok {
				sendMessage(config, peer)
			}
		}
	}
}
