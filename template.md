<h1>PSIA 3</h1>

<h2>Sender</h2>

*	send Lenght of file
* wait for confirmation
* while
  * prepare N packets
  * send N packets
  * request confirmation (contains number of first and last packet)
  * resend unconfirmed packets (wait for confirmation after each packet)   
* send hash
* wait for confirmation

<h2>Receiver</h2>

*	receive Lenght of file
* send confirmation and start transfer
* while
  * receive packets until request confirmation
  * send confirmation (1 bit = 1 received packet )
  * receive corrupted packets (send confirmation after each packet)
* wait for hash from sender
* send confirmation
