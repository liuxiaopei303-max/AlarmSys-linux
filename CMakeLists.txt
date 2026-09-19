cmake_minimum_required(VERSION 3.16)
project(AlarmSys-linux LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC OFF)
set(CMAKE_AUTOUIC OFF)

add_definitions(-DVS_LINUX)

set(FASTDDS_ROOT "/usr/local/fastdds3" CACHE PATH "Fast DDS install prefix (include + lib)")

# 默认用 quick_build_start.sh / build_and_restart.sh 传入的 /usr Qt；RPATH 与 Qt5::Core 目录一致。
find_package(Qt5 REQUIRED COMPONENTS Core Network Sql Concurrent Gui Widgets)

find_library(FASTDDS_LIB NAMES fastdds HINTS "${FASTDDS_ROOT}/lib" /usr/local/lib /usr/lib/x86_64-linux-gnu)
find_library(FASTCDR_LIB NAMES fastcdr HINTS "${FASTDDS_ROOT}/lib" /usr/local/lib /usr/lib/x86_64-linux-gnu)

if(NOT FASTDDS_LIB OR NOT FASTCDR_LIB)
    message(FATAL_ERROR "未找到 libfastdds / libfastcdr，请设置 -DFASTDDS_ROOT=... 或安装 Fast DDS 3.x（可参考 watch-sys-camera 使用的 /usr/local/fastdds3）")
endif()

set(ALARM_SRC_DIR "${CMAKE_SOURCE_DIR}/src")
set(SPX_INCLUDES "${CMAKE_SOURCE_DIR}/third_party/spx/Includes")
set(NEWTRACK_STRUCT_DIR "${CMAKE_SOURCE_DIR}/third_party/NewTrackStruct")
set(GRPC_ALARM_GEN_DIR "${CMAKE_SOURCE_DIR}/third_party/grpc_alarm/generated")
set(GRPC_DESTROY_GEN_DIR "${CMAKE_SOURCE_DIR}/third_party/grpc_destroy/generated")
set(GRPC_TARGET_TYPE_GEN_DIR "${CMAKE_SOURCE_DIR}/third_party/grpc_target_type/generated")
set(GRPC_SYSTEM_ALARM_GEN_DIR "${CMAKE_SOURCE_DIR}/third_party/grpc_system_alarm/generated")
set(GRPC_TARGET_THREAT_GEN_DIR "${CMAKE_SOURCE_DIR}/third_party/grpc_target_threat/generated")
set(GRPC_FUSION_TRACK_GEN_DIR "${CMAKE_SOURCE_DIR}/third_party/grpc_fusion_track/generated")
set(GRPC_NEW_TRACK_STRUCT_GEN_DIR "${CMAKE_SOURCE_DIR}/third_party/grpc_new_track_struct/generated")
set(GRPC_ROOT "/home/inspur/anaconda3" CACHE PATH "gRPC/protobuf install prefix")

set(ALARM_SOURCES
    "${ALARM_SRC_DIR}/main.cpp"
    "${ALARM_SRC_DIR}/AlarmHttpServer.cpp"
    "${ALARM_SRC_DIR}/alarm_geoproj.cpp"
    "${ALARM_SRC_DIR}/customconfig.cpp"
    "${ALARM_SRC_DIR}/common/commonfunc.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AlarmAreaGeometryParser.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/ArchiveVisitEvidence.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AirAlarmEligibility.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/NoAlarmAreaPolicy.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/TrackAlarmThread.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationEvaluator.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationProtectionResolver.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/ThreatAssessmentCalculator.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/SuspiciousTargetThread.cpp"
    "${ALARM_SRC_DIR}/dialog/analysis/TargetTypeFusionThread.cpp"
    "${ALARM_SRC_DIR}/dialog/analysis/TargetTypeFusionLogic.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AlarmFileLogger.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AlarmContentBuilder.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationAlarmContent.cpp"
    "${ALARM_SRC_DIR}/http_alarm/AlarmHttpSnapshotFilter.cpp"
    "${ALARM_SRC_DIR}/http_alarm/AlarmHttpSnapshotJson.cpp"
    "${ALARM_SRC_DIR}/http_alarm/AlarmHttpSnapshotPublisher.cpp"
    "${ALARM_SRC_DIR}/db/DataAccessLayer.cpp"
    "${ALARM_SRC_DIR}/db/DatabaseStartupRetry.cpp"
    "${ALARM_SRC_DIR}/db/DatabaseManager.cpp"
    "${ALARM_SRC_DIR}/db/dbconnectionpool.cpp"
    "${ALARM_SRC_DIR}/db/dbhelper.cpp"
)

