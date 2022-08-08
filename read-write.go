package main

import (
	"bytes"
	"errors"
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"syscall"
)

const (
	HTTP_SUCCESS = "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n"
	BUFFER_SIZE  = 1 << 10
)

func main() {
	listener, err := net.Listen("tcp", ":8080")
	fatal(err)
	for {
		connection, err := listener.Accept()
		fatal(err)
		handleConnection(connection)
	}
}

func fatal(err error) {
	if err != nil {
		log.Fatal(err)
	}
}

func handleConnection(conn net.Conn) {
	//read request
	readRequest(conn)

	html, err := os.OpenFile("index.html", os.O_RDONLY, 0)
	fatal(err)

	fileInfo, err := html.Stat()
	fatal(err)

	res := fmt.Sprintf(HTTP_SUCCESS, fileInfo.Size())
	conn.Write([]byte(res))

	//stream file
	streamFile(html, conn)

	// clean up
	conn.Close()
	html.Close()

}

func streamFile(f *os.File, conn net.Conn) {
	// allocate buffer
	var buffer [BUFFER_SIZE]byte

	numWrite := int(0)
	// read from html
	for {
		n, err := f.Read(buffer[:])
		if err != nil {
			if err == io.EOF {
				break
			}

			fatal(err)
		}

		if n == 0 {
			break
		}
		// write to connection
		bWritten, err := conn.Write(buffer[:n])
		if errors.Is(err, syscall.EPIPE) || errors.Is(err, syscall.ECONNRESET) {
			// just ignore.
			break
		}
		fatal(err)
		numWrite += bWritten
	}

	log.Println("wrote ", numWrite, "bytes")
}

func readRequest(conn net.Conn) string {
	buffer := bytes.Buffer{}

	copierBuff := [BUFFER_SIZE]byte{}

	// return
loopConnection:
	for {
		lineLength := 0
	loopLine:
		for {
			// zero out the buffer
			bzero(copierBuff[:])
			// read line from connection
			n, err := conn.Read(copierBuff[:])
			lineLength = n
			if err != nil {
				if err == io.EOF {
					if n > 0 {
						buffer.Write(copierBuff[:lineLength])
					}
					break loopConnection
				}
				fatal(err)
			}
			buffer.Write(copierBuff[:lineLength])
			if n == 0 || bytes.HasSuffix(copierBuff[:lineLength], []byte{0xD, 0xA}) {
				break loopLine
			}
		}
		if len(copierBuff) == 0 || bytes.HasSuffix(copierBuff[:lineLength], []byte{0xD, 0xA, 0xD, 0xA}) {
			break loopConnection
		}
	}
	return buffer.String()
}

func bzero(s []byte) {
	for i, _ := range s {
		s[i] = 0x0
	}
}
