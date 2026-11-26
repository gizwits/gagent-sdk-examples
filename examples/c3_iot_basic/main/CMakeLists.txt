idf_component_register(SRCS "app_main.cc" "board.cc" "gizwits_product.cc" "generated/gizwits_protocol.cc"
    INCLUDE_DIRS "." "generated" "../gagent-sdk/board"
    REQUIRES gagent_core gagent_esp gagent_cloud_gizwits gagent_chip_policy)
target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
