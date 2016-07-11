WinMessage v0. Based off of the Bitmessage protocol.

TODO:

- Connections
 
	-	Create timer to maintain the network health (attempt a connection to each node on the list every 20 mins?)
	-	if a connection drops, remove it from the connection list.(the list is for active connections only).
	-	create counters to keep track of lists
	-	stop deleting nodes if the list goes under 1000
	-	max nodes == 20 000
	-	create Ping and Pong functions	
	-	start connecting to network?


- Messaging
	-	make sure all objects are stored as vectors when we receive them or create them.
	-	create a list for Address's that is seperate from the Vectors list
	-	modify appropriate functions to reflect the above change.
	-	create the function to handle pubkey incoming objects
	-	finish the handling of public keys (sending & receiving)


