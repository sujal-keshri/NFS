# final-project-52

1. Make sure you compile the storage server code as "gcc storageserver.c server_functionalities.c" as it requires some functions that are written in server_funcitionalities.c

2. Give the ports for Client Comms and Naming Server Comms as command line arguments while running the executable for Storage Server. 

3. The storage server expects Accessible paths as user input.

4. Make sure you give the Accessible paths that are present inside the folder where the executable of storage server is run. 

# Assumptions

1. It is assumed that the Client will should go on to next operation if there is already a writer present without waiting until the writer completes his operation. 

2. All file /paths are unique.

3. Cache Size is kept as 10

4. Directory is deleted only when its empty, if it had some contents the will result in an unsuccessful deletion. 