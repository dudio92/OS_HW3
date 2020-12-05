
cmake_minimum_required(VERSION 3.17)
project(OS_HW3 C)

set(CMAKE_C_STANDARD 11)

add_executable(OS_HW3 main.c message_slot.c message_slot.h message_sender.c message_reader.c)

