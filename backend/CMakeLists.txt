cmake_minimum_required(VERSION 3.15)
project(TrafficManagementBackend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

find_package(Crow CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

add_executable(TrafficManagementBackend
    src/main.cpp

    # Data Access Layer
    src/DataAccessLayer/DatabaseConnection.cpp
    src/DataAccessLayer/OwnerRepository.cpp
    src/DataAccessLayer/VehicleRepository.cpp
    src/DataAccessLayer/JunctionRepository.cpp
    src/DataAccessLayer/JunctionLogRepository.cpp
    src/DataAccessLayer/ViolationTypeRepository.cpp
    src/DataAccessLayer/ViolationRepository.cpp
    src/DataAccessLayer/FineRepository.cpp
    src/DataAccessLayer/CongestionRepository.cpp
    src/DataAccessLayer/RoadSegmentRepository.cpp
    src/DataAccessLayer/DriverProfileRepository.cpp
    src/DataAccessLayer/EmergencyVehicleRepository.cpp
    src/DataAccessLayer/EmergencyEventRepository.cpp
    src/DataAccessLayer/SignalRepository.cpp
    src/DataAccessLayer/NotificationRepository.cpp

    # Business Logic (MicroserviceLayer)
    src/MicroserviceLayer/VehicleValidator.cpp
    src/MicroserviceLayer/PlateValidator.cpp
    src/MicroserviceLayer/ViolationRules.cpp
    src/MicroserviceLayer/TrafficRules.cpp
    src/MicroserviceLayer/RoutePlanner.cpp
    src/MicroserviceLayer/VehicleController.cpp
    src/MicroserviceLayer/JunctionController.cpp
    src/MicroserviceLayer/ViolationController.cpp
    src/MicroserviceLayer/FineController.cpp
    src/MicroserviceLayer/TrafficController.cpp
    src/MicroserviceLayer/EmergencyController.cpp
    src/MicroserviceLayer/NotificationController.cpp
)

target_include_directories(TrafficManagementBackend PRIVATE src ${PQXX_INCLUDE_DIRS})
target_link_libraries(TrafficManagementBackend PRIVATE Crow::Crow ${PQXX_LIBRARIES})
target_compile_options(TrafficManagementBackend PRIVATE ${PQXX_CFLAGS_OTHER})

if(WIN32)
    target_compile_definitions(TrafficManagementBackend PRIVATE
        _WIN32_WINNT=0x0601
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
    target_link_libraries(TrafficManagementBackend PRIVATE ws2_32 wsock32)
endif()

add_executable(TrafficManagementBackendTests
    tests/RulesTests.cpp
    src/MicroserviceLayer/VehicleValidator.cpp
    src/MicroserviceLayer/PlateValidator.cpp
    src/MicroserviceLayer/ViolationRules.cpp
    src/MicroserviceLayer/TrafficRules.cpp
    src/MicroserviceLayer/RoutePlanner.cpp
)

target_include_directories(TrafficManagementBackendTests PRIVATE src)
add_test(NAME TrafficManagementBackendTests COMMAND TrafficManagementBackendTests)
