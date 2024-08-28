# Network File System (NFS)

## Introduction
This Network File System (NFS) project is a component of an Operating System course, designed to implement a distributed file system that enables clients to access and manage files across multiple servers.

## System Components

### Client Systems
Clients are the users or systems that interact with NFS to perform operations such as file reading, writing, deletion, and more.

### Naming Server (NM)
The Naming Server serves as the main coordinator, handling the directory structure and directing client requests to the corresponding Storage Servers.

### Storage Servers (SS)
Storage Servers are responsible for storing and managing the actual files and directories requested by clients, ensuring data persistence and availability.

## Features

### Core File Operations
1. **File/Folder Creation or Update:** Clients can create or modify files and folders within the NFS.
2. **File Reading:** Clients can access the contents of files stored in the NFS.
3. **File/Folder Deletion:** Clients can remove files and folders that are no longer needed from the NFS.
4. **New File/Folder Creation:** Clients initiate the creation of new files and directories.
5. **Directory Listing:** Clients can browse the NFS to view files and subfolders within directories.
6. **Metadata Retrieval:** Clients can access file details such as size, permissions, and timestamps.

### System Specifications

#### Startup Process
1. **Initialization of Naming and Storage Servers:** 
   - The Naming Server (NM) starts first and coordinates with the Storage Servers (SS).
   - Each Storage Server (SS) registers its details (IP, ports, accessible paths) with the NM.
   - The NM begins processing client requests once all SS are initialized.

2. **Storage Server Operations (SS):**
   - SS can dynamically register new entries with the NM during operation.
   - Execute commands from NM such as creating, deleting, or copying files/directories.
   - Facilitate client operations like file read, write, and metadata retrieval.

3. **Naming Server (NM):**
   - Manages essential SS information and handles client task feedback.
   - Efficiently locates SS to handle client requests using optimized search methods.
   - Implements LRU caching for quicker responses and uses data redundancy for fault tolerance.

4. **Client Interaction:**
   - Clients communicate with NM to locate files and perform operations.
   - Directly interact with the assigned SS for efficient data exchanges.
   - Supports concurrent client access and file reading; handles errors with clear codes.

5. **Additional Capabilities:**
   - **Concurrency:** Manages multiple clients and simultaneous file access.
   - **Error Handling:** Provides clear error codes for different NFS conditions.
   - **Optimized Search:** Utilizes efficient data structures for fast file lookup.
   - **Data Redundancy:** Implements replication for fault tolerance.
   - **Logging:** Records all operations for debugging and monitoring purposes.

## Implementation
- Developed using C.

### Usage Instructions

1. Compile the storage server code using: `gcc storageserver.c server_functionalities.c`, as certain functions are in `server_functionalities.c`.
2. Provide the ports for Client Communication and Naming Server Communication as command-line arguments when running the Storage Server executable.
3. The storage server expects accessible paths to be provided by the user as input.
4. Ensure the accessible paths are within the directory where the Storage Server executable is run.

## Assumptions

1. It is assumed that clients will proceed to the next operation if a writer is present, without waiting for the writer to finish.
2. All file paths are unique.
3. Cache size is set to 10.
4. A directory is deleted only if it is empty; attempting to delete a non-empty directory will fail.