file(GLOB ALARM_DDS_ALARM_SRC CONFIGURE_DEPENDS
    "${ALARM_SRC_DIR}/fastdds_alarmevent/*.cxx")
list(FILTER ALARM_DDS_ALARM_SRC EXCLUDE REGEX ".*main\\.cxx$")

file(GLOB ALARM_DDS_TRACK_SRC CONFIGURE_DEPENDS
    "${ALARM_SRC_DIR}/fastdds_track/*.cxx")
list(FILTER ALARM_DDS_TRACK_SRC EXCLUDE REGEX ".*main\\.cxx$")

file(GLOB ALARM_DDS_NEWTRACK_SRC CONFIGURE_DEPENDS
    "${ALARM_SRC_DIR}/fastdds_newtrack/*.cxx")

file(GLOB ALARM_GRPC_SRC CONFIGURE_DEPENDS
    "${ALARM_SRC_DIR}/grpc_alarm/*.cpp"
    "${ALARM_SRC_DIR}/grpc_target_type/*.cpp"
    "${ALARM_SRC_DIR}/grpc_system_alarm/*.cpp"
    "${ALARM_SRC_DIR}/grpc_target_threat/*.cpp"
    "${ALARM_SRC_DIR}/grpc_track/*.cpp")

set(GRPC_ALARM_GEN_SRC
    "${GRPC_ALARM_GEN_DIR}/alarm_service.pb.cc"
    "${GRPC_ALARM_GEN_DIR}/alarm_service.grpc.pb.cc")

set(GRPC_DESTROY_GEN_SRC
    "${GRPC_DESTROY_GEN_DIR}/destroy.pb.cc"
    "${GRPC_DESTROY_GEN_DIR}/destroy.grpc.pb.cc")

set(GRPC_TARGET_TYPE_GEN_SRC
    "${GRPC_TARGET_TYPE_GEN_DIR}/target_type.pb.cc"
    "${GRPC_TARGET_TYPE_GEN_DIR}/target_type.grpc.pb.cc")

set(GRPC_SYSTEM_ALARM_GEN_SRC
    "${GRPC_SYSTEM_ALARM_GEN_DIR}/system_alarm.pb.cc"
    "${GRPC_SYSTEM_ALARM_GEN_DIR}/system_alarm.grpc.pb.cc")

set(GRPC_TARGET_THREAT_GEN_SRC
    "${GRPC_TARGET_THREAT_GEN_DIR}/target_threat_query.pb.cc"
    "${GRPC_TARGET_THREAT_GEN_DIR}/target_threat_query.grpc.pb.cc")

set(GRPC_FUSION_TRACK_GEN_SRC
    "${GRPC_FUSION_TRACK_GEN_DIR}/fusion_track_stream.pb.cc"
    "${GRPC_FUSION_TRACK_GEN_DIR}/fusion_track_stream.grpc.pb.cc")

set(GRPC_NEW_TRACK_STRUCT_GEN_SRC
    "${GRPC_NEW_TRACK_STRUCT_GEN_DIR}/target_stream.pb.cc"
    "${GRPC_NEW_TRACK_STRUCT_GEN_DIR}/target_stream.grpc.pb.cc")

set(NEWTRACK_STRUCT_GEN_SRC
    "${NEWTRACK_STRUCT_DIR}/NewTrackRealTimeStatusPubSubTypes.cxx"
    "${NEWTRACK_STRUCT_DIR}/NewTrackRealTimeStatusTypeObjectSupport.cxx")

