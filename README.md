# TicketingTask_SarahAbdelrahman
Subway Ticketing system simulating ticket vending machine, entrance gate and Backoffice system create and validate the tickets.

# Project Tree:
```text
.
├── mosquitto/
│   └── mosquitto.conf
│
├── Project/
│   ├── Backoffice_SWC/
│   │   ├── include/
│   │   │   └── Backoffice_app.hpp
│   │   ├── Src/
│   │   │   └── Backoffice_app.cpp
│   │   ├── tests/
│   │   └── CMakeLists.txt 
│   │
│   ├── Gate_SWC/
│   │   ├── include/
│   │   │   └── gate_app.hpp
│   │   ├── Src/
│   │   │   └── gate_app.cpp
│   │   ├── tests/
│   │   └── CMakeLists.txt
│   │
│   ├── TVM_SWC/
│   │   ├── include/
│   │   │   └── TVM_app.hpp
│   │   ├── Src/
│   │   │   └── TVM_app.cpp
│   │   ├── tests/
│   │   └── CMakeLists.txt
│   │
│   ├── build/
│   ├── Entry_Point.sh
│   └── Makefile
│
└── Dockerfile


# How To start and Run
1- Open linux shell terminal cd to inside repo path
2- Build Docker with command "docker build --network=host -t ticketing-app ."
3- Run Docker with command "docker run --rm -it --name ticketing-app ticketing-app"
4- Running the docker will start mosquitto broker and build cpp files and run the backoffice_app then let you chose you want to run TVM_app or Gate_app
