#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Release
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/libtrudp/src/hash.o \
	${OBJECTDIR}/libtrudp/src/map.o \
	${OBJECTDIR}/libtrudp/src/packet.o \
	${OBJECTDIR}/libtrudp/src/packet_queue.o \
	${OBJECTDIR}/libtrudp/src/queue.o \
	${OBJECTDIR}/libtrudp/src/tr-udp.o \
	${OBJECTDIR}/libtrudp/src/tr-udp_stat.o \
	${OBJECTDIR}/libtrudp/src/udp.o \
	${OBJECTDIR}/libtrudp/src/utils.o \
	${OBJECTDIR}/libtrudp/src/write_queue.o \
	${OBJECTDIR}/main.o \
	${OBJECTDIR}/snake.o \
	${OBJECTDIR}/trudpcat.o \
	${OBJECTDIR}/trudpcat_ev.o

# Test Directory
TESTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}/tests

# Test Files
TESTFILES= \
	${TESTDIR}/TestFiles/f1

# Test Object Files
TESTOBJECTFILES= \
	${TESTDIR}/libtrudp/tests/map_t.o \
	${TESTDIR}/libtrudp/tests/packet_queue_t.o \
	${TESTDIR}/libtrudp/tests/packet_t.o \
	${TESTDIR}/libtrudp/tests/queue_t.o \
	${TESTDIR}/libtrudp/tests/tr-udp_t.o

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=-lev

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trudp

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trudp: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${LINK.cc} -o ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trudp ${OBJECTFILES} ${LDLIBSOPTIONS}

${OBJECTDIR}/libtrudp/src/hash.o: libtrudp/src/hash.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/hash.o libtrudp/src/hash.c

${OBJECTDIR}/libtrudp/src/map.o: libtrudp/src/map.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/map.o libtrudp/src/map.c

${OBJECTDIR}/libtrudp/src/packet.o: libtrudp/src/packet.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/packet.o libtrudp/src/packet.c

${OBJECTDIR}/libtrudp/src/packet_queue.o: libtrudp/src/packet_queue.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/packet_queue.o libtrudp/src/packet_queue.c

${OBJECTDIR}/libtrudp/src/queue.o: libtrudp/src/queue.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/queue.o libtrudp/src/queue.c

${OBJECTDIR}/libtrudp/src/tr-udp.o: libtrudp/src/tr-udp.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/tr-udp.o libtrudp/src/tr-udp.c

${OBJECTDIR}/libtrudp/src/tr-udp_stat.o: libtrudp/src/tr-udp_stat.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/tr-udp_stat.o libtrudp/src/tr-udp_stat.c

${OBJECTDIR}/libtrudp/src/udp.o: libtrudp/src/udp.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/udp.o libtrudp/src/udp.c

${OBJECTDIR}/libtrudp/src/utils.o: libtrudp/src/utils.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/utils.o libtrudp/src/utils.c

${OBJECTDIR}/libtrudp/src/write_queue.o: libtrudp/src/write_queue.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/write_queue.o libtrudp/src/write_queue.c

${OBJECTDIR}/main.o: main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.cc) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main.o main.cpp

${OBJECTDIR}/snake.o: snake.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/snake.o snake.c

${OBJECTDIR}/trudpcat.o: trudpcat.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/trudpcat.o trudpcat.c

${OBJECTDIR}/trudpcat_ev.o: trudpcat_ev.c 
	${MKDIR} -p ${OBJECTDIR}
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/trudpcat_ev.o trudpcat_ev.c

# Subprojects
.build-subprojects:

# Build Test Targets
.build-tests-conf: .build-tests-subprojects .build-conf ${TESTFILES}
.build-tests-subprojects:

${TESTDIR}/TestFiles/f1: ${TESTDIR}/libtrudp/tests/map_t.o ${TESTDIR}/libtrudp/tests/packet_queue_t.o ${TESTDIR}/libtrudp/tests/packet_t.o ${TESTDIR}/libtrudp/tests/queue_t.o ${TESTDIR}/libtrudp/tests/tr-udp_t.o ${OBJECTFILES:%.o=%_nomain.o}
	${MKDIR} -p ${TESTDIR}/TestFiles
	${LINK.cc}   -o ${TESTDIR}/TestFiles/f1 $^ ${LDLIBSOPTIONS} -lcunit 