add_executable(alarmsys
    ${ALARM_SOURCES}
    ${ALARM_DDS_ALARM_SRC}
    ${ALARM_DDS_TRACK_SRC}
    ${ALARM_DDS_NEWTRACK_SRC}
    ${ALARM_GRPC_SRC}
    ${GRPC_ALARM_GEN_SRC}
    ${GRPC_DESTROY_GEN_SRC}
    ${GRPC_TARGET_TYPE_GEN_SRC}
    ${GRPC_SYSTEM_ALARM_GEN_SRC}
    ${GRPC_TARGET_THREAT_GEN_SRC}
    ${GRPC_FUSION_TRACK_GEN_SRC}
    ${GRPC_NEW_TRACK_STRUCT_GEN_SRC}
    ${NEWTRACK_STRUCT_GEN_SRC})

target_include_directories(alarmsys PRIVATE
    "${ALARM_SRC_DIR}"
    "${SPX_INCLUDES}"
    "${NEWTRACK_STRUCT_DIR}"
    "${GRPC_ALARM_GEN_DIR}"
    "${GRPC_DESTROY_GEN_DIR}"
    "${GRPC_TARGET_TYPE_GEN_DIR}"
    "${GRPC_SYSTEM_ALARM_GEN_DIR}"
    "${GRPC_TARGET_THREAT_GEN_DIR}"
    "${GRPC_FUSION_TRACK_GEN_DIR}"
    "${GRPC_NEW_TRACK_STRUCT_GEN_DIR}"
    "${FASTDDS_ROOT}/include"
    "${GRPC_ROOT}/include"
)

find_library(GRPCXX_LIB NAMES grpc++ HINTS "${GRPC_ROOT}/lib")
find_library(GRPC_LIB NAMES grpc HINTS "${GRPC_ROOT}/lib")
find_library(GPR_LIB NAMES gpr HINTS "${GRPC_ROOT}/lib")
find_library(PROTOBUF_LIB NAMES protobuf HINTS "${GRPC_ROOT}/lib")
find_library(ABSL_SYNC_LIB NAMES absl_synchronization HINTS "${GRPC_ROOT}/lib")
find_library(ABSL_STRINGS_LIB NAMES absl_strings HINTS "${GRPC_ROOT}/lib")
find_library(UPB_LIB NAMES upb HINTS "${GRPC_ROOT}/lib")

if(NOT GRPCXX_LIB OR NOT GRPC_LIB OR NOT GPR_LIB OR NOT PROTOBUF_LIB)
    message(FATAL_ERROR "未找到 gRPC/protobuf 库，请设置 -DGRPC_ROOT=... (当前 ${GRPC_ROOT})")
endif()

target_link_libraries(alarmsys PRIVATE
    Qt5::Core
    Qt5::Network
    Qt5::Sql
    Qt5::Concurrent
    Qt5::Gui
    Qt5::Widgets
    ${FASTDDS_LIB}
    ${FASTCDR_LIB}
    ${GRPCXX_LIB}
    ${GRPC_LIB}
    ${GPR_LIB}
    ${UPB_LIB}
    ${ABSL_SYNC_LIB}
    ${ABSL_STRINGS_LIB}
    ${PROTOBUF_LIB}
    pthread
)

enable_testing()
add_executable(alarm_rule_boolean_test
    "${CMAKE_SOURCE_DIR}/tests/AlarmRuleBooleanTest.cpp")
