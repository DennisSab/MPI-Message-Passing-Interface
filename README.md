# MPI-Message-Passing-Interface

Distributed Computing Principles - Assignment 2

## 1. General Description

In this assignment, you will implement a distributed satellite communication system where ground stations coordinate with satellites for data exchange and command execution. Satellites collect data of interest to ground stations, which also monitor the satellites' operational status.

The implementation must be written in C using the Message Passing Interface (MPI), available on the university's computing infrastructure.

## 2. System and Algorithm Description

The system consists of three types of processes:
- **Coordinator**: Manages and orchestrates communication between satellites and ground stations.
- **Satellite processes**: Collect Earth temperature measurements and maintain an operational status.
- **Ground station processes**: Receive temperature data and monitor satellite status.

### Communication
- The **coordinator** reads a `testfile` defining system events and propagates commands to appropriate processes.
- **Ground stations** send requests for temperature data and satellite status.
- **Satellites** respond to requests using stored temperature data.
- **Satellites** are connected in a **ring topology**.
- **Ground stations** are connected in a **tree topology**, defined dynamically by the coordinator.

## Future Enhancements
- Add a GUI for better user interaction.
- Implement a database for persistent storage.
- Improve performance using advanced thread management techniques.

## Contributing
Feel free to fork this repository, make improvements, and submit a pull request.

## License
This project is licensed under the MIT License.

---

### Author
[Ntenis Sampani]

For any queries, contact: [https://github.com/DennisSab]
