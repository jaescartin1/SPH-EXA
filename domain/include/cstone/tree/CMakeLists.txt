if(CMAKE_HIP_COMPILER)
    set_source_files_properties(octree_gpu.cu PROPERTIES LANGUAGE HIP)
endif()

if(CMAKE_CUDA_COMPILER OR CMAKE_HIP_COMPILER)
    add_library(octree_obj OBJECT octree_gpu.cu)
    target_include_directories(octree_obj PRIVATE ${PROJECT_SOURCE_DIR}/include)
endif()