target_include_directories(alarm_rule_boolean_test PRIVATE "${ALARM_SRC_DIR}")
target_link_libraries(alarm_rule_boolean_test PRIVATE Qt5::Core Qt5::Sql)
set_target_properties(alarm_rule_boolean_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests" AUTOMOC OFF)
add_test(NAME alarm_rule_boolean_test COMMAND alarm_rule_boolean_test)

add_executable(alarm_motion_conditions_test
    "${CMAKE_SOURCE_DIR}/tests/AlarmMotionConditionsTest.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationEvaluator.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationProtectionResolver.cpp")
target_include_directories(alarm_motion_conditions_test PRIVATE "${ALARM_SRC_DIR}")
target_link_libraries(alarm_motion_conditions_test PRIVATE
    Qt5::Core Qt5::Gui Qt5::Sql Qt5::Concurrent Qt5::Network Qt5::Widgets)
set_target_properties(alarm_motion_conditions_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests" AUTOMOC OFF)
add_test(NAME alarm_motion_conditions_test COMMAND alarm_motion_conditions_test)

add_executable(area_escalation_evaluator_test
    "${CMAKE_SOURCE_DIR}/tests/AreaEscalationEvaluatorTest.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationEvaluator.cpp"
)
target_include_directories(area_escalation_evaluator_test PRIVATE "${ALARM_SRC_DIR}")
target_link_libraries(area_escalation_evaluator_test PRIVATE Qt5::Core Qt5::Gui)
set_target_properties(area_escalation_evaluator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME area_escalation_evaluator_test COMMAND area_escalation_evaluator_test)

add_executable(air_alarm_eligibility_test
    "${CMAKE_SOURCE_DIR}/tests/AirAlarmEligibilityTest.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AirAlarmEligibility.cpp"
)
target_include_directories(air_alarm_eligibility_test PRIVATE
    "${ALARM_SRC_DIR}"
    "${SPX_INCLUDES}"
)
target_link_libraries(air_alarm_eligibility_test PRIVATE Qt5::Core Qt5::Gui)
set_target_properties(air_alarm_eligibility_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME air_alarm_eligibility_test COMMAND air_alarm_eligibility_test)

add_executable(no_alarm_area_policy_test
    "${CMAKE_SOURCE_DIR}/tests/NoAlarmAreaPolicyTest.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/NoAlarmAreaPolicy.cpp"
)
target_include_directories(no_alarm_area_policy_test PRIVATE "${ALARM_SRC_DIR}")
target_link_libraries(no_alarm_area_policy_test PRIVATE Qt5::Core)
set_target_properties(no_alarm_area_policy_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME no_alarm_area_policy_test COMMAND no_alarm_area_policy_test)

add_executable(database_startup_retry_test
    "${CMAKE_SOURCE_DIR}/tests/DatabaseStartupRetryTest.cpp"
    "${ALARM_SRC_DIR}/db/DatabaseStartupRetry.cpp"
)
target_include_directories(database_startup_retry_test PRIVATE "${ALARM_SRC_DIR}")
set_target_properties(database_startup_retry_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME database_startup_retry_test COMMAND database_startup_retry_test)

add_executable(area_escalation_protection_resolver_test
    "${CMAKE_SOURCE_DIR}/tests/AreaEscalationProtectionResolverTest.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationProtectionResolver.cpp"
)
target_include_directories(area_escalation_protection_resolver_test PRIVATE "${ALARM_SRC_DIR}")
target_link_libraries(area_escalation_protection_resolver_test PRIVATE Qt5::Core Qt5::Gui)
set_target_properties(area_escalation_protection_resolver_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME area_escalation_protection_resolver_test COMMAND area_escalation_protection_resolver_test)

add_executable(alarm_area_geometry_parser_test
    "${CMAKE_SOURCE_DIR}/tests/AlarmAreaGeometryParserTest.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AlarmAreaGeometryParser.cpp"
)
target_include_directories(alarm_area_geometry_parser_test PRIVATE "${ALARM_SRC_DIR}")
target_link_libraries(alarm_area_geometry_parser_test PRIVATE Qt5::Core Qt5::Gui)
set_target_properties(alarm_area_geometry_parser_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME alarm_area_geometry_parser_test COMMAND alarm_area_geometry_parser_test)

add_executable(archive_visit_evidence_test
    "${CMAKE_SOURCE_DIR}/tests/ArchiveVisitEvidenceTest.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/ArchiveVisitEvidence.cpp"
)
target_include_directories(archive_visit_evidence_test PRIVATE "${ALARM_SRC_DIR}")
target_link_libraries(archive_visit_evidence_test PRIVATE Qt5::Core)
set_target_properties(archive_visit_evidence_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME archive_visit_evidence_test COMMAND archive_visit_evidence_test)

add_executable(alarm_grpc_snapshot_mapping_test
    "${CMAKE_SOURCE_DIR}/tests/AlarmGrpcSnapshotMappingTest.cpp"
)
target_include_directories(alarm_grpc_snapshot_mapping_test PRIVATE "${ALARM_SRC_DIR}")
target_link_libraries(alarm_grpc_snapshot_mapping_test PRIVATE Qt5::Core)
set_target_properties(alarm_grpc_snapshot_mapping_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME alarm_grpc_snapshot_mapping_test COMMAND alarm_grpc_snapshot_mapping_test)

add_executable(alarm_http_snapshot_json_test
    "${CMAKE_SOURCE_DIR}/tests/AlarmHttpSnapshotJsonTest.cpp"
    "${ALARM_SRC_DIR}/http_alarm/AlarmHttpSnapshotFilter.cpp"
    "${ALARM_SRC_DIR}/http_alarm/AlarmHttpSnapshotJson.cpp"
    "${GRPC_ALARM_GEN_DIR}/alarm_service.pb.cc"
)
target_include_directories(alarm_http_snapshot_json_test PRIVATE
    "${ALARM_SRC_DIR}"
    "${GRPC_ALARM_GEN_DIR}"
    "${GRPC_ROOT}/include"
)
target_link_libraries(alarm_http_snapshot_json_test PRIVATE
    Qt5::Core
    ${PROTOBUF_LIB}
    ${ABSL_SYNC_LIB}
    ${ABSL_STRINGS_LIB}
    pthread
)
set_target_properties(alarm_http_snapshot_json_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME alarm_http_snapshot_json_test COMMAND alarm_http_snapshot_json_test)

add_executable(alarm_http_snapshot_publisher_test
    "${CMAKE_SOURCE_DIR}/tests/AlarmHttpSnapshotPublisherTest.cpp"
    "${ALARM_SRC_DIR}/http_alarm/AlarmHttpSnapshotFilter.cpp"
    "${ALARM_SRC_DIR}/http_alarm/AlarmHttpSnapshotJson.cpp"
    "${ALARM_SRC_DIR}/http_alarm/AlarmHttpSnapshotPublisher.cpp"
    "${GRPC_ALARM_GEN_DIR}/alarm_service.pb.cc"
)
target_include_directories(alarm_http_snapshot_publisher_test PRIVATE
    "${ALARM_SRC_DIR}"
    "${GRPC_ALARM_GEN_DIR}"
    "${GRPC_ROOT}/include"
)
target_link_libraries(alarm_http_snapshot_publisher_test PRIVATE
    Qt5::Core
    Qt5::Network
    ${PROTOBUF_LIB}
    ${ABSL_SYNC_LIB}
    ${ABSL_STRINGS_LIB}
    pthread
)
set_target_properties(alarm_http_snapshot_publisher_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME alarm_http_snapshot_publisher_test COMMAND alarm_http_snapshot_publisher_test)

add_executable(new_track_struct_routing_test
    "${CMAKE_SOURCE_DIR}/tests/NewTrackStructRoutingTest.cpp"
    "${ALARM_SRC_DIR}/grpc_track/new_track_struct_grpc_convert.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AirAlarmEligibility.cpp"
    "${GRPC_NEW_TRACK_STRUCT_GEN_DIR}/target_stream.pb.cc"
)
target_include_directories(new_track_struct_routing_test PRIVATE
    "${ALARM_SRC_DIR}"
    "${SPX_INCLUDES}"
    "${GRPC_NEW_TRACK_STRUCT_GEN_DIR}"
    "${GRPC_ROOT}/include"
)
target_link_libraries(new_track_struct_routing_test PRIVATE
    Qt5::Core
    Qt5::Gui
    ${PROTOBUF_LIB}
    ${ABSL_SYNC_LIB}
    ${ABSL_STRINGS_LIB}
    pthread
)
set_target_properties(new_track_struct_routing_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME new_track_struct_routing_test COMMAND new_track_struct_routing_test)

add_executable(threat_assessment_calculator_test
    "${CMAKE_SOURCE_DIR}/tests/ThreatAssessmentCalculatorTest.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/ThreatAssessmentCalculator.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationProtectionResolver.cpp"
    "${ALARM_SRC_DIR}/grpc_track/new_track_struct_grpc_convert.cpp"
    "${GRPC_NEW_TRACK_STRUCT_GEN_DIR}/target_stream.pb.cc"
)
target_include_directories(threat_assessment_calculator_test PRIVATE
    "${ALARM_SRC_DIR}"
    "${SPX_INCLUDES}"
    "${GRPC_NEW_TRACK_STRUCT_GEN_DIR}"
    "${GRPC_ROOT}/include"
)
target_link_libraries(threat_assessment_calculator_test PRIVATE
    Qt5::Core
    Qt5::Gui
    ${PROTOBUF_LIB}
    ${ABSL_SYNC_LIB}
    ${ABSL_STRINGS_LIB}
    pthread
)
set_target_properties(threat_assessment_calculator_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME threat_assessment_calculator_test COMMAND threat_assessment_calculator_test)

add_executable(target_threat_proto_test
    "${CMAKE_SOURCE_DIR}/tests/TargetThreatProtoTest.cpp"
    "${GRPC_TARGET_THREAT_GEN_DIR}/target_threat_query.pb.cc"
    "${GRPC_ALARM_GEN_DIR}/alarm_service.pb.cc"
)
target_include_directories(target_threat_proto_test PRIVATE
    "${GRPC_TARGET_THREAT_GEN_DIR}"
    "${GRPC_ALARM_GEN_DIR}"
    "${GRPC_ROOT}/include"
)
target_link_libraries(target_threat_proto_test PRIVATE
    ${PROTOBUF_LIB}
    ${ABSL_SYNC_LIB}
    ${ABSL_STRINGS_LIB}
    pthread
)
set_target_properties(target_threat_proto_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME target_threat_proto_test COMMAND target_threat_proto_test)

add_executable(target_threat_query_assessment_test
    "${CMAKE_SOURCE_DIR}/tests/TargetThreatQueryAssessmentTest.cpp"
    "${ALARM_SRC_DIR}/grpc_target_threat/TargetThreatQueryAssessment.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AlarmAreaGeometryParser.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/ThreatAssessmentCalculator.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationProtectionResolver.cpp"
    "${ALARM_SRC_DIR}/grpc_track/new_track_struct_grpc_convert.cpp"
    "${GRPC_TARGET_THREAT_GEN_DIR}/target_threat_query.pb.cc"
    "${GRPC_ALARM_GEN_DIR}/alarm_service.pb.cc"
    "${GRPC_NEW_TRACK_STRUCT_GEN_DIR}/target_stream.pb.cc"
)
target_include_directories(target_threat_query_assessment_test PRIVATE
    "${ALARM_SRC_DIR}" "${SPX_INCLUDES}" "${GRPC_ROOT}/include"
    "${GRPC_TARGET_THREAT_GEN_DIR}" "${GRPC_ALARM_GEN_DIR}" "${GRPC_NEW_TRACK_STRUCT_GEN_DIR}"
)
target_link_libraries(target_threat_query_assessment_test PRIVATE
    Qt5::Core Qt5::Gui ${PROTOBUF_LIB} ${ABSL_SYNC_LIB} ${ABSL_STRINGS_LIB} pthread
)
set_target_properties(target_threat_query_assessment_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests" AUTOMOC OFF
)
add_test(NAME target_threat_query_assessment_test COMMAND target_threat_query_assessment_test)

add_executable(alarm_content_builder_test
    "${CMAKE_SOURCE_DIR}/tests/AlarmContentBuilderTest.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationEvaluator.cpp"
    "${ALARM_SRC_DIR}/dialog/alarm/AreaEscalationAlarmContent.cpp"
)
target_include_directories(alarm_content_builder_test PRIVATE "${ALARM_SRC_DIR}")
target_link_libraries(alarm_content_builder_test PRIVATE
    Qt5::Core Qt5::Gui Qt5::Sql Qt5::Concurrent Qt5::Network Qt5::Widgets)
set_target_properties(alarm_content_builder_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests"
    AUTOMOC OFF
)
add_test(NAME alarm_content_builder_test COMMAND alarm_content_builder_test)

if(UNIX AND NOT APPLE)
    # 必须与 find_package(Qt5) 实际链接的 lib 目录一致；硬编码 /usr 会在 conda/Qt5.15 链接下
    # 仍从 RPATH 先加载系统 libQt5Core.so.5，触发 Qt_5.15 not found。
    set(_alarmsys_qt5_core_lib "")
    foreach(_cfg RELEASE RELWITHDEBINFO MINSIZEREL DEBUG)
        get_target_property(_alarmsys_qt5_core_lib Qt5::Core "IMPORTED_LOCATION_${_cfg}")
        if(_alarmsys_qt5_core_lib)
            break()
        endif()
        set(_alarmsys_qt5_core_lib "")
    endforeach()
    if(NOT _alarmsys_qt5_core_lib)
        get_target_property(_alarmsys_qt5_core_lib Qt5::Core IMPORTED_LOCATION)
    endif()
    if(_alarmsys_qt5_core_lib)
        get_filename_component(_alarmsys_qt5_lib_dir "${_alarmsys_qt5_core_lib}" DIRECTORY)
    else()
        set(_alarmsys_qt5_lib_dir "/usr/lib/x86_64-linux-gnu")
        message(WARNING "未能解析 Qt5::Core 库路径，RPATH 退回 ${_alarmsys_qt5_lib_dir}")
    endif()
    target_link_options(alarmsys PRIVATE "LINKER:--disable-new-dtags")
    set_target_properties(alarmsys PROPERTIES
        INSTALL_RPATH "${_alarmsys_qt5_lib_dir};${FASTDDS_ROOT}/lib;${GRPC_ROOT}/lib"
        BUILD_WITH_INSTALL_RPATH TRUE
    )
endif()

set_target_properties(alarmsys PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)

add_custom_command(TARGET alarmsys POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/alarmevent_multi.xml"
        "$<TARGET_FILE_DIR:alarmsys>/alarmevent_multi.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/alarmevent_single.xml"
        "$<TARGET_FILE_DIR:alarmsys>/alarmevent_single.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/trackclass_recv_multi.xml"
        "$<TARGET_FILE_DIR:alarmsys>/trackclass_recv_multi.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/newtrack_sub_recv.xml"
        "$<TARGET_FILE_DIR:alarmsys>/newtrack_sub_recv.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/newtrack_alarm_multi.xml"
        "$<TARGET_FILE_DIR:alarmsys>/newtrack_alarm_multi.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/newtrack_alarm_single.xml"
        "$<TARGET_FILE_DIR:alarmsys>/newtrack_alarm_single.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/db.ini"
        "$<TARGET_FILE_DIR:alarmsys>/db.ini"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/Config.ini"
        "$<TARGET_FILE_DIR:alarmsys>/Config.ini"
    COMMENT "Copying runtime configs (DDS XML, db.ini, Config.ini) next to executable"
)

# 双 DDS 航迹字段对比（老 TrackDataClass vs 新 TargetOutputSet）
add_executable(track_dds_compare_test
    tools/track_dds_compare_test.cpp
    "${ALARM_SRC_DIR}/fastdds_track/TrackClassPubSubTypes.cxx"
    "${ALARM_SRC_DIR}/fastdds_track/TrackClassTypeObjectSupport.cxx"
    ${NEWTRACK_STRUCT_GEN_SRC}
)
target_include_directories(track_dds_compare_test PRIVATE
    "${ALARM_SRC_DIR}"
    "${ALARM_SRC_DIR}/fastdds_track"
    "${NEWTRACK_STRUCT_DIR}"
    "${FASTDDS_ROOT}/include"
)
target_link_libraries(track_dds_compare_test PRIVATE ${FASTDDS_LIB} ${FASTCDR_LIB} pthread)
set_target_properties(track_dds_compare_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
add_custom_command(TARGET track_dds_compare_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/trackclass_recv_multi.xml"
        "$<TARGET_FILE_DIR:track_dds_compare_test>/trackclass_recv_multi.xml"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/newtrack_sub_recv.xml"
        "$<TARGET_FILE_DIR:track_dds_compare_test>/newtrack_sub_recv.xml"
    COMMENT "Copy DDS XML for track_dds_compare_test"
)

add_executable(track_classified_type_probe
    tools/track_classified_type_probe.cpp
    ${NEWTRACK_STRUCT_GEN_SRC}
)
target_include_directories(track_classified_type_probe PRIVATE
    "${NEWTRACK_STRUCT_DIR}"
    "${FASTDDS_ROOT}/include"
)
target_link_libraries(track_classified_type_probe PRIVATE ${FASTDDS_LIB} ${FASTCDR_LIB} pthread)
set_target_properties(track_classified_type_probe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
add_custom_command(TARGET track_classified_type_probe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/newtrack_sub_recv.xml"
        "$<TARGET_FILE_DIR:track_classified_type_probe>/newtrack_sub_recv.xml"
    COMMENT "Copy DDS XML for track_classified_type_probe"
)

add_executable(track_field_probe
    tools/track_field_probe.cpp
    ${NEWTRACK_STRUCT_GEN_SRC}
)
target_include_directories(track_field_probe PRIVATE
    "${NEWTRACK_STRUCT_DIR}"
    "${FASTDDS_ROOT}/include"
)
target_link_libraries(track_field_probe PRIVATE ${FASTDDS_LIB} ${FASTCDR_LIB} pthread)
set_target_properties(track_field_probe PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
add_custom_command(TARGET track_field_probe POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/runtime/newtrack_sub_recv.xml"
        "$<TARGET_FILE_DIR:track_field_probe>/newtrack_sub_recv.xml"
    COMMENT "Copy DDS XML for track_field_probe"
)

# Linux libpq：AccessMode=1 时由 alarmsys 的 DatabaseManager 走 PgQt；Qt 路径(AccessMode=0)仍保留
find_path(LIBPQ_INCLUDE_DIR NAMES libpq-fe.h
    PATHS /usr/include/postgresql /usr/include /usr/local/include)
find_library(LIBPQ_LIB NAMES pq)
if(NOT LIBPQ_INCLUDE_DIR OR NOT LIBPQ_LIB)
    message(WARNING "未找到 libpq（libpq-dev）；AccessMode=1 不可用。安装: apt install libpq-dev")
else()
    set(PG_LIBPQ_SOURCES
        "${ALARM_SRC_DIR}/db/postgres/PgConnectionPool.cpp"
        "${ALARM_SRC_DIR}/db/postgres/PgDatabaseManager.cpp"
        "${ALARM_SRC_DIR}/db/postgres/PgHelper.cpp"
        "${ALARM_SRC_DIR}/db/postgres/PgQtDatabaseManager.cpp"
    )
    target_sources(alarmsys PRIVATE ${PG_LIBPQ_SOURCES})
    target_include_directories(alarmsys PRIVATE "${LIBPQ_INCLUDE_DIR}")
    target_link_libraries(alarmsys PRIVATE ${LIBPQ_LIB})
    message(STATUS "alarmsys linked libpq (${LIBPQ_LIB})")

    add_executable(pg_iface_test
        tools/pg_iface_test.cpp
        ${PG_LIBPQ_SOURCES}
    )
    target_include_directories(pg_iface_test PRIVATE
        "${ALARM_SRC_DIR}"
        "${LIBPQ_INCLUDE_DIR}"
    )
    target_link_libraries(pg_iface_test PRIVATE
        Qt5::Core
        ${LIBPQ_LIB}
        pthread
    )
    set_target_properties(pg_iface_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
        AUTOMOC OFF
    )
    message(STATUS "pg_iface_test enabled (libpq=${LIBPQ_LIB})")
endif()

