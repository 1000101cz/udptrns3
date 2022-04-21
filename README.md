<h1>UDP HW3</h1>

<h2>Description</h2>

*	Client application transfer file to server application via UDP packets. Each packet is checked with CRC32, whole file with SHA256 hash function. Packet data length is 1024 bytes and all transfered data are unsigned char strings.

<h2>Compilation:</h2>

*	compile with  `$ make`

<h2>Run Application:</h2>

*	first start server binary `$ ./server [/path/to/new_file.x]`

*	then start client binary `$ ./client [server IP] [/path/to/file.x]`

<h2>Client side communication</h2>

*	<h3>Handshake</h3>
		Client sends packet with length of file in bytes and waits for confirmation  that data were receiver from server.

	If confirmation from server was not received during defined time [TIMEOUT_MS], file length is sent again.

* <h3>File transfer</h3>
		Transfered file is divided into 1024B blocks.

	*	Client sends packet with data, `Number of packet` (first packet number is 1) and `CRC` of this packet.
	* Waits for server confirmation that CRC is valid. If client do not receive confirmation from server or server returns error, it sends packet again.

* <h3>SHA256 check</h3>
	Client sends packet with SHA256 hash of whole file and waits for confirmation from server that hashes of file before and after transfer are the same.



<h2>Server side communication</h2>

* <h3>Handshake</h3>
	Server receives length of transfered file from client and sends back Success message.

* <h3>File transfer</h3>

	*	Server receive packet with data, `number of upcoming packet` and its `CRC`
	*	Now computes its own CRC from data section and checks if it is the same as received CRC.
	*	If received CRC is the same as computed and if server received expected package number, it sends Success message to client. Otherwise it sends Error and waits for the same packet again.

* <h3>SHA256 check</h3>
	When whole file is received, server computes SHA256 hash of new file. Then waits for SHA256 hash of original file from client.

	If hashes are the same, Success message is sent to client, otherwise Error message.
