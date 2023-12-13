cmake_minimum_required(VERSION 3.10)
project(concurrency C)

set(CMAKE_C_STANDARD 11)

include_directories(. test_images test_files sod_118)

#configure_file(biscuit.txt . COPYONLY)


add_executable(SeqMain
        SeqMain.c
        PicProcess.c PicProcess.h
        PicStore.c PicStore.h
        Utils.c Utils.h
#        Compare.c
        sod_118/sod.c sod_118/sod.h
        pool/thpool.c pool/thpool.h
        Picture.c Picture.h)
target_compile_options(SeqMain PRIVATE -DMAIN)

#add_executable(test
#        test.c
#        laser.c laser.h
#        SeqMain.c)
#target_compile_options(test PRIVATE -DTEST)
