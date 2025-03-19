# Pintos-Project

Pintos is an educational operating system framework designed for teaching and learning operating system concepts. This project involves implementing and extending various components of the Pintos operating system.

## Project Structure

The project is organized into the following directories:

- **src/**: Contains the source code for the Pintos operating system.
  - **devices/**: Device drivers for hardware components like block devices, keyboard, and timers.
  - **examples/**: Example programs to test the operating system.
  - **filesys/**: File system implementation.
  - **lib/**: Utility libraries used across the project.
  - **misc/**: Miscellaneous files.
  - **tests/**: Test cases for validating the operating system's functionality.
  - **threads/**: Thread management and synchronization primitives.
  - **userprog/**: User program loader and system call implementations.
  - **utils/**: Utility scripts and tools.
  - **vm/**: Virtual memory management.

## Getting Started

### Prerequisites

To build and run Pintos, you need the following tools installed on your system:

- GCC (GNU Compiler Collection)
- GDB (GNU Debugger)
- QEMU or Bochs (for emulating the operating system)

### Building the Project

To build the project, navigate to the `src/` directory and run:

```bash
make
```

### Running Pintos

To run Pintos, use the `pintos` script provided in the `src/utils/` directory. For example:

```bash
pintos -- run <program>
```

## Acknowledgments
Built as part of the Operating Systems course.

Based on the original Pintos framework by Stanford University.
