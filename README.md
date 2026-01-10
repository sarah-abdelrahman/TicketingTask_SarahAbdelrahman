# TicketingTask_SarahAbdelrahman
Train Ticketing system simulating ticket vending machine, entrance gate and Backoffice system that create and validate the tickets.

# Project Tree:
```text
.
├── mosquitto/
│   └── mosquitto.conf
├── build/
│   ├── backoffice_app
│   ├── tvm_app
│   └── gate_app
├── Project/
│   ├── Backoffice_SWC/
│   │   ├── include/
│   │   │   └── Backoffice_app.hpp
│   │   ├── Src/
│   │   │   └── Backoffice_app.cpp
│   │   ├── tests/
│   │   ├── CDD/
│   │   │    ├── BO_ClassDiagram.uml
│   │   │    └── BO_component_design.uml
│   │   └── CMakeLists.txt 
│   │
│   ├── Gate_SWC/
│   │   ├── include/
│   │   │   └── gate_app.hpp
│   │   ├── Src/
│   │   │   └── gate_app.cpp
│   │   ├── tests/
│   │   ├── CDD/
│   │   │    ├── Gate_ClassDiagram.uml
│   │   │    └── Gate_component_design.uml
│   │   └── CMakeLists.txt
│   │
│   ├── TVM_SWC/
│   │   ├── include/
│   │   │   └── TVM_app.hpp
│   │   ├── Src/
│   │   │   └── TVM_app.cpp
│   │   ├── tests/
│   │   ├── CDD/
│   │   │    ├── TVM_ClassDiagram.uml
│   │   │    └── TVM_component_design.uml
│   │   └── CMakeLists.txt
│   │
│   ├── Entry_Point.sh
│   └── Makefile
│
└── Dockerfile


# How To start and Run
1- Open linux shell terminal cd to inside repo path
2- Build Docker with command "docker build --network=host -t ticketing-app ."
3- Run Docker with command "docker run --rm -it --name ticketing-app ticketing-app"
4- Running the docker will start mosquitto broker and build cpp files and run the backoffice_app then let you chose you want to run TVM_app or Gate_app
5- regardless of the defualt build and start from  dockerfile you can build each application individually inside docker with make file rules as below:
make tvm
make gate
make backoffice
make all
make clean