${TESTDIR}/libtrudp/tests/map_t.o: libtrudp/tests/map_t.c 
	${MKDIR} -p ${TESTDIR}/libtrudp/tests
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${TESTDIR}/libtrudp/tests/map_t.o libtrudp/tests/map_t.c


${TESTDIR}/libtrudp/tests/packet_queue_t.o: libtrudp/tests/packet_queue_t.c 
	${MKDIR} -p ${TESTDIR}/libtrudp/tests
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${TESTDIR}/libtrudp/tests/packet_queue_t.o libtrudp/tests/packet_queue_t.c


${TESTDIR}/libtrudp/tests/packet_t.o: libtrudp/tests/packet_t.c 
	${MKDIR} -p ${TESTDIR}/libtrudp/tests
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${TESTDIR}/libtrudp/tests/packet_t.o libtrudp/tests/packet_t.c


${TESTDIR}/libtrudp/tests/queue_t.o: libtrudp/tests/queue_t.c 
	${MKDIR} -p ${TESTDIR}/libtrudp/tests
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${TESTDIR}/libtrudp/tests/queue_t.o libtrudp/tests/queue_t.c


${TESTDIR}/libtrudp/tests/tr-udp_t.o: libtrudp/tests/tr-udp_t.c 
	${MKDIR} -p ${TESTDIR}/libtrudp/tests
	${RM} "$@.d"
	$(COMPILE.c) -O2 -MMD -MP -MF "$@.d" -o ${TESTDIR}/libtrudp/tests/tr-udp_t.o libtrudp/tests/tr-udp_t.c


${OBJECTDIR}/libtrudp/src/hash_nomain.o: ${OBJECTDIR}/libtrudp/src/hash.o libtrudp/src/hash.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/hash.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/hash_nomain.o libtrudp/src/hash.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/hash.o ${OBJECTDIR}/libtrudp/src/hash_nomain.o;\
	fi

${OBJECTDIR}/libtrudp/src/map_nomain.o: ${OBJECTDIR}/libtrudp/src/map.o libtrudp/src/map.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/map.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/map_nomain.o libtrudp/src/map.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/map.o ${OBJECTDIR}/libtrudp/src/map_nomain.o;\
	fi

${OBJECTDIR}/libtrudp/src/packet_nomain.o: ${OBJECTDIR}/libtrudp/src/packet.o libtrudp/src/packet.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/packet.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/packet_nomain.o libtrudp/src/packet.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/packet.o ${OBJECTDIR}/libtrudp/src/packet_nomain.o;\
	fi

${OBJECTDIR}/libtrudp/src/packet_queue_nomain.o: ${OBJECTDIR}/libtrudp/src/packet_queue.o libtrudp/src/packet_queue.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/packet_queue.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/packet_queue_nomain.o libtrudp/src/packet_queue.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/packet_queue.o ${OBJECTDIR}/libtrudp/src/packet_queue_nomain.o;\
	fi

${OBJECTDIR}/libtrudp/src/queue_nomain.o: ${OBJECTDIR}/libtrudp/src/queue.o libtrudp/src/queue.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/queue.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/queue_nomain.o libtrudp/src/queue.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/queue.o ${OBJECTDIR}/libtrudp/src/queue_nomain.o;\
	fi

${OBJECTDIR}/libtrudp/src/tr-udp_nomain.o: ${OBJECTDIR}/libtrudp/src/tr-udp.o libtrudp/src/tr-udp.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/tr-udp.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/tr-udp_nomain.o libtrudp/src/tr-udp.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/tr-udp.o ${OBJECTDIR}/libtrudp/src/tr-udp_nomain.o;\
	fi

