#####################################################################
# CS:APP Malloc Lab
# Handout files for students
#
# Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
# May not be used, modified, or copied without permission.
#
######################################################################

***********
Main Files:
***********

mm.{c,h}	
	Your solution malloc package. mm.c is the file that you
	will be handing in, and is the only file you should modify.

mdriver.c	
	The malloc driver that tests your mm.c file

short{1,2}-bal.rep
	Two tiny tracefiles to help you get started. 

Makefile	
	Builds the driver

**********************************
Other support files for the driver
**********************************

config.h	Configures the malloc lab driver
fsecs.{c,h}	Wrapper function for the different timer packages
clock.{c,h}	Routines for accessing the Pentium and Alpha cycle counters
fcyc.{c,h}	Timer functions based on cycle counters
ftimer.{c,h}	Timer functions based on interval timers and gettimeofday()
memlib.{c,h}	Models the heap and sbrk function

*******************************
Building and running the driver
*******************************
To build the driver, type "make" to the shell.

To run the driver on a tiny test trace:

	unix> mdriver -V -f short1-bal.rep

The -V option prints out helpful tracing and summary information.

To get a list of the driver flags:

	unix> mdriver -h

/*
 * 언리얼 스타일 메모리 할당기 구조
 *
 * [구분]                     [방식]                                 [특징]
 *
 * 메모리 관리 구조           Segregated Free List                   요청 크기에 따라 32개 bin(`bins[]`)의 분리형 free list 사용.
 *                                                                  각 bin은 특정 크기 범위의 free 블록만 관리.
 *                                                                  BIN_MAX_SIZE(=512) 초과 블록은 별도 `large_listp`에서 관리.
 *                                                                  크기별 분리 관리로 탐색 효율 향상, 단편화 감소.
 *
 * Free List 연결             Explicit Doubly Linked List            모든 free 블록은 pred/succ 포인터 포함 이중 연결 리스트로 연결.
 *                                                                  large_listp는 주소 오름차순 유지(coalesce 효율↑).
 *                                                                  small bin은 LIFO(맨 앞에 삽입, first-fit 최적화).
 *
 * 탐색 정책(find_fit)        First-Fit(bin) / Best-Fit(large)       small bin은 first-fit(LIFO), large_list는 best-fit 탐색.
 *                                                                  요청 크기 이상인 첫 블록을 즉시 할당(Unreal 엔진 실제 방식과 유사).
 *
 * 삽입 정책                  주소 오름차순(large) / LIFO(small)     large_listp는 주소순, small bin은 맨 앞(LIFO) 삽입 → 탐색/병합 효율 ↑
 *
 * 할당 정책(place)           분할 시 최소 블록 크기 보장            남는 블록이 MIN_BLOCK_SIZE 이상일 때만 분할.
 *
 * 병합 정책(coalesce)        즉시 병합                              인접 free 블록과 즉시 병합 후 bin/large list에 재삽입.
 *
 * 힙 확장                    mem_sbrk()                             fit 실패 시 CHUNKSIZE 또는 요청 크기만큼 확장.
 *
 * 블록 구조                  Header + Footer + Payload(+pred/succ)  free 블록은 pred/succ 포인터 포함, 모든 블록 8바이트 정렬.
 *
 * 정렬 단위                  8바이트 (ALIGNMENT = 8)                모든 블록 크기를 8바이트 단위로 정렬.
 */
