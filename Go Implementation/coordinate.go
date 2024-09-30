package main

import (
	"bufio"
	"flag"
	"fmt"
	"log"
	"net"
	"os"
	"strings"
	"sync"
	"time"
)

const (
	udpPort      = 8000
	message      = "HEARTBEAT"
	timeout      = 1 * time.Minute
	sendInterval = 5 * time.Second
)

var (
	debugFlag bool
)

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
		if line != "" && !strings.HasPrefix(line, "#") { // leave out empty/commented line
			peers = append(peers, line)
		}
	}

	if err := scanner.Err(); err != nil {
		return nil, err
	}

	return peers, nil
}

func logDebug(message string) {
	if debugFlag {
		log.Println(message)
	}
}

func sendReady(peer string) {
	addr, err := net.ResolveUDPAddr("udp", fmt.Sprintf("%s:%d", peer, udpPort))
	if err != nil {
		log.Printf("Error resolving address for %s: %v", peer, err)
		return
	}

	conn, err := net.DialUDP("udp", nil, addr)
	if err != nil {
		log.Printf("Error dialing %s: %v", peer, err)
		return
	}
	defer conn.Close()

	_, err = conn.Write([]byte(message))
	if err != nil {
		log.Printf("Error sending to %s: %v", peer, err)
	} else {
		logDebug(fmt.Sprintf("Sent READY to %s\n", peer))
	}
}

func listen(peers []string) {
	addr, err := net.ResolveUDPAddr("udp", fmt.Sprintf(":%d", udpPort))
	if err != nil {
		log.Printf("Error resolving UDP address: %v", err)
	}

	conn, err := net.ListenUDP("udp", addr)
	if err != nil {
		log.Printf("Error listening: %v", err)
	}

	defer conn.Close()

	readyPeers := make(map[string]bool)
	readyPrinted := false
	var mu sync.Mutex

	for {
		buffer := make([]byte, 1024)
		n, remoteAddr, err := conn.ReadFromUDP(buffer)
		if err != nil {
			log.Printf("Error reading from UDP: %v", err)
			continue
		}

		if string(buffer[:n]) == message {
			mu.Lock()
			readyPeers[remoteAddr.String()] = true
			count := len(readyPeers)
			mu.Unlock()

			logDebug(fmt.Sprintf("Received READY from %s\n", remoteAddr.String()))

			if count == len(peers) && !readyPrinted {
				log.Print("READY")
				readyPrinted = true
			}
		}
	}
}

func main() {
	// Define command-line flag for hosts file
	hostsFile := flag.String("h", "", "Path to hosts file")
	debug := flag.Bool("d", false, "Enable debugging mode")
	flag.Parse()

	debugFlag = *debug

	// Check if hosts file is provided
	if *hostsFile == "" {
		log.Print("Hosts file not provided. Use -h flag to specify the hosts file.")
	}

	// Read and parse the hosts file
	peers, err := readHostsFile(*hostsFile)
	if err != nil {
		log.Print("Error reading hosts file: ", err)
	}

	// Get the hostname
	hostname, err := os.Hostname()
	if err != nil {
		log.Print("Error getting hostname: ", err)
	}

	// Remove own hostname from peers list
	for i, peer := range peers {
		if peer == hostname {
			peers = append(peers[:i], peers[i+1:]...)
			break
		}
	}

	logDebug(fmt.Sprint("My hostname: ", hostname))
	logDebug(fmt.Sprint("Peers: ", strings.Join(peers, ",")))

	// Start listener goroutine
	go listen(peers)

	// Start sending messages
	go func() {
		for {
			for _, peer := range peers {
				sendReady(peer)
			}
			time.Sleep(sendInterval)
		}
	}() // Keep on sending messages

	var wg sync.WaitGroup
	wg.Add(1)

	go func() {
		time.Sleep(timeout)
		wg.Done()
	}() // Overall wait for timeout

	wg.Wait()
}