${OBJECTDIR}/libtrudp/src/tr-udp_stat_nomain.o: ${OBJECTDIR}/libtrudp/src/tr-udp_stat.o libtrudp/src/tr-udp_stat.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/tr-udp_stat.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/tr-udp_stat_nomain.o libtrudp/src/tr-udp_stat.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/tr-udp_stat.o ${OBJECTDIR}/libtrudp/src/tr-udp_stat_nomain.o;\
	fi

${OBJECTDIR}/libtrudp/src/udp_nomain.o: ${OBJECTDIR}/libtrudp/src/udp.o libtrudp/src/udp.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/udp.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/udp_nomain.o libtrudp/src/udp.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/udp.o ${OBJECTDIR}/libtrudp/src/udp_nomain.o;\
	fi

${OBJECTDIR}/libtrudp/src/utils_nomain.o: ${OBJECTDIR}/libtrudp/src/utils.o libtrudp/src/utils.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/utils.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/utils_nomain.o libtrudp/src/utils.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/utils.o ${OBJECTDIR}/libtrudp/src/utils_nomain.o;\
	fi

${OBJECTDIR}/libtrudp/src/write_queue_nomain.o: ${OBJECTDIR}/libtrudp/src/write_queue.o libtrudp/src/write_queue.c 
	${MKDIR} -p ${OBJECTDIR}/libtrudp/src
	@NMOUTPUT=`${NM} ${OBJECTDIR}/libtrudp/src/write_queue.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/libtrudp/src/write_queue_nomain.o libtrudp/src/write_queue.c;\
	else  \
	    ${CP} ${OBJECTDIR}/libtrudp/src/write_queue.o ${OBJECTDIR}/libtrudp/src/write_queue_nomain.o;\
	fi

${OBJECTDIR}/main_nomain.o: ${OBJECTDIR}/main.o main.cpp 
	${MKDIR} -p ${OBJECTDIR}
	@NMOUTPUT=`${NM} ${OBJECTDIR}/main.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.cc) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/main_nomain.o main.cpp;\
	else  \
	    ${CP} ${OBJECTDIR}/main.o ${OBJECTDIR}/main_nomain.o;\
	fi

${OBJECTDIR}/snake_nomain.o: ${OBJECTDIR}/snake.o snake.c 
	${MKDIR} -p ${OBJECTDIR}
	@NMOUTPUT=`${NM} ${OBJECTDIR}/snake.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/snake_nomain.o snake.c;\
	else  \
	    ${CP} ${OBJECTDIR}/snake.o ${OBJECTDIR}/snake_nomain.o;\
	fi

${OBJECTDIR}/trudpcat_nomain.o: ${OBJECTDIR}/trudpcat.o trudpcat.c 
	${MKDIR} -p ${OBJECTDIR}
	@NMOUTPUT=`${NM} ${OBJECTDIR}/trudpcat.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/trudpcat_nomain.o trudpcat.c;\
	else  \
	    ${CP} ${OBJECTDIR}/trudpcat.o ${OBJECTDIR}/trudpcat_nomain.o;\
	fi

${OBJECTDIR}/trudpcat_ev_nomain.o: ${OBJECTDIR}/trudpcat_ev.o trudpcat_ev.c 
	${MKDIR} -p ${OBJECTDIR}
	@NMOUTPUT=`${NM} ${OBJECTDIR}/trudpcat_ev.o`; \
	if (echo "$$NMOUTPUT" | ${GREP} '|main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T main$$') || \
	   (echo "$$NMOUTPUT" | ${GREP} 'T _main$$'); \
	then  \
	    ${RM} "$@.d";\
	    $(COMPILE.c) -O2 -Dmain=__nomain -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/trudpcat_ev_nomain.o trudpcat_ev.c;\
	else  \
	    ${CP} ${OBJECTDIR}/trudpcat_ev.o ${OBJECTDIR}/trudpcat_ev_nomain.o;\
	fi

# Run Test Targets
.test-conf:
	@if [ "${TEST}" = "" ]; \
	then  \
	    ${TESTDIR}/TestFiles/f1 || true; \
	else  \
	    ./${TEST} || true; \
	fi

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/trudp

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
