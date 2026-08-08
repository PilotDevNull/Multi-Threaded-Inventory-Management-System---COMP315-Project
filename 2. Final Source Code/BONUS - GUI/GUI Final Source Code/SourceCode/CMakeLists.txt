cmake_minimum_required(VERSION 3.16)

project(NexusLogistics VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent Charts)

qt_standard_project_setup()

set(PROJECT_SOURCES
    main.cpp
    resources/resources.qrc
    src/models/ProductDto.h
    src/models/InventorySnapshotDto.h
    src/models/LogsSnapshotDto.h
    src/models/OrdersSummaryDto.h
    src/models/OrdersBatchResultDto.h
    src/models/AnalyticsSnapshotDto.h
    src/integration/AppController.h
    src/integration/AppController.cpp
    src/integration/AnalyticsAggregator.h
    src/integration/AnalyticsAggregator.cpp
    src/integration/InventoryServiceAdapter.h
    src/integration/InventoryServiceAdapter.cpp
    src/integration/OrdersServiceAdapter.h
    src/integration/OrdersServiceAdapter.cpp
    src/ui/mainwindow.h
    src/ui/mainwindow.cpp
    src/ui/mainwindow.ui
    src/ui/navbutton.h
    src/ui/navbutton.cpp
    src/ui/dashboardpage.h
    src/ui/dashboardpage.cpp
    src/ui/dashboardpage.ui
    src/ui/inventorypage.h
    src/ui/inventorypage.cpp
    src/ui/inventorypage.ui
    src/ui/orderspage.h
    src/ui/orderspage.cpp
    src/ui/orderspage.ui
    src/ui/logspage.h
    src/ui/logspage.cpp
    src/ui/logspage.ui
    src/ui/analyticspage.h
    src/ui/analyticspage.cpp
    src/ui/analyticspage.ui
    src/ui/settingspage.h
    src/ui/settingspage.cpp
    src/ui/settingspage.ui
    src/ui/uihelpers.h
    src/ui/uihelpers.cpp
    final_project_v2/Product.h
    final_project_v2/Product.cpp
    final_project_v2/StandardProduct.h
    final_project_v2/StandardProduct.cpp
    final_project_v2/TaxableProduct.h
    final_project_v2/TaxableProduct.cpp
    final_project_v2/DiscountedProduct.h
    final_project_v2/DiscountedProduct.cpp
    final_project_v2/InventorySystem.h
    final_project_v2/InventorySystem.cpp
    final_project_v2/Order.h
    final_project_v2/Order.cpp
    final_project_v2/OrderPool.h
    final_project_v2/OrderPool.cpp
    final_project_v2/Warehouse.h
    final_project_v2/Warehouse.cpp
    final_project_v2/WorkerThread.h
    final_project_v2/WorkerThread.cpp
    final_project_v2/AnalyticsRecord.h
    final_project_v2/AnalyticsRecord.cpp
    final_project_v2/AnalyticsManager.h
    final_project_v2/AnalyticsManager.cpp
)


qt_add_executable(NexusLogistics
    ${PROJECT_SOURCES}
)

target_include_directories(NexusLogistics PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/integration
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/final_project_v2
)

target_link_libraries(NexusLogistics PRIVATE Qt6::Widgets Qt6::Concurrent Qt6::Charts)
target_compile_definitions(NexusLogistics PRIVATE
    NEXUS_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

# Force resource rebuild
